
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

typedef enum {
    ON ,
    OFF ,
    UNKNOWN 
} ForceState ;


typedef struct  {
    std::string device ;
    std::string zip ;
    int desiredMMRain = 5 ;
    int previousHoursToLookForRain = 12 ;
    ForceState state = UNKNOWN ;
    bool verbose = false ;
} Args ;


Args parseOptions( int argc, char **argv ) ;


int main( int argc, char **argv ) {
    bool turnDeviceOn = false ;

    Args args = parseOptions( argc, argv ) ;
    try {
        Connection con ; 
        con.discover() ;

        if( args.state == ON ) {
            turnDeviceOn = true ;
        } else if( args.state == OFF ) {
            turnDeviceOn = false ;
        } else {
            Weather weather( args.zip, args.previousHoursToLookForRain ) ;
            double totalRain = weather.getRecentRainfall() ;
            if( args.verbose ) {
                std::cout << args.zip << " had " << totalRain << "mm of rain." << std::endl ;
            }
            turnDeviceOn = totalRain < args.desiredMMRain ;
        }

        for( int i=0 ; i<10 ; i++ ) {
            if( con.found( args.device ) ) break ;
	        sleep( 5 ) ;
        }
        
        if( args.verbose ) {
            std::cout << "Turning " << args.device << (turnDeviceOn?" ON":" OFF") << std::endl ;
        }

        bool on = con.get( args.device ) ;
        while( on != turnDeviceOn ) {
            con.set( args.device, turnDeviceOn ) ;
            on = con.get( args.device ) ;
            if( args.verbose ) {
                std::cout << "Device is " << (on?"ON":"OFF") << std::endl ;
            }
        }
    } catch( std::string err ) {
        std::cerr << err << std::endl ;
        exit( -2 ) ;
    }    
    return turnDeviceOn ? 1 : 0 ;
}


void usage( char *argv0 ) ;

Args parseOptions( int argc, char **argv ) {
    Args rc ;

    struct option long_options[] = {
        {"device",  required_argument, nullptr,  'd' },
        {"zip",     optional_argument, nullptr,  'z' },
        {"period",  optional_argument, nullptr,  'p' },
        {"needed",  optional_argument, nullptr,  'n' },
        {"state",   optional_argument, nullptr,  's' },
        {"verbose", no_argument,       nullptr,  'v' },
        {nullptr,   0,                 nullptr,  0 }
    } ;

    int opt ;
    while( (opt = getopt_long(argc, argv, "n:p:d:z:s:v", long_options, nullptr )) != -1 ) {
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
        case 'n':
            rc.desiredMMRain = ::atol( optarg ) ;
            break;
        case 's':
            rc.state = (::strcasecmp( "on", optarg )==0) ? ON : OFF ;
            break;
        case 'v':
            rc.verbose = true ;
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
    std::cerr << "Usage:" << argv0 << " --device device --zip zip <--period HH> <--needed NN>" << std::endl ;
    std::cerr << "       if rain less than NN mm of rain in past HH hours turn on sprinklers" << std::endl ;
    std::cerr << "Usage:" << argv0 << " --device device --state on|off" << std::endl ;
    exit(-1) ;
}