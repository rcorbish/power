
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
#include "Options.hpp"



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

