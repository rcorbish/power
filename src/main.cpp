
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

using namespace std;

typedef enum {
    ON ,
    OFF ,
    UNKNOWN 
} ForceState ;



typedef struct  {
    string device ;
    string zip ;
    int desiredMMRain = 5 ;
    int previousHoursToLookForRain = 48 ;
    ForceState state = UNKNOWN ;
    bool verbose = false ;
    int minutesToSprinkle = 45 ;
    bool test = false ;
    bool list = false ;
    int forecastHours = 24 ;
} Args ;


Args parseOptions( int argc, char **argv ) ;
string getTime() ;


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
        con.discover();

        if( args.list ) {
            for( auto entry : con.list() ) {
                cout << entry.first << endl ;
            }
        } else if( args.state == ON ) {
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

        if( args.list ) {
            con.discover() ;
            this_thread::sleep_for(chrono::seconds(12));
            for( auto entry : con.list() ) {
                cout << entry.first << endl ;
            }
        } else { 
            for( int i=0 ; i<10 ; i++ ) {
                if( con.found( args.device ) ) break ;
                con.discover() ;
                this_thread::sleep_for(chrono::seconds(7));
            }
        }

        bool on = con.get( args.device ) ;
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


void usage( char *argv0 ) ;

constexpr struct option long_options[] = {
    {"device",  required_argument, nullptr,  'd' },
    {"list",    no_argument,       nullptr,  'l' },
    {"zip",     required_argument, nullptr,  'z' },
    {"period",  required_argument, nullptr,  'p' },
    {"forecast",  required_argument, nullptr,  'f' },
    {"needed",  required_argument, nullptr,  'n' },
    {"state",   required_argument, nullptr,  's' },
    {"minutes", required_argument, nullptr,  'm' },
    {"test",    no_argument,       nullptr,  't' },
    {"verbose", no_argument,       nullptr,  'v' },
    {nullptr,   0,                 nullptr,  0 }
} ;

Args parseOptions( int argc, char **argv ) {
    Args rc ;
    int opt ;
    while( (opt = getopt_long(argc, argv, "n:p:d:z:s:m:vtl", long_options, nullptr )) != -1 ) {
        switch (opt) {
        case 'd':
            rc.device = optarg ;
            break;
        case 'z':
            rc.zip = optarg ;
            break;
        case 'p':
            rc.previousHoursToLookForRain = ::atol( optarg ) ;
            break;
        case 'f':
            rc.forecastHours = ::atol( optarg ) ;
            break;
        case 'n':
            rc.desiredMMRain = ::atol( optarg ) ;
            break;
        case 'm':
            rc.minutesToSprinkle = ::atol( optarg ) ;
            break;
        case 's':
            rc.state = (::strcasecmp( "on", optarg )==0) ? ON : OFF ;
            break;
        case 'v':
            rc.verbose = true ;
            break;
        case 't':
            rc.test = true ;
            break;
        case 'l':
            rc.list = true ;
            break;
        default: /* '?' */
            usage( argv[0] ) ;
            break ;
        }
    }

    if( rc.device.size() == 0 && !rc.list )  usage( argv[0] ) ;
    if( rc.state == UNKNOWN && rc.zip.size() == 0 && !rc.list )  usage( argv[0] ) ;
    return rc ;
}

void usage( char *argv0 ) {
    cerr << "Usage:" << argv0 << " --device device --zip zip <--minutes=MMM> <--period HH> <--forecast FF> <--needed NN> <--test>" << endl ;
    cerr << "       sprinkle for MMM minutes if rain less than NN mm of rain in past HH hours plus FF forecast rain" << endl ;
    cerr << "       --test means show what would be done, don't change sprinklers" << endl ;
    cerr << "Usage:" << argv0 << " --device device --state on|off" << endl ;
    cerr << "Usage:" << argv0 << " --list" << endl ;
    
    exit(-1) ;
}

string getTime() {
    time_t rawtime;
    struct tm * timeinfo;
    char buffer [80]; 

    time( &rawtime );
    timeinfo = localtime( &rawtime ) ;

    strftime (buffer,80,"%d-%b-%Y %H:%M:%S ",timeinfo);

    return string( buffer ) ;
}
