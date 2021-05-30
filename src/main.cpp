
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

#include "Connection.hpp"
#include "Weather.hpp"

int main( int argc, char **argv ) {
    bool turnDeviceOn = false ;

    try {
        Connection con ; 
        con.discover() ;

        if( argc < 3 ) {
            std::cerr << "Usage:" << argv[0] << " eco-device on|off|NNN <zip> <HHH>" << std::endl ;
            std::cerr << "       if rain less than NNN mm of rain in past HHH hours turn on sprinklers" << std::endl ;
            // "ECO-780C4AA9"
            exit( -1 ) ;
        }
        std::string device( argv[1] ) ;
        std::string onoff( argv[2] ) ;


        if( "on"==onoff ) {
            turnDeviceOn = true ;
        } else if( "off"==onoff ) {
            turnDeviceOn = false ;
        } else {
            if( argc < 4 ) {
                std::cerr << "Usage:" << argv[0] << " eco-device on|off|NNN <zip> <HHH>" << std::endl ;
                std::cerr << "       if rain less than NNN mm of rain in past HHH hours turn on sprinklers" << std::endl ;
                exit( -1 ) ;
            }
            std::string zip( argv[3] ) ;
            double minRainfall = ::atof( onoff.c_str() ) ;

            double pastHours = 48 ;
            if( argc < 5 ) {
                std::string hhh( argv[4] ) ;
                pastHours = ::atof( hhh.c_str() ) ;
            }

            Weather weather( zip, pastHours ) ;
            double totalRain = weather.getRecentRainfall() ;
            std::cout << zip << " had " << totalRain << "mm of rain." << std::endl ;
            turnDeviceOn = totalRain < minRainfall ;
        }

	sleep( 5 ) ;
        std::cout << "Turning " << device << (turnDeviceOn?" ON":" OFF") << std::endl ;
        bool on = con.get( device ) ;
        while( on != turnDeviceOn ) {
            con.set( device, turnDeviceOn ) ;
            on = con.get( device ) ;
            std::cout << "Device is " << (on?"ON":"OFF") << std::endl ;
        }

    } catch( std::string err ) {
        std::cerr << err << std::endl ;
        exit( -2 ) ;
    }
    
    return turnDeviceOn ? 1 : 0 ;
}

