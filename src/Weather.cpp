
#include "Weather.hpp"

#include <curl/curl.h>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <stack>
#include <cmath>
#include <vector>

#include <jsoncpp/json/json.h>


using namespace std;

Weather::Weather( string zip, long pastHours, long forecastHours ) {
    char *api_key = getenv("WEATHER_API_KEY");
    if (api_key == nullptr) {
        cerr <<"Missing WEATHER_API_KEY environment variable" << endl;
        exit(-1);
    }
    string Server( "https://api.openweathermap.org/" ) ;

    location_url = Server + "geo/1.0/zip?zip=" + zip + "&APPID=" + api_key;
    history_url = Server + "data/2.5/onecall/timemachine?units=metric&appid=" + api_key;
    forecast_url = Server + "data/2.5/onecall?units=metric&exclude=current,minutely,daily,alerts&APPID=" + api_key;

    rainSince = time( nullptr ) - ( pastHours * 3600 ) ;
    hoursForecast = forecastHours ;
}




size_t
WriteMemoryCallbackLocation(char *contents, size_t size, size_t nmemb, void *userp) {
    string json( (char*)contents);
    Weather * self = (Weather *)userp;
    self->parseLocation( (char*)contents, size * nmemb );
    return size * nmemb;
}

size_t
WriteMemoryCallbackHistory(char *contents, size_t size, size_t nmemb, void *userp) {
    string json( (char*)contents);
    Weather * self = (Weather *)userp;
    self->parseHistory( (char*)contents, size * nmemb);
    return size * nmemb;
}

size_t
WriteMemoryCallbackForecast(char *contents, size_t size, size_t nmemb, void *userp) {
    string json( (char*)contents);
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
        sendUrlRequest( curl, location_url.c_str(), WriteMemoryCallbackLocation ) ;
        curl_easy_cleanup(curl);
    }
    curl_global_cleanup() ;
}

void Weather::parseLocation( char *contents, size_t sz ) {
    stack<string> tags;
    char *p = (char*)contents;
    Json::CharReaderBuilder builder;
    Json::Value root;
    auto reader = builder.newCharReader();
    bool parsingSuccessful = reader->parse( p, p+sz, &root, nullptr );
    
    if( !parsingSuccessful ) {
        cerr << "Failed to parse json read\n" << p << endl;
    }
    lon = root["lon"].asDouble();
    lat = root["lat"].asDouble();
    cout << lon << "," << lat << endl ;
}


void Weather::parseHistory( char *contents, size_t sz ) {
    stack<string> tags ;
    char *p = (char*)contents ;
    Json::CharReaderBuilder builder;
    Json::Value root;
    auto reader = builder.newCharReader();
    bool parsingSuccessful = reader->parse( p, p+sz, &root, nullptr );
    if( !parsingSuccessful ) {
        cerr << "Failed to parse json read\n" << p << endl;
    }
    for( int i=0 ; i<24 ; i++ ) {
        totalRainFall += root["hourly"][i]["rain"]["1h"].asDouble();
    }

    double degc = root["current"]["temp"].asDouble();
    double degf = degc * 9.0 / 5.0 + 32 ;    

    description << setprecision(1) << fixed << degc << "°C/" << degf << "°F " 
                << root["current"]["humidity"].asDouble() << "% "
                << root["current"]["weather"][0]["description"].asString()
                << root["current"]["weather"][1]["description"].asString()
                << root["current"]["weather"][2]["description"].asString();                
    
}


void Weather::parseForecast( char *contents, size_t sz ) {
    stack<string> tags ;
    char *p = (char*)contents ;
    Json::CharReaderBuilder builder;
    Json::Value root;
    auto reader = builder.newCharReader();
    bool parsingSuccessful = reader->parse( p, p+sz, &root, nullptr );
    if( !parsingSuccessful ) {
        cerr << "Failed to parse json read\n" << p << endl;
    }
    for( int i=0 ; i<24 ; i++ ) {
        forecastRainChance += root["hourly"][i]["rain"]["1h"].asDouble();
    }   
}


void Weather::read() {
    // setup accumulators
    forecastRainChance = 0;
    totalRainFall = 0;
    description.str("");

    CURL *curl;
    CURLcode res;
    curl_global_init(CURL_GLOBAL_DEFAULT) ;
    curl = curl_easy_init();
    if (curl) {
        time_t now = time( nullptr ) ;
        constexpr int NUM_DAYS = 1;
        long yesterday = now - (NUM_DAYS * 86400);
        // We'll look back N days of history for rainfall
        for( int i=NUM_DAYS ; i>0 ; i-- ) {
            ostringstream ss ;
            ss << history_url << "&lat=" << lat << "&lon=" << lon << "&dt=" << yesterday ;
            sendUrlRequest( curl, ss.str().c_str(), WriteMemoryCallbackHistory ) ;
            yesterday += 86400 ;
        }

        ostringstream ss ;
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
        throw string( curl_easy_strerror(res) ) ;
}

double Weather::getRecentRainfall() const {
    return totalRainFall;
}

double Weather::getForecastRainChance() const {
    return forecastRainChance;
}

const string Weather::getDescription() const {
    return description.str();
}


