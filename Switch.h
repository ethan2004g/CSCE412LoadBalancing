/**
 * @file Switch.h
 * @brief Definition of the Switch class for bonus: route jobs by type to dedicated load balancers.
 *
 * The Switch acts as a higher-level load balancer that routes incoming requests
 * to one of two LoadBalancer instances based on job type: Streaming ('S') and
 * Processing ('P'). This satisfies the bonus objective of routing by job type
 * and running multiple load balancers with separate statistics.
 */

#pragma once

#include <iosfwd>

#include "LoadBalancer.h"
#include "Request.h"
#include "Config.h"

/**
 * @brief Routes requests by job type to a dedicated streaming or processing load balancer.
 *
 * Runs two LoadBalancer instances in lockstep (one cycle of each per simulation cycle),
 * keeping separate statistics for each pool.
 */
class Switch
{
public:
    /**
     * @brief Construct a Switch with two load balancer pools.
     *
     * @param initialServersPerPool Initial number of web servers in each pool (streaming and processing).
     * @param totalCycles Total number of clock cycles to simulate.
     * @param config Simulation configuration (task times, arrival probability, etc.).
     */
    Switch(int initialServersPerPool, int totalCycles, const SimulationConfig &config);

    /**
     * @brief Populate both pools with an initial queue (requests distributed by job type).
     *
     * Generates (initialServersPerPool * 100) requests per pool and routes each to
     * the appropriate load balancer by job type.
     */
    void populateInitialQueue();

    /**
     * @brief Run the simulation: each cycle, generate and route new requests, then run one cycle on each LB.
     *
     * @param log Output stream for logging.
     */
    void runSimulation(std::ostream &log);

    /**
     * @brief Get total queue size across both load balancers.
     */
    int getTotalQueueSize() const;

    /**
     * @brief Print a summary including per–load balancer statistics and combined totals.
     *
     * @param out Output stream for the summary.
     */
    void printSummary(std::ostream &out) const;

private:
    void generateAndRouteRequests();

    SimulationConfig config_;
    int totalCycles_;
    int initialServersPerPool_;

    LoadBalancer streamingLB_;   ///< Load balancer for job type 'S' (streaming).
    LoadBalancer processingLB_;  ///< Load balancer for job type 'P' (processing).
};
