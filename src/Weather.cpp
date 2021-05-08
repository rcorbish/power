
#include "Weather.hpp"

#include <curl/curl.h>
#include <iostream>
#include <sstream>
#include <stack>

char * parseArray( char *p, std::stack<std::string> &tags ) ;
char * parseValue( char *p, std::stack<std::string> &tags ) ;
char * parseObject( char *p, std::stack<std::string> &tags ) ;

Weather::Weather(std::string city, std::string state, std::string country) {
    char *api_key = getenv("WEATHER_API_KEY");
    if (api_key == nullptr) {
        throw std::string( "Missing WEATHER_API_KEY environment variable" ) ;
    }
    url = "https://api.openweathermap.org/data/2.5/weather?q=" +
          city + "," +
          state + "," +
          country + "&units=metric&APPID=" + api_key;
    // curl "https://api.openweathermap.org/data/2.5/onecall/timemachine?lat=31.1499528&lon=-81.4914894&dt=1620392400&APPID=4302a8f4ea04545b9d2304d3f63ab702"
}


size_t
WriteMemoryCallback(void *contents, size_t size, size_t nmemb, void *userp) {
    std::string json( (char*)contents ) ;
    Weather * self = (Weather *)userp ;
    self->parseIsRaining( (char*)contents, size * nmemb ) ;
    return size * nmemb ;
}

bool Weather::parseIsRaining( char *contents, size_t sz ) {
    std::stack<std::string> tags ;
    char *p = (char*)contents ;
    char *end = p + sz ;

    parseObject( p, tags ) ;

    return true ;
}

char * parseArray( char *p, std::stack<std::string> &tags ) {
    if( *p++ != '[' ) {
        throw std::string( "Malformed JSON, arrays must start with '['." ) ;
    }

    std::string arrayElement = tags.top() ;
    tags.pop() ;

    int item = 0 ;
    while( *p != ']' ) {
        std::ostringstream indexedArrayElement;
        indexedArrayElement << arrayElement << '[' << item << ']' ;
        tags.push( indexedArrayElement.str() ) ;
        p = parseValue( p, tags ) ;
        while( isspace( *p ) ) p++ ;     // skip whitespace
        if( *p == ',' ) {               // skip commas between values
            p++ ;            
            while( isspace( *p ) ) p++ ;     // skip whitespace
            if( *p != '"' ) {
                throw std::string( "Malformed JSON, trailing commas not allowed in array." ) ;
            }
        }
        tags.pop() ;
    }
    tags.push( arrayElement ) ;
    return ++p ;
}

char * parseValue( char *p, std::stack<std::string> &tags ) {
    while( isspace( *p ) ) p++ ;     // skip whitespace
    
    if( *p == '"' ) {   // value is a text

        char *textStart = ++p ;

        // skip to end of text - ignore escaped quotes
        while( *p != '"' ) if( *p++ == '\\' && *p=='"' ) p++ ;

        *p++ = 0 ;
        std::cout << tags.top() << " = " << textStart << std::endl ;
    } else if ( *p == '{' ) {            // value is an object
        p = parseObject( p, tags ) ;
    } else if ( *p == '[' ) {            // value is an array
        p = parseArray( p, tags ) ;
    } else {
        char *numberStart = p ;
        if( *p == '-' || *p == '+' ) p++ ;   // skip number sign

        while( (*p>='0' && *p<='9') || *p == '.' ) p++ ;

        char c = *p ;
        *p = 0 ;
        double fval = ::atof( numberStart ) ;
        *p = c ;
        std::cout << tags.top() << " = " << fval << std::endl ;
    }
    return p ;
}

char * parseObject( char *p, std::stack<std::string> &tags ) {
    if( *p++ != '{' ) {
        throw std::string( "Malformed JSON, objects must start with '{'." ) ;
    }
    while( isspace( *p ) ) p++ ;     // skip whitespace
    while( *p != '}' ) {         // at end ?
        if( *p++ != '"' ) {
            throw std::string( "Malformed JSON, identifiers must be quoted \"." ) ;
        }
        char *tagName = p ;         // got start of tag
        while( *p != '"' ) p++ ;    // find end of tag
        *p++ = 0 ;                  // null terminator written to input !!!

        std::string parentTag = tags.empty() ? tagName : (tags.top() + "." + tagName) ;
        tags.push( parentTag ) ;

        while( isspace( *p ) ) p++ ;     // skip whitespace
        if( *p++ != ':' ) {
            throw std::string( "Malformed JSON, Missing : between name & value." ) ;
        }

        p = parseValue( p, tags ) ;
        tags.pop() ;
        while( isspace( *p ) ) p++ ;     // skip whitespace
        if( *p == ',' ) {               // skip commas between values
            p++ ;            
            while( isspace( *p ) ) p++ ;     // skip whitespace
            if( *p == '{' ) {
                throw std::string( "Malformed JSON, trailing commas not allowed." ) ;
            }
        }
    }
    return ++p ;
}



bool Weather::isRaining() {
    CURL *curl;
    CURLcode res;
    curl_global_init(CURL_GLOBAL_DEFAULT) ;
    curl = curl_easy_init();
    bool isRaining ;
    if (curl) {
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str() ) ;

        /* send all data to this function  */
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);
        
        /* we pass our 'chunk' struct to the callback function */
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &isRaining ) ;
        
        /* some servers don't like requests that are made without a user-agent
            field, so we provide one */
        curl_easy_setopt(curl, CURLOPT_USERAGENT, "sprinklers");

        /* Perform the request, res will get the return code */
        res = curl_easy_perform(curl);
        /* Check for errors */
        if (res != CURLE_OK)
            throw std::string( curl_easy_strerror(res) ) ;

        curl_easy_cleanup(curl);
    }
    curl_global_cleanup() ;

    return isRaining ;
}