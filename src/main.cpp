
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

using namespace std;

int main(int argc, char **argv) {
    bool turnDeviceOn = false;

    Args args = parseOptions( argc, argv );
    if (args.verbose && args.test) {
        cout << getTime() << "Test mode" << endl;
    }
    Weather weather(args.zip, args.previousHoursToLookForRain, args.forecastHours);
    weather.init();

    try {
        Connection con; 

        if( args.state == ON ) {
            turnDeviceOn = true ;
        } else if( args.state == OFF ) {
            turnDeviceOn = false ;
        } else {
            weather.read() ;
            double totalRain = weather.getRecentRainfall() ;
            double forecastRain = weather.getForecastRainfall() ;

            if( args.verbose ) {
                cout << getTime() << args.zip << " received " << totalRain << "mm and forecasts " << forecastRain << "mm of rain." << endl ;
            }
            turnDeviceOn = (totalRain+forecastRain*.5) < args.desiredMMRain ;
            HistoryEntry he( totalRain, forecastRain, turnDeviceOn ? args.minutesToSprinkle : 0 ) ;
            if( !args.test ) {
                appendHistory( he ) ;
            }
        }

        con.discover() ;
        if( args.list ) {
            this_thread::sleep_for(chrono::seconds(12));
            for( auto entry : con.list() ) {
                cout << entry.first << endl ;
            }
        } else { 
            for( int i=0 ; i<20 ; i++ ) {
                if( con.found( args.device ) ) break ;
                // give some time for response
                this_thread::sleep_for(chrono::seconds(1));
            }
        }

        bool on = con.get( args.device );
        if( args.verbose ) {
            if( on == turnDeviceOn ) {
                cout << getTime() << "Device is already " << (on?"ON":"OFF") << endl ;
            } else {
                cout << getTime() << "Need to turn device " << (turnDeviceOn?"ON":"OFF") << endl ;
            }
        }

        if( !args.test ) {
            if( turnDeviceOn ) {
                while( !on ) {
                    con.set( args.device, true ) ;
                    this_thread::sleep_for(chrono::milliseconds(1500));
                    on = con.get( args.device ) ;
                    if(!on) {
                        cout << getTime() << "Device is not on, will retry" << endl ;
                    }
                } 

                if( args.verbose ) {
                    cout << getTime() << "Sprinkling for " << args.minutesToSprinkle << " minutes." << endl ;
                }
                this_thread::sleep_for(chrono::minutes(args.minutesToSprinkle));
            }

            if( args.verbose ) {
                cout << getTime() << "Forcing device to off " << endl ;
            }

            while(on) {
                con.set( args.device, false ) ;
                this_thread::sleep_for(chrono::milliseconds(1500));
                on = con.get( args.device ) ;
                if(on) {
                    cout << getTime() << "Device is still on, will retry" << endl ;
                }
            } 
        }
        
        if( args.verbose ) {
            cout << getTime() << "Program ended" << endl ;
        }
    } catch( string err ) {
        cerr << err << endl ;
        exit( -2 ) ;
    }    
    return 0 ;
}
