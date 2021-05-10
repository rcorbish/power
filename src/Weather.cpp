
#include "JsonParser.hpp"
#include "Weather.hpp"

#include <curl/curl.h>
#include <iostream>
#include <sstream>
#include <stack>
#include <cmath>


Weather::Weather( std::string zip ) {
    char *api_key = getenv("WEATHER_API_KEY");
    if (api_key == nullptr) {
        throw std::string( "Missing WEATHER_API_KEY environment variable" ) ;
    }
    std::string Server( "https://api.openweathermap.org/data/2.5/" ) ;

    current_url = Server + "weather?zip=" + zip + "&units=metric&APPID=" + api_key ;
    history_url = Server + "onecall/timemachine?units=metric&appid=" + api_key ;
}

size_t
WriteMemoryCallbackCurrent(void *contents, size_t size, size_t nmemb, void *userp) {
    std::string json( (char*)contents ) ;
    Weather * self = (Weather *)userp ;
    self->parseCurrent( (char*)contents, size * nmemb ) ;
    return size * nmemb ;
}
size_t
WriteMemoryCallbackHistory(void *contents, size_t size, size_t nmemb, void *userp) {
    std::string json( (char*)contents ) ;
    Weather * self = (Weather *)userp ;
    self->parseHistory( (char*)contents, size * nmemb ) ;
    return size * nmemb ;
}

void Weather::parseCurrent( char *contents, size_t sz ) {
    std::stack<std::string> tags ;
    char *p = (char*)contents ;
    char *end = p + sz ;

    std::set<std::string> keys ;
    keys.emplace( "coord.lon" ) ;
    keys.emplace( "coord.lat" ) ;
    JsonParser parser( contents, keys ) ;
    
    lon = parser.getNumber( "coord.lon" ) ;
    lat = parser.getNumber( "coord.lat" ) ;
    // std::cout << lon << "," << lat << std::endl ;
}

void Weather::parseHistory( char *contents, size_t sz ) {
    std::stack<std::string> tags ;
    char *p = (char*)contents ;
    char *end = p + sz ;

    std::set<std::string> keys ;
    for( int i=0 ; i<24 ; i++ ) {
        std::ostringstream ss ;
        ss << "hourly[" << i << "].rain.1h" ;
        keys.emplace( ss.str() ) ;
    }
    JsonParser parser( contents, keys ) ;

    for( auto &i : keys ) {
        double mmRainfall = parser.getNumber( i ) ;
        if( !std::isnan(mmRainfall) ) {
            totalRainFall += mmRainfall ;
        }
    }
}




void Weather::read() {
    CURL *curl;
    CURLcode res;
    curl_global_init(CURL_GLOBAL_DEFAULT) ;
    curl = curl_easy_init();
    if (curl) {
        curl_easy_setopt(curl, CURLOPT_URL, current_url.c_str() ) ;

        /* send all data to this function  */
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteMemoryCallbackCurrent);
        
        /* we pass our 'chunk' struct to the callback function */
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, this ) ;
        
        /* some servers don't like requests that are made without a user-agent
            field, so we provide one */
        curl_easy_setopt(curl, CURLOPT_USERAGENT, "sprinklers");

        /* Perform the request, res will get the return code */
        res = curl_easy_perform(curl);
        /* Check for errors */
        if (res != CURLE_OK)
            throw std::string( curl_easy_strerror(res) ) ;

        totalRainFall = 0 ;

        time_t now = time( nullptr ) ;
        long yesterday =  now ;
        std::ostringstream ss ;
        ss << history_url << "&lat=" << lat << "&lon=" << lon << "&dt=" << yesterday ;
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteMemoryCallbackHistory );
        curl_easy_setopt(curl, CURLOPT_URL, ss.str().c_str() ) ;
        res = curl_easy_perform(curl);
        /* Check for errors */
        if (res != CURLE_OK)
            throw std::string( curl_easy_strerror(res) ) ;

        yesterday -=  86400 ;
        ss.str("");
        ss.clear();
        ss << history_url << "&lat=" << lat << "&lon=" << lon << "&dt=" << yesterday ;
        curl_easy_setopt(curl, CURLOPT_URL, ss.str().c_str() ) ;
        res = curl_easy_perform(curl);
        /* Check for errors */
        if (res != CURLE_OK)
            throw std::string( curl_easy_strerror(res) ) ;

        yesterday -=  86400 ;
        ss.str("");
        ss.clear();
        ss << history_url << "&lat=" << lat << "&lon=" << lon << "&dt=" << yesterday ;
        curl_easy_setopt(curl, CURLOPT_URL, ss.str().c_str() ) ;
        res = curl_easy_perform(curl);
        /* Check for errors */
        if (res != CURLE_OK)
            throw std::string( curl_easy_strerror(res) ) ;

        curl_easy_cleanup(curl);
    }

    curl_global_cleanup() ;
}


double Weather::getRecentRainfall() {
    read() ;
    return totalRainFall ;
}

