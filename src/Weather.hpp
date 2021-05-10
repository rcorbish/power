
#pragma once

#ifndef TEST_FRIENDS
#define TEST_FRIENDS
#endif

#include <string>

class Weather {
    TEST_FRIENDS 
    friend size_t WriteMemoryCallbackCurrent(void *, size_t, size_t, void *) ;
    friend size_t WriteMemoryCallbackHistory(void *, size_t, size_t, void *) ;

    double lon ; 
    double lat ;
    double totalRainFall ;

private :
    std::string current_url ;
    std::string history_url ;
protected:
    void parseCurrent( char *contents, size_t sz ) ;
    void parseHistory( char *contents, size_t sz ) ;
    void read() ;

public :
    Weather( std::string zip ) ;
    double getRecentRainfall() ;
} ;