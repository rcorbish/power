
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

#define TEST_FRIENDS friend int main( int , char ** ) ; 

#include "Connection.hpp"
#include "Weather.hpp"

#define TEST_JSON1 "{\"coord\":{\"lon\":-81.4915,\"lat\":31.15},\"weather\":[{\"id\":800,\"main\":\"Clear\",\"description\":\"clear sky\",\"icon\":\"01d\"}],\"base\":\"stations\",\"main\":{\"temp\":284.53,\"feels_like\":283.34,\"temp_min\":283.15,\"temp_max\":286.15,\"pressure\":1020,\"humidity\":62},\"visibility\":10000,\"wind\":{\"speed\":3.09,\"deg\":330},\"clouds\":{\"all\":1},\"dt\":1620474176,\"sys\":{\"type\":1,\"id\":5922,\"country\":\"US\",\"sunrise\":1620470109,\"sunset\":1620518984},\"timezone\":-14400,\"id\":4184845,\"name\":\"Brunswick\",\"cod\":200}"

int main( int argc, char **argv ) {

    try {
        Weather weather( "brunswick", "ga", "us" ) ;

        char s1[sizeof(TEST_JSON1)+1] ;
        strncpy( s1, TEST_JSON1, sizeof(TEST_JSON1) ) ;

        weather.parseIsRaining( s1, sizeof(TEST_JSON1) ) ;

    } catch( std::string err ) {
        std::cerr << err << std::endl ;
    }
    
	return 0 ;
}

