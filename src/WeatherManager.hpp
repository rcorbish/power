#pragma once

#include "Weather.hpp"
#include "Exceptions.hpp"
#include "Options.hpp"
#include <chrono>
#include <thread>
#include <optional>
#include <iostream>

struct WeatherData {
    double recentRainfall = 0.0;
    double forecastRainfall = 0.0;
    std::string description = "Unknown";
    std::chrono::system_clock::time_point timestamp;
    bool isValid = false;
};

class WeatherManager {
private:
    Weather weather;
    std::string location;
    WeatherData cachedData;
    std::chrono::minutes cacheValidityPeriod;
    
    bool isCacheValid() const {
        if (!cachedData.isValid) return false;
        
        auto now = std::chrono::system_clock::now();
        auto age = std::chrono::duration_cast<std::chrono::minutes>(now - cachedData.timestamp);
        return age < cacheValidityPeriod;
    }
    
    WeatherData fetchFreshData() {
        try {
            weather.read();
            
            WeatherData data;
            data.recentRainfall = weather.getRecentRainfall();
            data.forecastRainfall = weather.getForecastRainfall();
            data.description = weather.getDescription();
            data.timestamp = std::chrono::system_clock::now();
            data.isValid = true;
            
            return data;
        } catch (const std::exception& e) {
            throw WeatherException(location, "Failed to fetch weather data: " + std::string(e.what()));
        }
    }
    
public:
    WeatherManager(const std::string& zip, int pastHours, int forecastHours, 
                   std::chrono::minutes cacheValidity = std::chrono::minutes(30)) 
        : weather(zip, pastHours, forecastHours), 
          location(zip),
          cacheValidityPeriod(cacheValidity) {
        
        try {
            weather.init();
        } catch (const std::exception& e) {
            throw WeatherException(location, "Failed to initialize weather service: " + std::string(e.what()));
        }
    }
    
    WeatherData getCurrentWeather(bool allowCached = true) {
        // Return cached data if valid and allowed
        if (allowCached && isCacheValid()) {
            return cachedData;
        }
        
        try {
            cachedData = fetchFreshData();
            return cachedData;
        } catch (const WeatherException& e) {
            // If we have cached data, use it as fallback even if expired
            if (cachedData.isValid) {
                std::cerr << getTime() << "Weather service failed, using cached data: " << e.what() << std::endl;
                return cachedData; // Return stale but valid data
            }
            throw; // Re-throw if no fallback available
        }
    }
    
    std::optional<WeatherData> getCurrentWeatherSafe() {
        try {
            return getCurrentWeather();
        } catch (const WeatherException& e) {
            std::cerr << getTime() << "Weather service unavailable: " << e.what() << std::endl;
            return std::nullopt;
        }
    }
    
    bool shouldWater(double desiredMMRain, double forecastWeight = 0.5) {
        auto weatherOpt = getCurrentWeatherSafe();
        
        if (!weatherOpt.has_value()) {
            std::cerr << getTime() << "No weather data available - defaulting to NOT watering for safety" << std::endl;
            return false;
        }
        
        const auto& data = weatherOpt.value();
        double effectiveRain = data.recentRainfall + (data.forecastRainfall * forecastWeight);
        
        return effectiveRain < desiredMMRain;
    }
    
    void invalidateCache() {
        cachedData.isValid = false;
    }
};