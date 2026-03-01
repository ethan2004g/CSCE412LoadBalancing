/**
 * @file LoadBalancer.h
 * @brief Definition of the LoadBalancer class.
 */

#pragma once

#include <queue>
#include <vector>
#include <string>
#include <iosfwd>

#include "Request.h"
#include "WebServer.h"
#include "Config.h"

/**
 * @brief Manages a pool of web servers and a queue of requests.
 *
 * The LoadBalancer is responsible for:
 * - Assigning queued requests to idle servers.
 * - Generating new random requests over time.
 * - Dynamically adding or removing servers to keep the queue size within
 *   configured thresholds.
 * - Tracking statistics and logging simulation progress.
 */
class LoadBalancer
{
public:
    /**
     * @brief Construct a new LoadBalancer.
     *
     * @param initialServers Initial number of web servers.
     * @param totalCycles Total number of clock cycles to simulate.
     * @param config Simulation configuration parameters.
     */
    LoadBalancer(int initialServers, int totalCycles, const SimulationConfig &config);

    /**
     * @brief Populate the initial request queue with servers * 100 requests.
     */
    void populateInitialQueue();

    /**
     * @brief Add a single request to the queue (e.g. when used under a Switch).
     * Blocked IPs are not added; totalBlocked_ is incremented instead.
     *
     * @param request The request to add.
     */
    void addRequest(const Request &request);

    /**
     * @brief Run one clock cycle: optionally generate new requests, assign, process, adjust.
     *
     * @param log Output stream for logging.
     * @param generateNewRequests If true, probabilistically add new requests this cycle.
     */
    void runOneCycle(std::ostream &log, bool generateNewRequests = true);

    /**
     * @brief Run the simulation, writing progress information to the log stream.
     *
     * @param log Output stream for logging (e.g., a file or std::cout).
     */
    void runSimulation(std::ostream &log);

    /**
     * @brief Get the current size of the request queue.
     *
     * @return Queue size as an integer.
     */
    int getQueueSize() const;

    /**
     * @brief Record current queue size as the starting size (for use when LB is fed by a Switch).
     */
    void recordStartingState();

    /**
     * @brief Print a summary of the simulation to the given stream.
     *
     * @param out Output stream where the summary will be written.
     */
    void printSummary(std::ostream &out) const;

    /**
     * @brief Print a detailed per-request log showing start and end events.
     *
     * Each entry includes the cycle number, server ID, IP in, IP out, job type,
     * and (for start events) the assigned duration in clock cycles.
     *
     * @param out Output stream where the request log will be written.
     */
    void printRequestLog(std::ostream &out) const;

private:
    void generateRandomRequests();
    void assignRequestsToServers();
    void processServers();
    void adjustServersIfNeeded(std::ostream &log);
    void logCycle(std::ostream &log) const;
    void updateQueueMax();

    int totalCycles_;
    int currentCycle_;
    SimulationConfig config_;

    std::queue<Request> requestQueue_;
    std::vector<WebServer> servers_;

    // Statistics
    int startingQueueSize_;
    int totalGenerated_;
    int totalCompleted_;
    int totalBlocked_;
    int totalDiscarded_;
    int maxQueueSize_;
    int minServerCount_;
    int maxServerCount_;
    int lastAdjustCycle_;

    // Fixed absolute queue cap (set once from initial servers * maxQueuePerServer).
    int maxQueueCapacity_;

    // Cached values for final summary
    int endingQueueSize_;

    // Per-request start/end event log
    std::vector<std::string> requestLog_;
};

