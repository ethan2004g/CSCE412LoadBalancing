/**
 * @file LoadBalancer.cpp
 * @brief Implementation of the LoadBalancer class.
 */

#include "LoadBalancer.h"

#include <iostream>
#include <random>
#include <sstream>
#include <iomanip>

namespace
{
    // ANSI color codes for optional colored console output.
    const char *COLOR_GREEN = "\033[32m";
    const char *COLOR_RED = "\033[31m";
    const char *COLOR_RESET = "\033[0m";
}

LoadBalancer::LoadBalancer(int initialServers, int totalCycles, const SimulationConfig &config)
    : totalCycles_(totalCycles),
      currentCycle_(0),
      config_(config),
      startingQueueSize_(0),
      totalGenerated_(0),
      totalCompleted_(0),
      totalBlocked_(0),
      totalDiscarded_(0),
      maxQueueSize_(0),
      minServerCount_(initialServers),
      maxServerCount_(initialServers),
      lastAdjustCycle_(0),
      maxQueueCapacity_(initialServers * config.maxQueuePerServer),
      endingQueueSize_(0)
{
    if (initialServers < 1)
    {
        initialServers = 1;
    }

    for (int i = 0; i < initialServers; ++i)
    {
        servers_.emplace_back(i);
    }
}

void LoadBalancer::populateInitialQueue()
{
    int initialRequests = static_cast<int>(servers_.size()) * 100;
    for (int i = 0; i < initialRequests; ++i)
    {
        Request r = generateRandomRequest(config_.minTaskTime, config_.maxTaskTime);
        if (isBlockedIp(r.ipIn))
        {
            ++totalBlocked_;
            continue;
        }
        requestQueue_.push(r);
        ++totalGenerated_;
    }
    startingQueueSize_ = static_cast<int>(requestQueue_.size());
    maxQueueSize_ = startingQueueSize_;
}

void LoadBalancer::addRequest(const Request &request)
{
    if (isBlockedIp(request.ipIn))
    {
        ++totalBlocked_;
        return;
    }
    if (static_cast<int>(requestQueue_.size()) >= maxQueueCapacity_)
    {
        ++totalDiscarded_;
        return;
    }
    requestQueue_.push(request);
    ++totalGenerated_;
}

void LoadBalancer::updateQueueMax()
{
    int q = static_cast<int>(requestQueue_.size());
    if (q > maxQueueSize_)
    {
        maxQueueSize_ = q;
    }
}

void LoadBalancer::runOneCycle(std::ostream &log, bool generateNewRequests)
{
    if (currentCycle_ >= totalCycles_)
    {
        return;
    }
    if (generateNewRequests)
    {
        generateRandomRequests();
    }
    assignRequestsToServers();
    processServers();
    adjustServersIfNeeded(log);
    updateQueueMax();
    if (currentCycle_ % 50 == 0)
    {
        logCycle(log);
    }
    ++currentCycle_;
    if (currentCycle_ >= totalCycles_)
    {
        endingQueueSize_ = static_cast<int>(requestQueue_.size());
    }
}

void LoadBalancer::runSimulation(std::ostream &log)
{
    currentCycle_ = 0;
    while (currentCycle_ < totalCycles_)
    {
        runOneCycle(log, true);
    }
    endingQueueSize_ = static_cast<int>(requestQueue_.size());
    logCycle(log);
}

int LoadBalancer::getQueueSize() const
{
    return static_cast<int>(requestQueue_.size());
}

void LoadBalancer::recordStartingState()
{
    startingQueueSize_ = getQueueSize();
    if (startingQueueSize_ > maxQueueSize_)
    {
        maxQueueSize_ = startingQueueSize_;
    }
}

void LoadBalancer::printSummary(std::ostream &out) const
{
    out << "\n=== Load Balancer Simulation Summary ===\n";
    out << "Starting queue size: " << startingQueueSize_ << "\n";
    out << "Ending queue size:   " << endingQueueSize_ << "\n";
    out << "Total requests generated:  " << totalGenerated_ << "\n";
    out << "Total requests completed:  " << totalCompleted_ << "\n";
    out << "Total requests blocked (firewall): " << totalBlocked_ << "\n";
    out << "Total requests discarded (queue full, cap=" << maxQueueCapacity_ << "): " << totalDiscarded_ << "\n";
    out << "Max queue size observed:   " << maxQueueSize_ << "\n";
    out << "Min server count: " << minServerCount_ << "\n";
    out << "Max server count: " << maxServerCount_ << "\n";
    out << "Final server count: " << servers_.size() << "\n";
    out << "Task time range: [" << config_.minTaskTime << ", " << config_.maxTaskTime << "]\n";
    out << "========================================\n";
}

void LoadBalancer::printRequestLog(std::ostream &out) const
{
    out << "\n=== Per-Request Event Log ===\n";
    out << "Format: [Cycle] START/END | Server | IP In | IP Out | Job Type | Duration (start only)\n";
    out << "-------------------------------------------------------------------------------------\n";
    for (const auto &entry : requestLog_)
    {
        out << entry << "\n";
    }
    out << "=== End Per-Request Event Log ===\n";
}

