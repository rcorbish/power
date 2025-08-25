
## Project Overview

This project manages garden sprinklers. It checks the weather (history and forecast) to determine whether to water the garden. This app is started (via cron) daily. There is a webserver to show the current status and history of activity and weather.

## Coding Guidelines

- use simple C++ syntax
- reduce the number of external libraries
- use cmake for building
- keep activity history in a simple text file

## Executables

- webserver : C++ (mongoose) webserver to display web pages
- readweather : simple app to display current weather forecast
- sprinklers : the application which is started every day to check the weather and turn on the sprinklers (or skip). 


