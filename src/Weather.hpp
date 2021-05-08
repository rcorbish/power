
#pragma once

#ifndef TEST_FRIENDS
#define TEST_FRIENDS
#endif

#include <string>

class Weather {
    TEST_FRIENDS 
    friend size_t WriteMemoryCallback(void *, size_t, size_t, void *) ;

private :
    std::string url ;
protected:
    bool parseIsRaining( char *contents, size_t sz ) ;

public :
    Weather( std::string city, std::string state, std::string country ) ;

    bool isRaining() ;
} ;