void LoadBalancer::generateRandomRequests()
{
    // Decide probabilistically whether to generate a new request this cycle.
    static std::mt19937 gen(std::random_device{}());
    std::uniform_real_distribution<double> dist(0.0, 1.0);

    double value = dist(gen);
    if (value <= config_.requestArrivalProbability)
    {
        Request r = generateRandomRequest(config_.minTaskTime, config_.maxTaskTime);
        if (isBlockedIp(r.ipIn))
        {
            ++totalBlocked_;
            return;
        }
        if (static_cast<int>(requestQueue_.size()) >= maxQueueCapacity_)
        {
            ++totalDiscarded_;
            return;
        }
        requestQueue_.push(r);
        ++totalGenerated_;
    }
}

void LoadBalancer::assignRequestsToServers()
{
    for (auto &server : servers_)
    {
        if (server.isIdle() && !requestQueue_.empty())
        {
            Request r = requestQueue_.front();
            requestQueue_.pop();
            server.assignRequest(r);

            std::ostringstream entry;
            entry << "[Cycle " << std::setw(6) << currentCycle_ << "] "
                  << "REQUEST START | Server " << std::setw(3) << server.getId()
                  << " | IP In: "  << std::setw(15) << std::left << r.ipIn
                  << " | IP Out: " << std::setw(15) << std::left << r.ipOut
                  << " | Job: " << r.jobType
                  << " | Duration: " << r.timeRemaining << " cycles"
                  << std::right;
            requestLog_.push_back(entry.str());
        }
    }
}

void LoadBalancer::processServers()
{
    for (auto &server : servers_)
    {
        server.tick();
        if (server.hasJustCompleted())
        {
            ++totalCompleted_;
            const Request &req = server.getCurrentRequest();

            std::ostringstream entry;
            entry << "[Cycle " << std::setw(6) << currentCycle_ << "] "
                  << "REQUEST END   | Server " << std::setw(3) << server.getId()
                  << " | IP In: "  << std::setw(15) << std::left << req.ipIn
                  << " | IP Out: " << std::setw(15) << std::left << req.ipOut
                  << " | Job: " << req.jobType
                  << std::right;
            requestLog_.push_back(entry.str());

            server.clearJustCompleted();
        }
    }
}

void LoadBalancer::adjustServersIfNeeded(std::ostream &log)
{
    int queueSize = static_cast<int>(requestQueue_.size());
    int serverCount = static_cast<int>(servers_.size());

    int lowerThreshold = 50 * serverCount;
    int upperThreshold = 80 * serverCount;

    if (currentCycle_ - lastAdjustCycle_ < config_.adjustCooldownCycles)
    {
        return;
    }

    if (queueSize > upperThreshold)
    {
        int newId = serverCount;
        servers_.emplace_back(newId);
        lastAdjustCycle_ = currentCycle_;

        std::string msg = "[Cycle " + std::to_string(currentCycle_) +
                          "] SCALE UP: queue=" + std::to_string(queueSize) +
                          " > upper threshold " + std::to_string(upperThreshold) +
                          " (80 x " + std::to_string(serverCount) + ")" +
                          " => servers: " + std::to_string(serverCount) +
                          " -> " + std::to_string(static_cast<int>(servers_.size()));

        std::cout << COLOR_GREEN << msg << COLOR_RESET << "\n";
        log << msg << "\n";

        if (static_cast<int>(servers_.size()) > maxServerCount_)
        {
            maxServerCount_ = static_cast<int>(servers_.size());
        }
    }
    else if (queueSize < lowerThreshold && serverCount > 1)
    {
        // Remove the first idle server found.
        for (auto it = servers_.begin(); it != servers_.end(); ++it)
        {
            if (it->isIdle())
            {
                servers_.erase(it);
                lastAdjustCycle_ = currentCycle_;

                std::string msg = "[Cycle " + std::to_string(currentCycle_) +
                                  "] SCALE DOWN: queue=" + std::to_string(queueSize) +
                                  " < lower threshold " + std::to_string(lowerThreshold) +
                                  " (50 x " + std::to_string(serverCount) + ")" +
                                  " => servers: " + std::to_string(serverCount) +
                                  " -> " + std::to_string(static_cast<int>(servers_.size()));

                std::cout << COLOR_RED << msg << COLOR_RESET << "\n";
                log << msg << "\n";

                if (static_cast<int>(servers_.size()) < minServerCount_)
                {
                    minServerCount_ = static_cast<int>(servers_.size());
                }
                break;
            }
        }
    }
}

void LoadBalancer::logCycle(std::ostream &log) const
{
    int activeServers = 0;
    int idleServers = 0;
    for (const auto &s : servers_)
    {
        if (s.isIdle())
        {
            ++idleServers;
        }
        else
        {
            ++activeServers;
        }
    }

    int serverCount = static_cast<int>(servers_.size());
    int queueSize   = static_cast<int>(requestQueue_.size());
    int lowerThresh = 50 * serverCount;
    int upperThresh = 80 * serverCount;

    const char *status = "IN RANGE";
    if (queueSize > upperThresh)       status = "OVERLOADED";
    else if (queueSize < lowerThresh)  status = "UNDERLOADED";

    log << "Cycle " << currentCycle_
        << ": Queue=" << queueSize
        << " [target " << lowerThresh << "-" << upperThresh << ", cap=" << maxQueueCapacity_ << "] (" << status << ")"
        << ", Servers=" << serverCount
        << " (active=" << activeServers << " idle=" << idleServers << ")"
        << ", Completed=" << totalCompleted_
        << ", Blocked=" << totalBlocked_
        << ", Discarded=" << totalDiscarded_
        << "\n";
}

