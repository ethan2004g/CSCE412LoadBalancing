/**
 * @file Switch.cpp
 * @brief Implementation of the Switch class for routing by job type.
 */

#include "Switch.h"

#include <iostream>
#include <random>

namespace
{
    std::mt19937 &switchRng()
    {
        static std::random_device rd;
        static std::mt19937 gen(rd());
        return gen;
    }
}

Switch::Switch(int initialServersPerPool, int totalCycles, const SimulationConfig &config)
    : config_(config),
      totalCycles_(totalCycles),
      initialServersPerPool_(initialServersPerPool),
      streamingLB_(initialServersPerPool, totalCycles, config),
      processingLB_(initialServersPerPool, totalCycles, config)
{
}

void Switch::populateInitialQueue()
{
    int requestsPerPool = initialServersPerPool_ * 100;
    int totalInitial = requestsPerPool * 2;

    for (int i = 0; i < totalInitial; ++i)
    {
        Request r = generateRandomRequest(config_.minTaskTime, config_.maxTaskTime);
        if (r.jobType == 'S')
        {
            streamingLB_.addRequest(r);
        }
        else
        {
            processingLB_.addRequest(r);
        }
    }

    streamingLB_.recordStartingState();
    processingLB_.recordStartingState();
}

int Switch::getTotalQueueSize() const
{
    return streamingLB_.getQueueSize() + processingLB_.getQueueSize();
}

void Switch::generateAndRouteRequests()
{
    std::uniform_real_distribution<double> dist(0.0, 1.0);
    if (dist(switchRng()) <= config_.requestArrivalProbability)
    {
        Request r = generateRandomRequest(config_.minTaskTime, config_.maxTaskTime);
        if (r.jobType == 'S')
        {
            streamingLB_.addRequest(r);
        }
        else
        {
            processingLB_.addRequest(r);
        }
    }
}

void Switch::runSimulation(std::ostream &log)
{
    log << "Switch mode: routing Streaming (S) and Processing (P) to separate load balancers.\n\n";

    for (int cycle = 0; cycle < totalCycles_; ++cycle)
    {
        generateAndRouteRequests();
        streamingLB_.runOneCycle(log, false);
        processingLB_.runOneCycle(log, false);
    }

    streamingLB_.logFinalState(log);
    processingLB_.logFinalState(log);
}

void Switch::printSummary(std::ostream &out) const
{
    out << "\n========== Switch (Bonus) – Summary ==========\n";
    out << "Two load balancers run concurrently: Streaming (S) and Processing (P).\n\n";

    out << "--- Streaming Load Balancer ---\n";
    streamingLB_.printSummary(out);

    out << "\n--- Processing Load Balancer ---\n";
    processingLB_.printSummary(out);

    out << "\n========== End Switch Summary ==========\n";
}

void Switch::printRequestLog(std::ostream &out) const
{
    out << "\n========== Switch – Per-Request Event Log ==========\n";

    out << "\n--- Streaming Load Balancer ---\n";
    streamingLB_.printRequestLog(out);

    out << "\n--- Processing Load Balancer ---\n";
    processingLB_.printRequestLog(out);

    out << "\n========== End Switch Per-Request Event Log ==========\n";
}
