
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
            std::cerr << "Usage:" << argv[0] << " zip eco-device" << std::endl ;
            // "ECO-780C4AA9"
            exit( 2 ) ;
        }
        std::string zip( argv[1] ) ;
        std::string device( argv[2] ) ;

        Weather weather( zip ) ;
        double totalRain = weather.getRecentRainfall() ;
        std::cout << "We had " << totalRain << "mm of rain." << std::endl ;
        if( totalRain > 12 ) {
            std::cout << "No need sprinklers." << std::endl ;
        } else {
            Connection con ; 
            con.discover() ;

            sleep(2) ;
            bool on = con.get( device ) ;
            std::cout << "Device is " << (on?"ON":"OFF") << std::endl ;
            sleep(2) ;
            con.set( device, true ) ;

            sleep(2) ;
            con.set( device, false ) ;
        }
    } catch( std::string err ) {
        std::cerr << err << std::endl ;
    }
    
	return 0 ;
}

