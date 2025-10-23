
#include <string.h>
#include <iostream>
#include <math.h>

#include <unistd.h>
#include <stdio.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h> 
#include <getopt.h>
#include <chrono>
#include <thread>

#include "Connection.hpp"
#include "Weather.hpp"
#include "History.hpp"
#include "Options.hpp"
#include "Logger.hpp"
#include "LoggerConfig.hpp"
#include "Configuration.hpp"

using namespace std;

int main(int argc, char **argv) {
    bool turnDeviceOn = false;

    try {
        Args args = parseOptions( argc, argv );
        
        // Load configuration
        Configuration config;
        config.load("sprinkler.conf");
        config.validate();
        
        // Initialize logging from config
        initLoggerFromConfig(config, args.verbose);
        
        if (args.test) {
            LOG_INFO("Running in test mode - no actual device changes will be made");
        }
        
        LOG_DEBUG("Starting sprinkler application with device: {}", args.device);
        
        Weather weather(args.zip, args.previousHoursToLookForRain, args.forecastHours);
        weather.init();
        Connection con; 

        if( args.state == ON ) {
            turnDeviceOn = true ;
            LOG_INFO("Manual override: turning device ON");
        } else if( args.state == OFF ) {
            turnDeviceOn = false ;
            LOG_INFO("Manual override: turning device OFF");
        } else {
            LOG_DEBUG("Reading weather data for zip code: {}", args.zip);
            weather.read() ;
            double totalRain = weather.getRecentRainfall() ;
            double forecastRain = weather.getForecastRainfall() ;

            LOG_INFO("Weather for {}: received {}mm rain, forecast {}mm rain", 
                    args.zip, totalRain, forecastRain);
            
            turnDeviceOn = (totalRain+forecastRain*.5) < args.desiredMMRain ;
            LOG_INFO("Decision: {} (effective rain: {}mm vs needed: {}mm)", 
                    turnDeviceOn ? "WATER" : "SKIP", 
                    totalRain+forecastRain*.5, 
                    args.desiredMMRain);
            
            HistoryEntry he( totalRain, forecastRain, turnDeviceOn ? args.minutesToSprinkle : 0 ) ;
            if( !args.test ) {
                appendHistory( he ) ;
                LOG_DEBUG("History entry saved");
            }
        }

        LOG_DEBUG("Starting device discovery");
        con.startDiscovery() ;
        if( args.list ) {
            LOG_INFO("Discovering devices...");
            this_thread::sleep_for(chrono::seconds(12));
            con.stopDiscovery ();   
            LOG_INFO("Found {} devices", con.list().size());
            for( const auto &deviceName : con.list() ) {
                LOG_INFO("  {}", deviceName);
            }
            return 0;  // just listing print & bail out
        } else { 
            LOG_DEBUG("Looking for device: {}", args.device);
            for( int i=0 ; i<20 ; i++ ) {
                if( con.found( args.device ) ) break ;
                this_thread::sleep_for(chrono::seconds(1));
            }
            if( !con.found( args.device ) ) {
                LOG_ERROR("Device {} not found after discovery period. Game over.", args.device);
                con.stopDiscovery ();   
                return -1;
            }
        }
        // con.stopDiscovery() ;

        bool on = con.get( args.device );
        if( on == turnDeviceOn ) {
            LOG_INFO("Device is already {}", on ? "ON" : "OFF");
        } else {
            LOG_INFO("Need to turn device {}", turnDeviceOn ? "ON" : "OFF");
        }

        if( !args.test ) {
            if( turnDeviceOn ) {
                LOG_INFO("Turning device ON and waiting for confirmation");
                while( !on ) {
                    con.set( args.device, true ) ;
                    this_thread::sleep_for(chrono::seconds(10));
                    on = con.get( args.device ) ;
                    if(!on) {
                        LOG_WARN("Device is not ON yet, retrying...");
                    }
                } 

                LOG_INFO("Confirmed ON. Sprinkling for {} minutes", args.minutesToSprinkle);
                this_thread::sleep_for(chrono::minutes(args.minutesToSprinkle));
            }

            while(on) {
                LOG_INFO("Ensuring device is turned OFF");
                con.set( args.device, false ) ;
                this_thread::sleep_for(chrono::seconds(10));
                on = con.get( args.device ) ;
                if(on) {
                    LOG_WARN("Device is still ON, retrying...");
                }
            } 
        }
        
        LOG_INFO("Sprinkler program completed successfully");
        shutdownLogger();
        
    } catch( const ConfigurationException& e ) {
        if (g_logger) {
            LOG_ERROR("Configuration error: {}", e.what());
            shutdownLogger();
        } else {
            cerr << "Configuration error: " << e.what() << endl;
        }
        return -1;
    } catch( const std::exception& e ) {
        if (g_logger) {
            LOG_FATAL("Fatal error: {}", e.what());
            shutdownLogger();
        } else {
            cerr << "Fatal error: " << e.what() << endl;
        }
        return -2;
    }    
    return 0 ;
}
