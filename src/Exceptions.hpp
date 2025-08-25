#pragma once

#include <stdexcept>
#include <string>

class SprinklerException : public std::runtime_error {
public:
    explicit SprinklerException(const std::string& message) 
        : std::runtime_error(message) {}
};

class DeviceException : public SprinklerException {
private:
    std::string deviceName;
    
public:
    DeviceException(const std::string& device, const std::string& message) 
        : SprinklerException("Device '" + device + "': " + message), deviceName(device) {}
    
    const std::string& getDeviceName() const { return deviceName; }
};

class WeatherException : public SprinklerException {
private:
    std::string location;
    
public:
    WeatherException(const std::string& loc, const std::string& message) 
        : SprinklerException("Weather service for '" + loc + "': " + message), location(loc) {}
    
    const std::string& getLocation() const { return location; }
};

class NetworkException : public SprinklerException {
private:
    int errorCode;
    
public:
    NetworkException(int code, const std::string& message) 
        : SprinklerException("Network error (" + std::to_string(code) + "): " + message), errorCode(code) {}
    
    int getErrorCode() const { return errorCode; }
};

class ConfigurationException : public SprinklerException {
public:
    explicit ConfigurationException(const std::string& message) 
        : SprinklerException("Configuration error: " + message) {}
};