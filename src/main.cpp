
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

#include "Connection.hpp"
#include "Weather.hpp"
#include "History.hpp"


typedef enum {
    ON ,
    OFF ,
    UNKNOWN 
} ForceState ;



typedef struct  {
    std::string device ;
    std::string zip ;
    int desiredMMRain = 5 ;
    int previousHoursToLookForRain = 24 ;
    ForceState state = UNKNOWN ;
    bool verbose = false ;
    int minutesToSprinkle = 45 ;
    bool test = false ;
    int forecastHours = 12 ;
} Args ;


Args parseOptions( int argc, char **argv ) ;
std::string getTime() ;


int main( int argc, char **argv ) {
    bool turnDeviceOn = false ;

    Args args = parseOptions( argc, argv ) ;
    if( args.verbose && args.test ) {
        std::cout << getTime() << "Test mode" << std::endl ;
    }

    try {
        Connection con ; 
        con.discover() ;

        if( args.state == ON ) {
            turnDeviceOn = true ;
        } else if( args.state == OFF ) {
            turnDeviceOn = false ;
        } else {
            Weather weather( args.zip, args.previousHoursToLookForRain, args.forecastHours ) ;
            weather.read() ;
            double totalRain = weather.getRecentRainfall() ;
            double forecastRain = weather.getForecastRainChance() ;

            if( args.verbose ) {
                std::cout << getTime() << args.zip << " received " << totalRain << "mm and forecasts " << forecastRain << "mm of rain." << std::endl ;
            }
            turnDeviceOn = (totalRain+forecastRain*.5) < args.desiredMMRain ;
            HistoryEntry he( totalRain, forecastRain, args.minutesToSprinkle ) ;
            appendHistory( he ) ;
        }

        for( int i=0 ; i<10 ; i++ ) {
            if( con.found( args.device ) ) break ;
            con.discover() ;
	        sleep( 5 ) ;
        }

        if( args.verbose ) {
            bool on = con.get( args.device ) ;
            if( on == turnDeviceOn ) {
                std::cout << getTime() << "Device is already " << (on?"ON":"OFF") << std::endl ;
            } else {
                std::cout << getTime() << "Need to turn device " << (turnDeviceOn?"ON":"OFF") << std::endl ;
            }
        }

        if( !args.test ) {
            bool on = con.get( args.device ) ;
            while( on != turnDeviceOn ) {
                con.set( args.device, turnDeviceOn ) ;
                on = con.get( args.device ) ;
            }

            if( on ) {
                if( args.verbose ) {
                    std::cout << getTime() << "Sprinkling for " << args.minutesToSprinkle << " minutes." << std::endl ;
                }
                sleep( args.minutesToSprinkle * 60 ) ;
                do {
                    con.set( args.device, false ) ;
                    on = con.get( args.device ) ;
                    sleep( 1 ) ;
                } while( on ) ;
            }
        }
        if( args.verbose ) {
            std::cout << getTime() << "Program ended" << std::endl ;
        }
    } catch( std::string err ) {
        std::cerr << err << std::endl ;
        exit( -2 ) ;
    }    
    return 0 ;
}


void usage( char *argv0 ) ;

constexpr struct option long_options[] = {
    {"device",  required_argument, nullptr,  'd' },
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
    while( (opt = getopt_long(argc, argv, "n:p:d:z:s:m:v", long_options, nullptr )) != -1 ) {
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
        default: /* '?' */
            usage( argv[0] ) ;
            break ;
        }
    }

    if( rc.device.size() == 0 )  usage( argv[0] ) ;
    if( rc.state == UNKNOWN && rc.zip.size() == 0 )  usage( argv[0] ) ;
    return rc ;
}

void usage( char *argv0 ) {
    std::cerr << "Usage:" << argv0 << " --device device --zip zip <--minutes=MMM> <--period HH> <--forecast FF> <--needed NN> <--test>" << std::endl ;
    std::cerr << "       sprinkle for MMM minutes if rain less than NN mm of rain in past HH hours plus FF forecast rain" << std::endl ;
    std::cerr << "Usage:" << argv0 << " --device device --state on|off" << std::endl ;
    std::cerr << "       --test means show what would be done, don't change sprinklers" << std::endl ;
    
    exit(-1) ;
}

std::string getTime() {
    time_t rawtime;
    struct tm * timeinfo;
    char buffer [80]; 

    time( &rawtime );
    timeinfo = localtime( &rawtime ) ;

    strftime (buffer,80,"%d-%b-%Y %H:%M:%S ",timeinfo);

    return std::string( buffer ) ;
}
