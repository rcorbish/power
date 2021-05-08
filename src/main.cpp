
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
        Weather weather( "Brunswick", "ga", "us" ) ;
        weather.isRaining() ;

        Connection con ; 
        con.discover() ;

        sleep(2) ;
        bool on = con.get( "ECO-780C4AA9" ) ;
        std::cout << "Device is " << (on?"ON":"OFF") << std::endl ;
        sleep(2) ;
        con.set( "ECO-780C4AA9", true ) ;

        sleep(2) ;
        con.set( "ECO-780C4AA9", false ) ;
    } catch( std::string err ) {
        std::cerr << err << std::endl ;
    }
    
	return 0 ;
}

