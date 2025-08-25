
#include <iostream>
#include <getopt.h>
#include <cstring>

#include "Options.hpp"
#include "Exceptions.hpp"

using namespace std;


void usage( char *argv0 ) ;

constexpr struct option long_options[] = {
    {"device",  required_argument, nullptr,  'd' },
    {"list",    no_argument,       nullptr,  'l' },
    {"zip",     required_argument, nullptr,  'z' },
    {"period",  required_argument, nullptr,  'p' },
    {"forecast",required_argument, nullptr,  'f' },
    {"needed",  required_argument, nullptr,  'n' },
    {"state",   required_argument, nullptr,  's' },
    {"minutes", required_argument, nullptr,  'm' },
    {"test",    no_argument,       nullptr,  't' },
    {"verbose", no_argument,       nullptr,  'v' },
    {"certFile",required_argument, nullptr,  'c' },
    {"keyFile", required_argument, nullptr,  'k' },
    {"histFile",required_argument, nullptr,  'h' },
    {nullptr,   0,                 nullptr,  0 }
} ;

Args parseOptions( int argc, char **argv, ProgramType programType ) {
    Args rc ;
    int opt ;
    while( (opt = getopt_long(argc, argv, "h:c:k:n:p:d:z:s:m:vtl", long_options, nullptr )) != -1 ) {
        switch (opt) {
        case 'd':
            rc.device = optarg ;
            break;
        case 'z':
            rc.zip = optarg ;
            break;
        case 'c':
            rc.certificateFile = optarg ;
            break;
        case 'h':
            rc.historyFile = optarg ;
            break;
        case 'k':
            rc.keyFile = optarg ;
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
            rc.state = (strncmp( "ON", optarg, 2)==0 || strncmp( "on", optarg, 2)==0) ? ON : OFF ;
            break;
        case 'v':
            rc.verbose = true ;
            break;
        case 't':
            rc.test = true ;
            break;
        case 'l':
            rc.list = true ;
            break;
        default: /* '?' */
            usage( argv[0] ) ;
            break ;
        }
    }

    // Validate required arguments based on program type
    switch (programType) {
        case ProgramType::SPRINKLERS:
            if (!rc.list) {
                if (rc.device.empty()) {
                    throw ConfigurationException("Device name is required (use --device or --list)");
                }
                
                if (rc.state == UNKNOWN && rc.zip.empty()) {
                    throw ConfigurationException("Either zip code (--zip) or explicit state (--state) is required");
                }
            }
            break;
            
        case ProgramType::READWEATHER:
            if (rc.zip.empty()) {
                throw ConfigurationException("Zip code is required for weather reading (use --zip)");
            }
            break;
            
        case ProgramType::WEBSERVER:
            // Webserver has minimal requirements - will get device from config
            break;
    }
    
    // Validate argument ranges
    if (rc.desiredMMRain < 0 || rc.desiredMMRain > 100) {
        throw ConfigurationException("desiredMMRain must be between 0 and 100mm");
    }
    
    if (rc.minutesToSprinkle < 1 || rc.minutesToSprinkle > 180) {
        throw ConfigurationException("minutesToSprinkle must be between 1 and 180 minutes");
    }
    
    if (rc.previousHoursToLookForRain < 1 || rc.previousHoursToLookForRain > 168) {
        throw ConfigurationException("previousHoursToLookForRain must be between 1 and 168 hours");
    }
    
    if (rc.forecastHours < 1 || rc.forecastHours > 72) {
        throw ConfigurationException("forecastHours must be between 1 and 72 hours");
    }
    
    return rc ;
}

void usage( char *argv0 ) {
    cerr << "Usage:" << argv0 << " --device device --zip zip <--minutes=MMM> <--period HH> <--forecast FF> <--needed NN> <--test>" << endl ;
    cerr << "       sprinkle for MMM minutes if rain less than NN mm of rain in past HH hours plus FF forecast rain" << endl ;
    cerr << "       --test means show what would be done, don't change sprinklers" << endl ;
    cerr << "Usage:" << argv0 << " --device device --state on|off" << endl ;
    cerr << "Usage:" << argv0 << " --list" << endl ;

    cerr << "Options --certFile & --keyFile are for https params for webserver" << endl ;
    cerr << "Options --histFile is where the weather & sprinkling history is stored" << endl ;
    
    exit(-1) ;
}



std::string getTime() {
    time_t rawtime;
    struct tm * timeinfo;
    char buffer [128]; 

    time( &rawtime );
    timeinfo = localtime( &rawtime ) ;

    strftime (buffer,80,"%Y-%m-%d %H:%M:%S ",timeinfo);

    return std::string( buffer ) ;
}
