
#pragma once

#ifndef TEST_FRIENDS
#define TEST_FRIENDS
#endif

#include <string>

class Weather {
    TEST_FRIENDS 
    friend size_t WriteMemoryCallbackCurrent(void *, size_t, size_t, void *) ;
    friend size_t WriteMemoryCallbackHistory(void *, size_t, size_t, void *) ;
    friend size_t WriteMemoryCallbackForecast(void *, size_t, size_t, void *) ;

    double lon ; 
    double lat ;
    double totalRainFall ;
    double forecastRainChance ;
    long rainSince ;
    int hoursForecast ;

private :
    std::string current_url ;
    std::string history_url ;
    std::string forecast_url ;
protected:
    void parseCurrent( char *contents, size_t sz ) ;
    void parseHistory( char *contents, size_t sz ) ;
    void parseForecast( char *contents, size_t sz ) ;

public :
    Weather( std::string zip, long pastHours, long forecastHours ) ;
    void read() ;
    double getRecentRainfall() ;
    double getForecastRainChance() ;
} ;