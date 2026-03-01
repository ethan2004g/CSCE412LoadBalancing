/**
 * @file Config.cpp
 * @brief Implementation of configuration loading for the simulation.
 */

#include "Config.h"

#include <fstream>
#include <sstream>

bool loadConfigFromFile(const std::string &path, SimulationConfig &outConfig)
{
    std::ifstream in(path);
    if (!in.is_open())
    {
        return false;
    }

    SimulationConfig cfg = outConfig; // start with existing values as defaults

    std::string line;
    while (std::getline(in, line))
    {
        if (line.empty() || line[0] == '#')
        {
            continue;
        }

        std::istringstream iss(line);
        std::string key;
        if (!std::getline(iss, key, '='))
        {
            continue;
        }

        std::string valueStr;
        if (!std::getline(iss, valueStr))
        {
            continue;
        }

        std::istringstream vs(valueStr);

        if (key == "minTaskTime")
        {
            vs >> cfg.minTaskTime;
        }
        else if (key == "maxTaskTime")
        {
            vs >> cfg.maxTaskTime;
        }
        else if (key == "requestArrivalProbability")
        {
            vs >> cfg.requestArrivalProbability;
        }
        else if (key == "adjustCooldownCycles")
        {
            vs >> cfg.adjustCooldownCycles;
        }
        else if (key == "blockedIpPrefix")
        {
            std::string prefix;
            if (vs >> prefix)
            {
                cfg.blockedIpPrefix = prefix;
            }
        }
        else if (key == "maxQueuePerServer")
        {
            vs >> cfg.maxQueuePerServer;
        }
        else if (key == "serverCooldownCycles")
        {
            vs >> cfg.serverCooldownCycles;
        }
    }

    outConfig = cfg;
    return true;
}

