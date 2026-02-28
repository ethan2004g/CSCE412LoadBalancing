/**
 * @file Config.h
 * @brief Definition of the SimulationConfig struct and helper functions.
 */

#pragma once

#include <string>

/**
 * @brief Configuration parameters for the load balancer simulation.
 */
struct SimulationConfig
{
    int minTaskTime;               ///< Minimum task processing time in cycles.
    int maxTaskTime;               ///< Maximum task processing time in cycles.
    double requestArrivalProbability; ///< Probability of a new request per cycle.
    int adjustCooldownCycles;      ///< Cooldown cycles between server scale actions.
    std::string blockedIpPrefix = "10.";      ///< IP prefix to block (firewall/DOS prevention).
    int maxQueuePerServer = 110;              ///< Max queue size per server before new requests are discarded.
};

/**
 * @brief Attempt to load simulation configuration from a simple key=value file.
 *
 * Supported keys:
 * - minTaskTime
 * - maxTaskTime
 * - requestArrivalProbability
 * - adjustCooldownCycles
 *
 * @param path Path to the configuration file.
 * @param outConfig Output parameter for the loaded configuration.
 * @return true if the file was read and parsed successfully; false otherwise.
 */
bool loadConfigFromFile(const std::string &path, SimulationConfig &outConfig);

