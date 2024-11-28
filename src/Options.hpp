
#pragma once

#include <string>

typedef enum {
    ON ,
    OFF ,
    UNKNOWN 
} ForceState ;


typedef struct  {
    std::string device ;
    std::string zip ;
    int desiredMMRain = 5 ;
    int previousHoursToLookForRain = 48 ;
    ForceState state = UNKNOWN ;
    bool verbose = false ;
    int minutesToSprinkle = 45 ;
    bool test = false ;
    bool list = false ;
    int forecastHours = 24 ;
    std::string certificateFile;
    std::string keyFile;
    std::string historyFile;
} Args ;


Args parseOptions( int argc, char **argv ) ;

std::string getTime() ;
