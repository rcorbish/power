
#include "JsonParser.hpp"
#include "Weather.hpp"

#include <curl/curl.h>
#include <iostream>
#include <sstream>
#include <stack>
#include <cmath>



Weather::Weather( std::string zip, long pastHours, long forecastHours ) {
    char *api_key = getenv("WEATHER_API_KEY");
    if (api_key == nullptr) {
        throw std::string( "Missing WEATHER_API_KEY environment variable" ) ;
    }
    std::string Server( "https://api.openweathermap.org/data/2.5/" ) ;

    current_url = Server + "weather?zip=" + zip + "&units=metric&APPID=" + api_key ;
    history_url = Server + "onecall/timemachine?units=metric&appid=" + api_key ;
    forecast_url = Server + "onecall?units=metric&exclude=current,minutely,daily,alerts&APPID=" + api_key ;

    rainSince = time( nullptr ) - ( pastHours * 3600 ) ;
    hoursForecast = forecastHours ;
}




size_t
WriteMemoryCallbackCurrent(char *contents, size_t size, size_t nmemb, void *userp) {
    std::string json( (char*)contents);
    Weather * self = (Weather *)userp;
    self->parseCurrent( (char*)contents, size * nmemb );
    return size * nmemb;
}

size_t
WriteMemoryCallbackHistory(char *contents, size_t size, size_t nmemb, void *userp) {
    std::string json( (char*)contents);
    Weather * self = (Weather *)userp;
    self->parseHistory( (char*)contents, size * nmemb);
    return size * nmemb;
}

size_t
WriteMemoryCallbackForecast(char *contents, size_t size, size_t nmemb, void *userp) {
    std::string json( (char*)contents);
    Weather * self = (Weather *)userp;
    self->parseForecast( (char*)contents, size * nmemb);
    return size * nmemb;
}

void Weather::init() {
    CURL *curl;
    CURLcode res;
    curl_global_init(CURL_GLOBAL_DEFAULT) ;
    curl = curl_easy_init();
    if (curl) {
        sendUrlRequest( curl, current_url.c_str(), WriteMemoryCallbackCurrent ) ;
        curl_easy_cleanup(curl);
    }
    curl_global_cleanup() ;
}

void Weather::parseCurrent( char *contents, size_t sz ) {
    std::stack<std::string> tags;
    char *p = (char*)contents;
    char *end = p + sz;

    std::set<std::string> keys;
    keys.emplace( "coord.lon" );
    keys.emplace( "coord.lat" );
    JsonParser parser( contents, keys ) ;
    
    lon = parser.getNumber( "coord.lon" );
    lat = parser.getNumber( "coord.lat" );
    // std::cout << lon << "," << lat << std::endl ;
}


void Weather::parseHistory( char *contents, size_t sz ) {
    std::stack<std::string> tags ;
    char *p = (char*)contents ;
    char *end = p + sz ;

    std::set<std::string> keys ;
    for( int i=0 ; i<24 ; i++ ) {
        std::ostringstream ssRain ;
        ssRain << "hourly[" << i << "].rain.1h" ;
        keys.emplace( ssRain.str() ) ;
        std::ostringstream ssDt ;
        ssDt << "hourly[" << i << "].dt" ;
        keys.emplace( ssDt.str() ) ;
    }
    keys.emplace("current.weather[0].description");
    keys.emplace("current.weather[1].description");
    keys.emplace("current.weather[2].description");
    keys.emplace("current.temp");
    keys.emplace("current.humidity");
    JsonParser parser( contents, keys ) ;

    for( int i=0 ; i<24 ; i++ ) {
        std::ostringstream ssDt ;
        ssDt << "hourly[" << i << "].dt" ;
        keys.emplace( ssDt.str() ) ;
        double date = parser.getNumber( ssDt.str() ) ;
        if( date > rainSince ) {    // only consider past 48 hours
            std::ostringstream ssRain ;
            ssRain << "hourly[" << i << "].rain.1h" ;
            keys.emplace( ssRain.str() ) ;

            double mmRainfall = parser.getNumber( ssRain.str() ) ;
            if( !std::isnan(mmRainfall) ) {
                totalRainFall += mmRainfall ;
            }
        }
    }
    double degc = parser.getNumber("current.temp");
    double degf = degc * 9.0 / 5.0 + 32 ;
    description << degc << "°C/" << degf << "°F " 
                << parser.getNumber("current.humidity") << "% "  
                << parser.getText("current.weather[0].description");
    if( parser.has("current.weather[1].description") ) {
        description << " " << parser.getText("current.weather[1].description");
    }
    if( parser.has("current.weather[2].description") ) {
        description << " " << parser.getText("current.weather[2].description");
    }
}


void Weather::parseForecast( char *contents, size_t sz ) {
    std::stack<std::string> tags ;
    char *p = (char*)contents ;
    char *end = p + sz ;
   

    std::set<std::string> keys ;
    for( int i=0 ; i<hoursForecast ; i++ ) {
        std::ostringstream ssRainPctChange ;
        ssRainPctChange << "hourly[" << i << "].pop" ;
        keys.emplace( ssRainPctChange.str() ) ;
    }

    JsonParser parser( contents, keys ) ;

    for( int i=0 ; i<hoursForecast ; i++ ) {
        std::ostringstream ssRainPctChange ;
        ssRainPctChange << "hourly[" << i << "].pop" ;

        double chanceOfRain = parser.getNumber( ssRainPctChange.str() ) ;
        if( !std::isnan(chanceOfRain) ) {
            forecastRainChance += chanceOfRain ;
        }
    }
}


void Weather::read() {
    // setup accumulators
    totalRainFall = 0 ;
    forecastRainChance = 0 ;
    description.str("");

    CURL *curl;
    CURLcode res;
    curl_global_init(CURL_GLOBAL_DEFAULT) ;
    curl = curl_easy_init();
    if (curl) {
        // sendUrlRequest( curl, current_url.c_str(), WriteMemoryCallbackCurrent ) ;

        time_t now = time( nullptr ) ;
        constexpr int NUM_DAYS = 1;
        long yesterday = now - (NUM_DAYS * 86400);
        // We'll look back N days of history for rainfall
        for( int i=NUM_DAYS ; i>0 ; i-- ) {
            yesterday += 86400 ;
            std::ostringstream ss ;
            ss << history_url << "&lat=" << lat << "&lon=" << lon << "&dt=" << yesterday ;
            sendUrlRequest( curl, ss.str().c_str(), WriteMemoryCallbackHistory ) ;

        }

        std::ostringstream ss ;
        ss << forecast_url << "&lat=" << lat << "&lon=" << lon ;
        sendUrlRequest( curl, ss.str().c_str(), WriteMemoryCallbackForecast ) ;

        curl_easy_cleanup(curl);
    }

    curl_global_cleanup() ;
}


void Weather::sendUrlRequest( void *x, const char *url, size_t (*write_callback)(char *ptr, size_t size, size_t nmemb, void *userdata) ) {
    CURL *curl = (CURL*)x ;
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback );
    curl_easy_setopt(curl, CURLOPT_URL, url ) ;
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, this ) ;
    CURLcode res = curl_easy_perform(curl);
    /* Check for errors */
    if (res != CURLE_OK)
        throw std::string( curl_easy_strerror(res) ) ;
}

double Weather::getRecentRainfall() const {
    return totalRainFall;
}

double Weather::getForecastRainChance() const {
    return forecastRainChance;
}

const std::string Weather::getDescription() const {
    return description.str();
}


