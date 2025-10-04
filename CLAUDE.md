# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

Smart garden sprinkler system that manages ECO Plugs based on weather conditions. The system checks weather history and forecasts to automatically decide whether to water the garden. Can be run manually or via cron for daily automation.

## Build System

**Dependencies:**
- libssl-dev
- libcurl4-openssl-dev
- jsoncpp
- CMake 3.17+
- C++20 compiler

**Build commands:**
```bash
cmake .
make
```

**Build output location:** Executables are placed in project root directory (not in build/)

## Executables

- **sprinklers**: Main application (runs daily via cron). Checks weather and controls sprinklers via ECO Plug
- **webserver**: Mongoose-based HTTPS server displaying status, history, and weather
- **readweather**: Simple utility to display current weather forecast

## Architecture

### Device Control Layer
- **Connection** (Connection.hpp/cpp): UDP broadcast discovery for ECO Plugs. Manages global device discovery and maintains device registry. Uses separate thread for UDP receiver.
- **Device** (Device.hpp/cpp): Individual device control. Each device has its own TCP connection for get/set operations.
- **MSG408 protocol**: Binary struct for ECO Plug communication containing device metadata (ID, name, MAC, IP, credentials).

### Weather Layer
- **Weather** (Weather.hpp/cpp): Fetches weather data via libcurl from external API. Parses location, rainfall history, and forecast using jsoncpp.
- **History** (History.hpp/cpp): Simple text file storage for sprinkler activity history (timestamp, rainfall, forecast, duration).

### Configuration System
Three-tier configuration with priority: defaults → config file → environment variables

- **Configuration** (Configuration.hpp): Handles system settings (ports, paths, retry counts, timeouts)
- **Options** (Options.hpp/cpp): Command-line argument parsing for runtime parameters
- **sprinkler.conf**: Key-value config file (# for comments)

Environment variables override file settings using pattern: `SPRINKLER_<KEY_NAME>` (e.g., `SPRINKLER_WEB_PORT`)

### Webserver
- Uses Mongoose library (fetched via CMake FetchContent)
- HTTPS with OpenSSL/TLS support
- REST endpoints:
  - `/home` - Main UI
  - `/history` - JSON history data
  - `/weather` - Current weather display
  - `/state` - Device state
  - `/security` - Security info (connects to local security port)
  - `/security-data` - Security JSON data

### Logging
- **Logger** (Logger.hpp/cpp): Centralized logging with levels (DEBUG, INFO, WARN, ERROR, FATAL)
- **LoggerConfig** (LoggerConfig.hpp): Integrates Configuration with Logger
- Log file rotation and timestamp control via config

## Key Logic

**Decision algorithm in main.cpp:**
```
turnDeviceOn = (totalRain + forecastRain * 0.5) < desiredMMRain
```

Where forecastRain weight (0.5) is configurable via `forecastWeight` setting.

**Device operation pattern:**
1. Start UDP discovery broadcast
2. Wait for device response (max 20 seconds, polls every 1 second)
3. Stop discovery
4. Read current device state
5. If state change needed, retry until confirmed (10 second intervals)
6. If watering, wait for duration then ensure OFF

## Common Development Tasks

**Run single test:**
```bash
./tst/test
```

**Generate self-signed certificates:**
```bash
openssl req -nodes -new -x509 -subj "/C=/ST=/L=/O=/CN=mars" -keyout key.pem -out cert.pem
```

**Run webserver:**
```bash
./webserver log.txt cert.pem key.pem
```

**Run sprinklers (test mode):**
```bash
./sprinklers --test --device "Sprinklers" --zip 94024
```

## Coding Guidelines

- Simple C++ syntax (C++20)
- Minimize external library dependencies
- CMake for building
- History stored in simple text file format
- Error handling via exceptions (ConfigurationException, std::exception, legacy string throws)
