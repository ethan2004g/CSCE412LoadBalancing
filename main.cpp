/**
 * @file main.cpp
 * @brief Entry point for the CSCE 412 Project 3 load balancer simulation.
 *
 * This program simulates a load balancer managing a pool of web servers that
 * process incoming requests. It demonstrates dynamic scaling of servers based
 * on queue length, generation of random requests, and basic logging suitable
 * for the project requirements. Optional bonus mode runs a Switch that routes
 * jobs by type (Streaming vs Processing) to two dedicated load balancers.
 */

#include <iostream>
#include <fstream>
#include <string>
#include <limits>
#include <filesystem>
#include <ctime>
#include <sstream>
#include <iomanip>

#include "LoadBalancer.h"
#include "Switch.h"
#include "Request.h"

/**
 * @brief Simple helper to read an integer from std::cin with a default value.
 *
 * @param prompt Text to show the user.
 * @param defaultValue Value to use if the user enters nothing or invalid input.
 * @return int The parsed or default value.
 */
int readIntWithDefault(const std::string &prompt, int defaultValue)
{
    std::cout << prompt << " [" << defaultValue << "]: ";
    int value = defaultValue;
    if (!(std::cin >> value))
    {
        std::cin.clear();
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        value = defaultValue;
    }
    return value;
}

int main()
{
    std::cout << "=== CSCE 412 Project 3: Load Balancer Simulation ===\n";

    const int defaultServers = 10;
    const int defaultCycles = 10000;

    int initialServers = readIntWithDefault("Enter initial number of web servers", defaultServers);
    int totalCycles = readIntWithDefault("Enter number of clock cycles to simulate", defaultCycles);

    bool useBonusSwitch = false;
    std::cout << "Run in BONUS mode (Switch: route Streaming vs Processing to separate LBs)? (y/n) [n]: ";
    char bonusChoice = 'n';
    if (std::cin >> bonusChoice && (bonusChoice == 'y' || bonusChoice == 'Y'))
    {
        useBonusSwitch = true;
        std::cout << "Bonus mode enabled: two load balancers (Streaming + Processing) with separate statistics.\n";
    }
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

    // Simulation configuration defaults
    SimulationConfig config;
    config.minTaskTime = 5;
    config.maxTaskTime = 100;
    config.requestArrivalProbability = 0.4; // Probability of a new request per cycle
    config.adjustCooldownCycles = 50;       // Wait this many cycles between scaling actions

    // Allow optional loading of configuration from a file.
    std::cout << "Load additional configuration from file config.txt? (y/n) [n]: ";
    char useConfig = 'n';
    if (std::cin >> useConfig && (useConfig == 'y' || useConfig == 'Y'))
    {
        SimulationConfig fromFile;
        if (loadConfigFromFile("config.txt", fromFile))
        {
            config = fromFile;
            std::cout << "Configuration loaded from config.txt.\n";
        }
        else
        {
            std::cout << "Could not load config.txt, using built-in defaults.\n";
        }
    }
    setBlockedIpPrefix(config.blockedIpPrefix);

    // Ensure logs directory exists, then open a timestamped log file
    std::filesystem::create_directories("logs");
    std::time_t now = std::time(nullptr);
    std::tm localTime{};
#ifdef _WIN32
    localtime_s(&localTime, &now);
#else
    localtime_r(&now, &localTime);
#endif
    std::ostringstream logFileName;
    logFileName << "logs/load_balancer_log_"
                << std::put_time(&localTime, "%Y-%m-%d_%H-%M-%S")
                << ".txt";
    std::string logFilePath = logFileName.str();
    std::ofstream logFile(logFilePath);
    bool useFileLog = logFile.is_open();

    std::ostream &logStream = useFileLog ? static_cast<std::ostream &>(logFile) : static_cast<std::ostream &>(std::cout);

    logStream << "CSCE 412 Project 3 - Load Balancer Log\n";
    logStream << "Mode: " << (useBonusSwitch ? "Bonus (Switch: Streaming + Processing LBs)" : "Single Load Balancer") << "\n";
    logStream << "Initial servers (per pool in Switch mode): " << initialServers << "\n";
    logStream << "Total cycles: " << totalCycles << "\n";
    logStream << "Task time range: [" << config.minTaskTime << ", " << config.maxTaskTime << "]\n";
    logStream << "Request arrival probability: " << config.requestArrivalProbability << "\n";
    logStream << "Adjust cooldown cycles: " << config.adjustCooldownCycles << "\n\n";

    if (useBonusSwitch)
    {
        Switch sw(initialServers, totalCycles, config);
        sw.populateInitialQueue();
        logStream << "Starting total queue size (both pools): " << sw.getTotalQueueSize() << "\n\n";
        std::cout << "Running simulation (Switch mode)... this may take a moment.\n";
        sw.runSimulation(logStream);
        sw.printSummary(logStream);
        sw.printRequestLog(logStream);
        if (useFileLog)
        {
            sw.printSummary(std::cout);
        }
    }
    else
    {
        LoadBalancer balancer(initialServers, totalCycles, config);
        balancer.populateInitialQueue();
        logStream << "Starting queue size: " << balancer.getQueueSize() << "\n\n";
        std::cout << "Running simulation... this may take a moment.\n";
        balancer.runSimulation(logStream);
        balancer.printSummary(logStream);
        balancer.printRequestLog(logStream);
        if (useFileLog)
        {
            balancer.printSummary(std::cout);
        }
    }

    if (useFileLog)
    {
        std::cout << "Log written to " << logFilePath << "\n";
    }
    else
    {
        std::cout << "Could not open log file, log output was printed to console.\n";
    }

    std::cout << "Simulation complete.\n";
    return 0;
}

