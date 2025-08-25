
#include "Weather.hpp"
#include "Logger.hpp"

#include <cmath>
#include <curl/curl.h>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <stack>
#include <vector>

#include <jsoncpp/json/json.h>

using namespace std;

Weather::Weather(string zip, int pastHours, int forecastHours) {
    responseBuffer = new char[64000];
    char *api_key = getenv("WEATHER_API_KEY");
    if (api_key == nullptr) {
        if (g_logger) {
            LOG_FATAL("Missing WEATHER_API_KEY environment variable");
        } else {
            cerr << "Missing WEATHER_API_KEY environment variable" << endl;
        }
        exit(-1);
    }
    string Server("https://api.weatherapi.com/v1/");

    // round up number of days to whole full day
    daysOfHistory = (pastHours + 23) / 24;
    const auto forecastDays = (forecastHours + 23) / 24;
    ostringstream buf;
    buf << Server << "forecast.json?key=" << api_key << "&q=" << zip << "&days=" << forecastDays << "&aqi=no&alerts=no" ;

    location_url = Server + "current.json?key=" + api_key + "&q=" + zip + "&aqi=no";
    history_url = Server + "history.json?key=" + api_key + "&q=" + zip ;
    forecast_url = buf.str();
}

size_t
WriteMemoryCallback(char *contents, size_t size, size_t nmemb, void *userp) {
    Weather *self = (Weather *)userp;
    strncpy(self->responseBufferPosition, contents, nmemb );
    self->responseBufferPosition += size * nmemb;
    return size * nmemb;
}

void Weather::init() {
    CURL *curl;
    CURLcode res;
    curl_global_init(CURL_GLOBAL_DEFAULT);
    curl = curl_easy_init();
    if (curl) {
        sendUrlRequest(curl, location_url.c_str(), WriteMemoryCallback);
        parseLocation(responseBuffer, responseBufferPosition-responseBuffer);
        curl_easy_cleanup(curl);
    }
    curl_global_cleanup();
}

void Weather::parseLocation(char *contents, size_t sz) {
    char *p = (char *)contents;
    Json::CharReaderBuilder builder;
    Json::Value root;
    auto reader = builder.newCharReader();
    bool parsingSuccessful = reader->parse(p, p + sz, &root, nullptr);

    if (!parsingSuccessful) {
        if (g_logger) {
            LOG_ERROR("Failed to parse location JSON: {}", p);
        } else {
            cerr << "Failed to parse json read\n" << p << endl;
        }
    }
    lon = root["location"]["lon"].asDouble();
    lat = root["location"]["lat"].asDouble();
    // cout << lon << "," << lat << endl;
}

void Weather::parseHistory(char *contents, size_t sz) {
    // cout << contents << endl;
    char *p = (char *)contents;
    Json::CharReaderBuilder builder;
    Json::Value root;
    auto reader = builder.newCharReader();
    bool parsingSuccessful = reader->parse(p, p + sz, &root, nullptr);
    if (!parsingSuccessful) {
        if (g_logger) {
            LOG_ERROR("Failed to parse weather history JSON: {}", p);
        } else {
            cerr << "Failed to parse json ...\n" << p << endl;
        }
    }
    totalRecentRainfall += root["forecast"]["forecastday"][0]["day"]["totalprecip_mm"].asDouble();
}

void Weather::parseForecast(char *contents, size_t sz) {
    // cout << contents << endl;
    char *p = contents;
    Json::CharReaderBuilder builder;
    Json::Value root;
    auto reader = builder.newCharReader();
    bool parsingSuccessful = reader->parse(p, p + sz, &root, nullptr);
    if (!parsingSuccessful) {
        if (g_logger) {
            LOG_ERROR("Failed to parse weather history JSON: {}", p);
        } else {
            cerr << "Failed to parse json ...\n" << p << endl;
        }
    }
    
    forecastRainfall = root["forecast"]["forecastday"][0]["day"]["totalprecip_mm"].asDouble()
                       + root["forecast"]["forecastday"][1]["day"]["totalprecip_mm"].asDouble();

    double degc = root["current"]["temp_c"].asDouble();
    double degf = degc * 9.0 / 5.0 + 32;
    double humidity = root["current"]["humidity"].asDouble();

    description << setprecision(1) << fixed << degc << "°C/" << degf << "°F "
                << humidity << "% "
                << root["current"]["condition"]["text"].asString();

}

void Weather::read() {
    // setup accumulators
    forecastRainfall = 0;
    totalRecentRainfall = 0;
    description.str("");

    CURL *curl;
    CURLcode res;
    curl_global_init(CURL_GLOBAL_DEFAULT);
    curl = curl_easy_init();
    if (curl) {
        auto now = time(nullptr);   // get time now
        // We'll look back N days of history for rainfall
        for (int i = 0 ; i<daysOfHistory ; i++) {
            auto ts = localtime(&now);
            ts->tm_mday -= i;
            mktime(ts); /* Normalise ts */
            char dateBuf[128] ;
            strftime(dateBuf, sizeof(dateBuf), "%Y-%m-%d", ts);

            ostringstream ss;
            ss << history_url << "&dt=" << dateBuf;
            sendUrlRequest(curl, ss.str().c_str(), WriteMemoryCallback);
            parseHistory(responseBuffer, responseBufferPosition-responseBuffer);
        }

        sendUrlRequest(curl, forecast_url.c_str(), WriteMemoryCallback);
        parseForecast(responseBuffer, responseBufferPosition-responseBuffer);

        curl_easy_cleanup(curl);
    }

    curl_global_cleanup();
}

void Weather::sendUrlRequest(void *x, const char *url, size_t (*write_callback)(char *ptr, size_t size, size_t nmemb, void *userdata)) {
    responseBufferPosition = responseBuffer;
    CURL *curl = (CURL *)x;
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, this);
    CURLcode res = curl_easy_perform(curl);
    /* Check for errors */
    if (res != CURLE_OK)
        throw string(curl_easy_strerror(res));
}

double Weather::getRecentRainfall() const {
    return totalRecentRainfall;
}

double Weather::getForecastRainfall() const {
    return forecastRainfall;
}

const string Weather::getDescription() const {
    return description.str();
}
