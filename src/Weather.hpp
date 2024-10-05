
#pragma once

#ifndef TEST_FRIENDS
#define TEST_FRIENDS
#endif

#include <sstream>
#include <string>

class Weather {
    TEST_FRIENDS 
    friend size_t WriteMemoryCallback(char *, size_t, size_t, void *);
private :
    char *responseBuffer;
    char *responseBufferPosition;
    double lon; 
    double lat;
    double totalRainFall;
    double forecastRainChance;
    std::ostringstream description;
    int daysOfHistory;

    std::string location_url ;
    std::string history_url ;
    std::string forecast_url ;
    void sendUrlRequest( void *curl, const char *url, size_t (*write_callback)(char *ptr, size_t size, size_t nmemb, void *userdata) ) ;

protected:
    void parseLocation( char *contents, size_t sz ) ;
    void parseHistory( char *contents, size_t sz ) ;
    void parseForecast( char *contents, size_t sz ) ;

public :
    Weather( std::string zip, int pastHours, int forecastHours );
    virtual ~Weather() { delete responseBuffer; }
    void init();
    void read();
    double getRecentRainfall() const;
    double getForecastRainChance() const;
    const std::string getDescription() const;
} ;