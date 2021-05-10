
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

    try {
        if( argc < 2 ) {
            std::cerr << "Usage:" << argv[0] << " eco-device on|off|NNN <zip>" << std::endl ;
            std::cerr << "       if rain less than NNN mm of rain turn on sprinklers" << std::endl ;
            // "ECO-780C4AA9"
            exit( 2 ) ;
        }
        std::string device( argv[1] ) ;
        std::string onoff( argv[2] ) ;

        bool turnDeviceOn ;

        if( "on"==onoff ) {
            turnDeviceOn = true ;
        } else if( "off"==onoff ) {
            turnDeviceOn = false ;
        } else {
            double minRainfall = ::atof( onoff.c_str() ) ;
            std::string zip( argv[3] ) ;

            Weather weather( zip ) ;
            double totalRain = weather.getRecentRainfall() ;
            std::cout << zip << " had " << totalRain << "mm of rain." << std::endl ;
            turnDeviceOn = totalRain < minRainfall ;
        }

        std::cout << "Turning " << device << (turnDeviceOn?" ON":" OFF") << std::endl ;
        Connection con ; 
        con.discover() ;
        bool on = con.get( device ) ;
        while( on != turnDeviceOn ) {
            con.set( device, turnDeviceOn ) ;
            on = con.get( device ) ;
            std::cout << "Device is " << (on?"ON":"OFF") << std::endl ;
        }

    } catch( std::string err ) {
        std::cerr << err << std::endl ;
    }
    
	return 0 ;
}

