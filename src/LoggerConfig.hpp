#pragma once

#include "Logger.hpp"
#include "Configuration.hpp"
#include <string>

inline LogLevel stringToLogLevel(const std::string& level) {
    std::string upperLevel = level;
    std::transform(upperLevel.begin(), upperLevel.end(), upperLevel.begin(), ::toupper);
    
    if (upperLevel == "DEBUG") return LogLevel::DEBUG;
    if (upperLevel == "INFO") return LogLevel::INFO;
    if (upperLevel == "WARN") return LogLevel::WARN;
    if (upperLevel == "ERROR") return LogLevel::ERROR;
    if (upperLevel == "FATAL") return LogLevel::FATAL;
    
    // Default to INFO if unknown
    return LogLevel::INFO;
}

inline void initLoggerFromConfig(const Configuration& config, bool verbose = false) {
    std::string logFile = config.getString("logFile", "sprinkler.log");
    LogLevel level = stringToLogLevel(config.getString("logLevel", "INFO"));
    
    // Override with verbose flag if provided
    if (verbose) {
        level = LogLevel::DEBUG;
    }
    
    bool enableTimestamp = config.getBool("enableTimestamp", true);
    bool enableRotation = config.getBool("enableRotation", true);
    
    initLogger(logFile, level, enableTimestamp, enableRotation);
}