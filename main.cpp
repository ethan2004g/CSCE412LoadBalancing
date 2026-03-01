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

// ---------------------------------------------------------------------------
// HTML log generation helpers
// ---------------------------------------------------------------------------

static std::string htmlEscape(const std::string &s)
{
    std::string out;
    out.reserve(s.size());
    for (char c : s)
    {
        if      (c == '&') out += "&amp;";
        else if (c == '<') out += "&lt;";
        else if (c == '>') out += "&gt;";
        else               out += c;
    }
    return out;
}

static bool strHas(const std::string &s, const std::string &sub)
{
    return s.find(sub) != std::string::npos;
}

static bool strStartsWith(const std::string &s, const std::string &prefix)
{
    return s.rfind(prefix, 0) == 0;
}

/**
 * @brief Colorize the "Server states:" line inline, giving each server token
 *        its own color (ACTIVE = green, INACTIVE(cd) = orange, INACTIVE = gray).
 */
static std::string colorizeServerStates(const std::string &escaped)
{
    std::string out;
    const std::string prefix = "  Server states:";
    out += "<span class=\"srv-label\">" + prefix + "</span>";
    size_t pos = prefix.size();

    while (pos < escaped.size())
    {
        size_t open = escaped.find('[', pos);
        if (open == std::string::npos)
        {
            out += "<span class=\"inactive\">" + escaped.substr(pos) + "</span>";
            break;
        }
        out += "<span class=\"inactive\">" + escaped.substr(pos, open - pos) + "</span>";

        size_t close = escaped.find(']', open);
        if (close == std::string::npos)
        {
            out += escaped.substr(open);
            break;
        }

        std::string token = escaped.substr(open, close - open + 1);
        std::string cls;
        if (strHas(token, ":ACTIVE"))           cls = "srv-active";
        else if (strHas(token, "cd:"))          cls = "srv-cooling";
        else                                    cls = "inactive";

        out += "<span class=\"" + cls + "\">" + token + "</span>";
        pos = close + 1;
    }
    return out;
}

/**
 * @brief Read the plain-text log at srcPath and write a colored HTML version
 *        to dstPath.  Opens in any browser with a dark VS-Code-style theme.
 *
 * @param srcPath Path to the source .txt log file.
 * @param dstPath Path for the generated .html file.
 */
static void generateHtmlLog(const std::string &srcPath, const std::string &dstPath)
{
    std::ifstream in(srcPath);
    if (!in.is_open()) return;
    std::ofstream out(dstPath);
    if (!out.is_open()) return;

    out << R"(<!DOCTYPE html>
<html>
<head>
  <meta charset="UTF-8">
  <title>Load Balancer Log</title>
  <style>
    body        { background:#1e1e1e; color:#d4d4d4; font-family:'Consolas','Courier New',monospace; font-size:13px; padding:24px; line-height:1.6; }
    pre         { margin:0; white-space:pre-wrap; word-break:break-all; }
    .log-hdr    { color:#9cdcfe; }
    .scale-up   { color:#4ec94e; font-weight:bold; }
    .scale-down { color:#ff8c00; font-weight:bold; }
    .req-start  { color:#569cd6; }
    .req-end    { color:#4ec9b0; }
    .overload   { color:#f44747; }
    .underload  { color:#dcdcaa; }
    .in-range   { color:#4ec94e; }
    .summary    { color:#c586c0; font-weight:bold; }
    .req-hdr    { color:#c586c0; }
    .srv-label  { color:#808080; }
    .srv-active { color:#4ec94e; }
    .srv-cooling{ color:#ff8c00; }
    .inactive   { color:#606060; }
  </style>
</head>
<body><pre>
)";

    std::string line;
    while (std::getline(in, line))
    {
        std::string esc = htmlEscape(line);

        if (strStartsWith(line, "  Server states:"))
        {
            out << colorizeServerStates(esc) << "\n";
            continue;
        }

        std::string cls;
        if      (strHas(line, "SCALE UP"))                                  cls = "scale-up";
        else if (strHas(line, "SCALE DOWN"))                                cls = "scale-down";
        else if (strHas(line, "REQUEST START"))                             cls = "req-start";
        else if (strHas(line, "REQUEST END"))                               cls = "req-end";
        else if (strStartsWith(line, "Cycle") && strHas(line, "OVERLOADED"))  cls = "overload";
        else if (strStartsWith(line, "Cycle") && strHas(line, "UNDERLOADED")) cls = "underload";
        else if (strStartsWith(line, "Cycle") && strHas(line, "IN RANGE"))    cls = "in-range";
        else if (strHas(line, "===") || strHas(line, "---"))                cls = "summary";
        else if (strHas(line, "Per-Request Event Log") ||
                 strHas(line, "Format:") ||
                 strStartsWith(line, "----"))                               cls = "req-hdr";
        else if (strStartsWith(line, "CSCE 412")  ||
                 strStartsWith(line, "Mode:")      ||
                 strStartsWith(line, "Initial")    ||
                 strStartsWith(line, "Total cycle")||
                 strStartsWith(line, "Task time")  ||
                 strStartsWith(line, "Request arr")||
                 strStartsWith(line, "Adjust cool")||
                 strStartsWith(line, "Server cool")||
                 strStartsWith(line, "Starting"))                           cls = "log-hdr";

        if (!cls.empty())
            out << "<span class=\"" << cls << "\">" << esc << "</span>\n";
        else
            out << esc << "\n";
    }

    out << "</pre></body></html>\n";
}

// ---------------------------------------------------------------------------

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
    logStream << "Adjust cooldown cycles: " << config.adjustCooldownCycles << "\n";
    logStream << "Server cooldown after request: " << config.serverCooldownCycles << " cycles\n\n";

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
        logFile.close();
        std::string htmlPath = logFilePath.substr(0, logFilePath.size() - 4) + ".html";
        generateHtmlLog(logFilePath, htmlPath);
        std::cout << "Log written to      " << logFilePath << "\n";
        std::cout << "Colored HTML log:   " << htmlPath << "\n";
    }
    else
    {
        std::cout << "Could not open log file, log output was printed to console.\n";
    }

    std::cout << "Simulation complete.\n";
    return 0;
}

