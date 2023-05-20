
#pragma once

#ifndef TEST_FRIENDS
#define TEST_FRIENDS
#endif

#include <string>

class Weather {
    TEST_FRIENDS 
    friend size_t WriteMemoryCallbackCurrent(char *, size_t, size_t, void *) ;
    friend size_t WriteMemoryCallbackHistory(char *, size_t, size_t, void *) ;
    friend size_t WriteMemoryCallbackForecast(char *, size_t, size_t, void *) ;

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
    void sendUrlRequest( void *curl, const char *url, size_t (*write_callback)(char *ptr, size_t size, size_t nmemb, void *userdata) ) ;

protected:
    void parseCurrent( char *contents, size_t sz ) ;
    void parseHistory( char *contents, size_t sz ) ;
    void parseForecast( char *contents, size_t sz ) ;

public :
    Weather( std::string zip, long pastHours, long forecastHours ) ;
    void init();
    void read() ;
    double getRecentRainfall() ;
    double getForecastRainChance() ;
} ;