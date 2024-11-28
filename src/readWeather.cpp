
#include <string.h>
#include <iostream>
#include <iomanip>
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


typedef struct  {
    std::string zip ;
    int previousHoursToLookForRain = 24 ;
    int forecastHours = 12 ;
} Args ;


Args parseOptions( int argc, char **argv ) ;
std::string getTime() ;


int main( int argc, char **argv ) {
    bool turnDeviceOn = false ;

    Args args = parseOptions( argc, argv ) ;

    try {
        
        Weather weather( args.zip, args.previousHoursToLookForRain, args.forecastHours );
        weather.init();
        weather.read();
        double totalRain = weather.getRecentRainfall() ;
        double forecastRain = weather.getForecastRainfall() ;

        std::cout << getTime() << std::setw(8) << args.zip << std::setw(8) << totalRain << std::setw(8) << forecastRain << std::endl ;
        
    } catch( std::string err ) {
        std::cerr << err << std::endl ;
        exit( -2 ) ;
    }    
    return 0 ;
}


void usage( char *argv0 ) ;

constexpr struct option long_options[] = {
    {"zip",     required_argument, nullptr,  'z' },
    {"period",  required_argument, nullptr,  'p' },
    {"forecast",  required_argument, nullptr,  'f' },
    {nullptr,   0,                 nullptr,  0   }
} ;

Args parseOptions( int argc, char **argv ) {
    Args rc ;
    int opt ;
    while( (opt = getopt_long(argc, argv, "p:z:f:", long_options, nullptr )) != -1 ) {
        switch (opt) {
        case 'z':
            rc.zip = optarg ;
            break;
        case 'p':
            rc.previousHoursToLookForRain = ::atol( optarg ) ;
            break;
        case 'f':
            rc.forecastHours = ::atol( optarg ) ;
            break;
        default: /* '?' */
            usage( argv[0] ) ;
            break ;
        }
    }

    if( rc.zip.size() == 0 )  usage( argv[0] ) ;
    return rc ;
}

void usage( char *argv0 ) {
    std::cerr << "Usage:" << argv0 << " --zip zip <--period HH> <--forecast FF> " << std::endl ;    
    exit(-1) ;
}

std::string getTime() {
    time_t rawtime;
    struct tm * timeinfo;
    char buffer [128]; 

    time( &rawtime );
    timeinfo = localtime( &rawtime ) ;

    strftime (buffer,80,"%Y-%m-%d %H:%M:%S",timeinfo);

    return std::string( buffer ) ;
}
