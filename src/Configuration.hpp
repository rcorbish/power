#pragma once

#include "Exceptions.hpp"
#include <string>
#include <map>
#include <optional>
#include <fstream>
#include <sstream>
#include <cstdlib>

class Configuration {
private:
    std::map<std::string, std::string> settings;
    
    std::string trim(const std::string& str) {
        size_t first = str.find_first_not_of(' ');
        if (first == std::string::npos) return "";
        size_t last = str.find_last_not_of(' ');
        return str.substr(first, (last - first + 1));
    }
    
    void loadFromFile(const std::string& filename) {
        std::ifstream file(filename);
        if (!file.is_open()) {
            throw ConfigurationException("Cannot open config file: " + filename);
        }
        
        std::string line;
        int lineNum = 0;
        
        while (std::getline(file, line)) {
            lineNum++;
            line = trim(line);
            
            // Skip empty lines and comments
            if (line.empty() || line[0] == '#') continue;
            
            size_t equals = line.find('=');
            if (equals == std::string::npos) {
                throw ConfigurationException("Invalid config line " + std::to_string(lineNum) + 
                                           " in " + filename + ": " + line);
            }
            
            std::string key = trim(line.substr(0, equals));
            std::string value = trim(line.substr(equals + 1));
            
            if (key.empty()) {
                throw ConfigurationException("Empty key in config line " + std::to_string(lineNum) + 
                                           " in " + filename);
            }
            
            settings[key] = value;
        }
    }
    
    void loadFromEnvironment() {
        const char* envVars[] = {
            "SPRINKLER_CERT_FILE",
            "SPRINKLER_KEY_FILE", 
            "SPRINKLER_HISTORY_FILE",
            "SPRINKLER_WEB_PORT",
            "SPRINKLER_SECURITY_PORT",
            "SPRINKLER_BUFFER_SIZE",
            "SPRINKLER_HISTORY_LENGTH",
            "SPRINKLER_MAX_RETRIES",
            "SPRINKLER_MAX_DEVICE_ATTEMPTS",
            "SPRINKLER_DEVICE_DISCOVERY_TIMEOUT",
            "SPRINKLER_WEATHER_CACHE_MINUTES",
            "SPRINKLER_FORECAST_WEIGHT",
            nullptr
        };
        
        for (int i = 0; envVars[i] != nullptr; i++) {
            const char* value = getenv(envVars[i]);
            if (value != nullptr) {
                // Convert environment variable name to config key
                std::string key = envVars[i];
                if (key.substr(0, 10) == "SPRINKLER_") {
                    key = key.substr(10); // Remove SPRINKLER_ prefix
                    std::transform(key.begin(), key.end(), key.begin(), ::tolower);
                    // Convert SNAKE_CASE to camelCase
                    size_t pos = 0;
                    while ((pos = key.find('_', pos)) != std::string::npos) {
                        key.erase(pos, 1);
                        if (pos < key.length()) {
                            key[pos] = std::toupper(key[pos]);
                        }
                    }
                    settings[key] = value;
                }
            }
        }
    }
    
public:
    Configuration() {
        // Set system defaults (not user argument defaults)
        settings["webPort"] = "8111";
        settings["securityPort"] = "9111";
        settings["bufferSize"] = "1024";
        settings["historyLength"] = "15";
        settings["maxRetries"] = "3";
        settings["maxDeviceAttempts"] = "5";
        settings["deviceDiscoveryTimeout"] = "20";
        settings["weatherCacheMinutes"] = "30";
        settings["forecastWeight"] = "0.5";
        settings["certFile"] = "cert.pem";
        settings["keyFile"] = "key.pem";
        settings["historyFile"] = "sprinkler.dat";
        settings["logLevel"] = "INFO";
        settings["logFile"] = "sprinkler.log";
        settings["enableTimestamp"] = "true";
        settings["enableRotation"] = "true";
    }
    
    void load(const std::string& configFile = "") {
        // 1. Load defaults (already done in constructor)
        // 2. Load from config file if specified
        if (!configFile.empty()) {
            loadFromFile(configFile);
        }
        // 3. Override with environment variables
        loadFromEnvironment();
    }
    
    std::string getString(const std::string& key, const std::string& defaultValue = "") const {
        auto it = settings.find(key);
        return (it != settings.end()) ? it->second : defaultValue;
    }
    
    int getInt(const std::string& key, int defaultValue = 0) const {
        auto it = settings.find(key);
        if (it == settings.end()) return defaultValue;
        
        try {
            return std::stoi(it->second);
        } catch (const std::invalid_argument&) {
            throw ConfigurationException("Invalid integer value for '" + key + "': " + it->second);
        } catch (const std::out_of_range&) {
            throw ConfigurationException("Integer value out of range for '" + key + "': " + it->second);
        }
    }
    
    double getDouble(const std::string& key, double defaultValue = 0.0) const {
        auto it = settings.find(key);
        if (it == settings.end()) return defaultValue;
        
        try {
            return std::stod(it->second);
        } catch (const std::invalid_argument&) {
            throw ConfigurationException("Invalid double value for '" + key + "': " + it->second);
        } catch (const std::out_of_range&) {
            throw ConfigurationException("Double value out of range for '" + key + "': " + it->second);
        }
    }
    
    bool getBool(const std::string& key, bool defaultValue = false) const {
        auto it = settings.find(key);
        if (it == settings.end()) return defaultValue;
        
        std::string value = it->second;
        std::transform(value.begin(), value.end(), value.begin(), ::tolower);
        
        if (value == "true" || value == "1" || value == "yes" || value == "on") {
            return true;
        } else if (value == "false" || value == "0" || value == "no" || value == "off") {
            return false;
        } else {
            throw ConfigurationException("Invalid boolean value for '" + key + "': " + it->second);
        }
    }
    
    void set(const std::string& key, const std::string& value) {
        settings[key] = value;
    }
    
    bool hasKey(const std::string& key) const {
        return settings.find(key) != settings.end();
    }
    
    void validate() const {
        // Validate system settings ranges
        int webPort = getInt("webPort");
        if (webPort < 1024 || webPort > 65535) {
            throw ConfigurationException("webPort must be between 1024 and 65535");
        }
        
        int securityPort = getInt("securityPort");
        if (securityPort < 1024 || securityPort > 65535) {
            throw ConfigurationException("securityPort must be between 1024 and 65535");
        }
        
        int maxRetries = getInt("maxRetries");
        if (maxRetries < 1 || maxRetries > 10) {
            throw ConfigurationException("maxRetries must be between 1 and 10");
        }
        
        int deviceTimeout = getInt("deviceDiscoveryTimeout");
        if (deviceTimeout < 5 || deviceTimeout > 120) {
            throw ConfigurationException("deviceDiscoveryTimeout must be between 5 and 120 seconds");
        }
        
        double forecastWeight = getDouble("forecastWeight");
        if (forecastWeight < 0.0 || forecastWeight > 1.0) {
            throw ConfigurationException("forecastWeight must be between 0.0 and 1.0");
        }
    }
    
    void printSettings() const {
        std::cout << "System configuration:" << std::endl;
        for (const auto& [key, value] : settings) {
            std::cout << "  " << key << " = " << value << std::endl;
        }
    }
};