#pragma once

#include "Connection.hpp"
#include "Exceptions.hpp"
#include <chrono>
#include <thread>
#include <functional>

class DeviceManager {
private:
    Connection connection;
    std::string deviceName;
    
    template<typename T>
    T executeWithRetry(const std::string& operation, std::function<T()> func, int maxRetries = 3) {
        for (int attempt = 1; attempt <= maxRetries; ++attempt) {
            try {
                return func();
            } catch (const std::exception& e) {
                if (attempt == maxRetries) {
                    throw DeviceException(deviceName, operation + " failed after " + 
                                        std::to_string(maxRetries) + " attempts: " + e.what());
                }
                
                // Exponential backoff: 1s, 2s, 4s...
                auto delay = std::chrono::seconds(1 << (attempt - 1));
                std::this_thread::sleep_for(delay);
            }
        }
        throw DeviceException(deviceName, operation + " failed unexpectedly");
    }
    
public:
    DeviceManager(const std::string& device) : deviceName(device) {}
    
    void discover() {
        executeWithRetry<void>("Device discovery", [this]() {
            connection.discover();
            return;
        });
    }
    
    bool waitForDevice(int maxWaitSeconds = 20) {
        for (int i = 0; i < maxWaitSeconds; i++) {
            if (connection.found(deviceName)) {
                return true;
            }
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
        throw DeviceException(deviceName, "Device not found after " + std::to_string(maxWaitSeconds) + " seconds");
    }
    
    bool getState() {
        return executeWithRetry<bool>("Get device state", [this]() {
            return connection.get(deviceName);
        });
    }
    
    void setState(bool on) {
        executeWithRetry<void>("Set device state", [this, on]() {
            connection.set(deviceName, on);
            return;
        });
    }
    
    void ensureState(bool desiredState) {
        int attempts = 0;
        const int maxAttempts = 5;
        
        while (attempts < maxAttempts) {
            try {
                bool currentState = getState();
                if (currentState == desiredState) {
                    return; // Success
                }
                
                setState(desiredState);
                std::this_thread::sleep_for(std::chrono::seconds(10));
                
                currentState = getState();
                if (currentState == desiredState) {
                    return; // Success
                }
                
                attempts++;
            } catch (const DeviceException& e) {
                attempts++;
                if (attempts >= maxAttempts) {
                    throw DeviceException(deviceName, "Failed to ensure state after " + 
                                        std::to_string(maxAttempts) + " attempts: " + e.what());
                }
                std::this_thread::sleep_for(std::chrono::seconds(5));
            }
        }
        
        throw DeviceException(deviceName, "Could not achieve desired state: " + 
                            std::string(desiredState ? "ON" : "OFF"));
    }
    
    std::map<std::string, Device> listDevices() {
        return executeWithRetry<std::map<std::string, Device>>("List devices", [this]() {
            return connection.list();
        });
    }
};