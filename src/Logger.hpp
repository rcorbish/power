#pragma once

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <memory>
#include <mutex>
#include <ctime>

enum class LogLevel {
    DEBUG = 0,
    INFO = 1,
    WARN = 2,
    ERROR = 3,
    FATAL = 4
};

class Logger {
private:
    LogLevel currentLevel;
    std::unique_ptr<std::ofstream> fileStream;
    std::ostream* logStream;
    std::mutex logMutex;
    bool timestampEnabled;
    std::string baseFilename;
    std::string currentDate;
    bool useFileRotation;
    
    std::string getLevelString(LogLevel level) const {
        switch (level) {
            case LogLevel::DEBUG: return "DEBUG";
            case LogLevel::INFO:  return "INFO ";
            case LogLevel::WARN:  return "WARN ";
            case LogLevel::ERROR: return "ERROR";
            case LogLevel::FATAL: return "FATAL";
            default: return "UNKNOWN";
        }
    }
    
    std::string getTimestamp() const {
        if (!timestampEnabled) return "";
        
        time_t rawtime;
        struct tm * timeinfo;
        char buffer[128];
        
        time(&rawtime);
        timeinfo = localtime(&rawtime);
        strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", timeinfo);
        
        return std::string(buffer) + " ";
    }
    
    std::string getCurrentDate() const {
        time_t rawtime;
        struct tm * timeinfo;
        char buffer[32];
        
        time(&rawtime);
        timeinfo = localtime(&rawtime);
        strftime(buffer, sizeof(buffer), "%Y%m%d", timeinfo);
        
        return std::string(buffer);
    }
    
    void rotateLogIfNeeded() {
        if (!useFileRotation) return;
        
        std::string today = getCurrentDate();
        if (today != currentDate) {
            currentDate = today;
            
            // Create new filename with date suffix
            std::string rotatedFilename = baseFilename + "." + currentDate;
            
            // Close current file and open new one
            fileStream.reset();
            fileStream = std::make_unique<std::ofstream>(rotatedFilename, std::ios::app);
            
            if (!fileStream->is_open()) {
                throw std::runtime_error("Cannot open rotated log file: " + rotatedFilename);
            }
            
            logStream = fileStream.get();
            
            // Log rotation message
            *logStream << getTimestamp() << "[INFO ] Log rotated to " << rotatedFilename << std::endl;
            logStream->flush();
        }
    }
    
    void writeLog(LogLevel level, const std::string& message) {
        if (level < currentLevel) return;
        
        std::lock_guard<std::mutex> lock(logMutex);
        
        // Check if we need to rotate the log file
        rotateLogIfNeeded();
        
        *logStream << getTimestamp() << "[" << getLevelString(level) << "] " << message << std::endl;
        logStream->flush();
    }
    
public:
    // Console logger constructor
    Logger(LogLevel level = LogLevel::INFO, bool enableTimestamp = true) 
        : currentLevel(level), timestampEnabled(enableTimestamp), 
          logStream(&std::cout), useFileRotation(false) {
    }
    
    // File logger constructor with rotation
    Logger(const std::string& filename, LogLevel level = LogLevel::INFO, bool enableTimestamp = true, bool enableRotation = true)
        : currentLevel(level), timestampEnabled(enableTimestamp), 
          baseFilename(filename), useFileRotation(enableRotation) {
        
        if (useFileRotation) {
            currentDate = getCurrentDate();
            std::string rotatedFilename = baseFilename + "." + currentDate;
            fileStream = std::make_unique<std::ofstream>(rotatedFilename, std::ios::app);
            if (!fileStream->is_open()) {
                throw std::runtime_error("Cannot open log file: " + rotatedFilename);
            }
        } else {
            fileStream = std::make_unique<std::ofstream>(filename, std::ios::app);
            if (!fileStream->is_open()) {
                throw std::runtime_error("Cannot open log file: " + filename);
            }
        }
        
        logStream = fileStream.get();
    }
    
    void setLevel(LogLevel level) {
        std::lock_guard<std::mutex> lock(logMutex);
        currentLevel = level;
    }
    
    LogLevel getLevel() const {
        return currentLevel;
    }
    
    void enableTimestamp(bool enable) {
        std::lock_guard<std::mutex> lock(logMutex);
        timestampEnabled = enable;
    }
    
    template<typename... Args>
    void debug(const std::string& format, Args... args) {
        if (currentLevel <= LogLevel::DEBUG) {
            std::ostringstream oss;
            formatMessage(oss, format, args...);
            writeLog(LogLevel::DEBUG, oss.str());
        }
    }
    
    template<typename... Args>
    void info(const std::string& format, Args... args) {
        if (currentLevel <= LogLevel::INFO) {
            std::ostringstream oss;
            formatMessage(oss, format, args...);
            writeLog(LogLevel::INFO, oss.str());
        }
    }
    
    template<typename... Args>
    void warn(const std::string& format, Args... args) {
        if (currentLevel <= LogLevel::WARN) {
            std::ostringstream oss;
            formatMessage(oss, format, args...);
            writeLog(LogLevel::WARN, oss.str());
        }
    }
    
    template<typename... Args>
    void error(const std::string& format, Args... args) {
        if (currentLevel <= LogLevel::ERROR) {
            std::ostringstream oss;
            formatMessage(oss, format, args...);
            writeLog(LogLevel::ERROR, oss.str());
        }
    }
    
    template<typename... Args>
    void fatal(const std::string& format, Args... args) {
        std::ostringstream oss;
        formatMessage(oss, format, args...);
        writeLog(LogLevel::FATAL, oss.str());
    }
    
private:
    void formatMessage(std::ostringstream& oss, const std::string& format) {
        oss << format;
    }
    
    template<typename T, typename... Args>
    void formatMessage(std::ostringstream& oss, const std::string& format, T value, Args... args) {
        size_t pos = format.find("{}");
        if (pos != std::string::npos) {
            oss << format.substr(0, pos) << value;
            formatMessage(oss, format.substr(pos + 2), args...);
        } else {
            oss << format << " " << value;
            formatMessage(oss, "", args...);
        }
    }
};

// Global logger instance
extern Logger* g_logger;

// Convenience macros
#define LOG_DEBUG(...) if(g_logger) g_logger->debug(__VA_ARGS__)
#define LOG_INFO(...) if(g_logger) g_logger->info(__VA_ARGS__)
#define LOG_WARN(...) if(g_logger) g_logger->warn(__VA_ARGS__)
#define LOG_ERROR(...) if(g_logger) g_logger->error(__VA_ARGS__)
#define LOG_FATAL(...) if(g_logger) g_logger->fatal(__VA_ARGS__)

// Initialize global logger
inline void initLogger(LogLevel level = LogLevel::INFO, bool timestamp = true) {
    if (!g_logger) {
        g_logger = new Logger(level, timestamp);
    }
}

inline void initLogger(const std::string& filename, LogLevel level = LogLevel::INFO, bool timestamp = true, bool rotation = true) {
    if (g_logger) delete g_logger;
    g_logger = new Logger(filename, level, timestamp, rotation);
}

inline void shutdownLogger() {
    delete g_logger;
    g_logger = nullptr;
}