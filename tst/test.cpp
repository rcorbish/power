
#include <string.h>
#include <iostream>
#include <math.h>

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <netdb.h> 
#include <sstream>

#define TEST_FRIENDS friend int main( int , char ** ) ; 

#include "Connection.hpp"
#include "Weather.hpp"
#include "JsonParser.hpp"

#define TEST_JSON1 "{\"coord\":{\"lon\":-81.4915,\"lat\":31.15},\"weather\":[{\"id\":800,\"main\":\"Clear\",\"description\":\"clear sky\",\"icon\":\"01d\"}],\"base\":\"stations\",\"main\":{\"temp\":284.53,\"feels_like\":283.34,\"temp_min\":283.15,\"temp_max\":286.15,\"pressure\":1020,\"humidity\":62},\"visibility\":10000,\"wind\":{\"speed\":3.09,\"deg\":330},\"clouds\":{\"all\":1},\"dt\":1620474176,\"sys\":{\"type\":1,\"id\":5922,\"country\":\"US\",\"sunrise\":1620470109,\"sunset\":1620518984},\"timezone\":-14400,\"id\":4184845,\"name\":\"Brunswick\",\"cod\":200}"
#define TEST_JSON2 "{\"lat\":40.7004,\"lon\":-74.4023,\"timezone\":\"America/New_York\",\"timezone_offset\":-14400,\"current\":{\"dt\":1620560651,\"sunrise\":1620553587,\"sunset\":1620604899,\"temp\":7.76,\"feels_like\":4.61,\"pressure\":1013,\"humidity\":71,\"dew_point\":2.84,\"uvi\":7.21,\"clouds\":1,\"visibility\":16093,\"wind_speed\":2.31,\"wind_deg\":280,\"wind_gust\":4.56,\"weather\":[{\"id\":800,\"main\":\"Clear\",\"description\":\"clear sky\",\"icon\":\"01d\"}]},\"hourly\":[{\"dt\":1620518400,\"temp\":9.31,\"feels_like\":7.23,\"pressure\":1009,\"humidity\":87,\"dew_point\":7.26,\"clouds\":40,\"visibility\":16093,\"wind_speed\":2.06,\"wind_deg\":330,\"weather\":[{\"id\":802,\"main\":\"Clouds\",\"description\":\"scattered clouds\",\"icon\":\"03d\"}]},{\"dt\":1620522000,\"temp\":8.02,\"feels_like\":5.41,\"pressure\":1014,\"humidity\":100,\"dew_point\":8.02,\"clouds\":75,\"visibility\":16093,\"wind_speed\":3.07,\"wind_deg\":313,\"wind_gust\":8.24,\"weather\":[{\"id\":803,\"main\":\"Clouds\",\"description\":\"broken clouds\",\"icon\":\"04n\"}]},{\"dt\":1620525600,\"temp\":7.19,\"feels_like\":5.46,\"pressure\":1010,\"humidity\":100,\"dew_point\":7.19,\"clouds\":1,\"visibility\":16093,\"wind_speed\":1.54,\"wind_deg\":280,\"weather\":[{\"id\":800,\"main\":\"Clear\",\"description\":\"clear sky\",\"icon\":\"01n\"}]},{\"dt\":1620529200,\"temp\":6.49,\"feels_like\":3.24,\"pressure\":1010,\"humidity\":100,\"dew_point\":6.49,\"clouds\":1,\"visibility\":8047,\"wind_speed\":3.49,\"wind_deg\":309,\"wind_gust\":10.02,\"weather\":[{\"id\":701,\"main\":\"Mist\",\"description\":\"mist\",\"icon\":\"50n\"}]},{\"dt\":1620532800,\"temp\":6.35,\"feels_like\":3.64,\"pressure\":1011,\"humidity\":100,\"dew_point\":6.35,\"clouds\":1,\"visibility\":16093,\"wind_speed\":2.67,\"wind_deg\":295,\"wind_gust\":8.8,\"weather\":[{\"id\":800,\"main\":\"Clear\",\"description\":\"clear sky\",\"icon\":\"01n\"}]},{\"dt\":1620536400,\"temp\":5.75,\"feels_like\":3.06,\"pressure\":1011,\"humidity\":100,\"dew_point\":5.75,\"clouds\":1,\"visibility\":16093,\"wind_speed\":2.46,\"wind_deg\":289,\"wind_gust\":8.72,\"weather\":[{\"id\":800,\"main\":\"Clear\",\"description\":\"clear sky\",\"icon\":\"01n\"}]},{\"dt\":1620540000,\"temp\":5.26,\"feels_like\":1.92,\"pressure\":1011,\"humidity\":100,\"dew_point\":5.26,\"clouds\":1,\"visibility\":16093,\"wind_speed\":3.24,\"wind_deg\":291,\"wind_gust\":9.94,\"weather\":[{\"id\":800,\"main\":\"Clear\",\"description\":\"clear sky\",\"icon\":\"01n\"}]},{\"dt\":1620543600,\"temp\":5.28,\"feels_like\":2.2,\"pressure\":1011,\"humidity\":100,\"dew_point\":5.28,\"clouds\":1,\"visibility\":4828,\"wind_speed\":2.87,\"wind_deg\":299,\"wind_gust\":9.63,\"weather\":[{\"id\":701,\"main\":\"Mist\",\"description\":\"mist\",\"icon\":\"50n\"}]},{\"dt\":1620547200,\"temp\":5.42,\"feels_like\":1.87,\"pressure\":1011,\"humidity\":81,\"dew_point\":2.43,\"clouds\":1,\"visibility\":16093,\"wind_speed\":2.78,\"wind_deg\":308,\"wind_gust\":9.99,\"weather\":[{\"id\":800,\"main\":\"Clear\",\"description\":\"clear sky\",\"icon\":\"01n\"}]},{\"dt\":1620550800,\"temp\":4.66,\"feels_like\":1.75,\"pressure\":1011,\"humidity\":100,\"dew_point\":4.66,\"clouds\":1,\"visibility\":16093,\"wind_speed\":2.46,\"wind_deg\":297,\"wind_gust\":8.38,\"weather\":[{\"id\":800,\"main\":\"Clear\",\"description\":\"clear sky\",\"icon\":\"01n\"}]},{\"dt\":1620554400,\"temp\":4.63,\"feels_like\":1.87,\"pressure\":1012,\"humidity\":100,\"dew_point\":4.63,\"clouds\":1,\"visibility\":11265,\"wind_speed\":2.23,\"wind_deg\":277,\"wind_gust\":6.47,\"weather\":[{\"id\":800,\"main\":\"Clear\",\"description\":\"clear sky\",\"icon\":\"01d\"}]},{\"dt\":1620558000,\"temp\":6.34,\"feels_like\":3.43,\"pressure\":1013,\"humidity\":79,\"dew_point\":2.97,\"clouds\":40,\"visibility\":16093,\"wind_speed\":2.01,\"wind_deg\":279,\"wind_gust\":5.82,\"weather\":[{\"id\":802,\"main\":\"Clouds\",\"description\":\"scattered clouds\",\"icon\":\"03d\"}]},{\"dt\":1620561600,\"temp\":8.32,\"feels_like\":5.81,\"pressure\":1013,\"humidity\":71,\"dew_point\":3.38,\"clouds\":1,\"visibility\":16093,\"wind_speed\":1.54,\"wind_deg\":290,\"weather\":[{\"id\":800,\"main\":\"Clear\",\"description\":\"clear sky\",\"icon\":\"01d\"}]},{\"dt\":1620565200,\"temp\":11.05,\"feels_like\":7.39,\"pressure\":1013,\"humidity\":66,\"dew_point\":4.95,\"clouds\":1,\"visibility\":16093,\"wind_speed\":3.6,\"wind_deg\":300,\"weather\":[{\"id\":800,\"main\":\"Clear\",\"description\":\"clear sky\",\"icon\":\"01d\"}]},{\"dt\":1620568800,\"temp\":13.48,\"feels_like\":10.27,\"pressure\":1013,\"humidity\":58,\"dew_point\":5.39,\"clouds\":75,\"visibility\":16093,\"wind_speed\":3.09,\"wind_deg\":300,\"weather\":[{\"id\":500,\"main\":\"Rain\",\"description\":\"light rain\",\"icon\":\"10d\"}],\"rain\":{\"1h\":0.25}},{\"dt\":1620572400,\"temp\":15.05,\"feels_like\":11.23,\"pressure\":1013,\"humidity\":48,\"dew_point\":4.14,\"clouds\":75,\"visibility\":16093,\"wind_speed\":3.6,\"wind_deg\":280,\"weather\":[{\"id\":803,\"main\":\"Clouds\",\"description\":\"broken clouds\",\"icon\":\"04d\"}]},{\"dt\":1620576000,\"temp\":15.42,\"feels_like\":10.79,\"pressure\":1012,\"humidity\":39,\"dew_point\":1.55,\"clouds\":75,\"visibility\":16093,\"wind_speed\":4.12,\"wind_deg\":270,\"weather\":[{\"id\":803,\"main\":\"Clouds\",\"description\":\"broken clouds\",\"icon\":\"04d\"}]},{\"dt\":1620579600,\"temp\":16.14,\"feels_like\":12.1,\"pressure\":1016,\"humidity\":41,\"dew_point\":2.89,\"clouds\":40,\"visibility\":16093,\"wind_speed\":3.6,\"wind_deg\":270,\"wind_gust\":7.2,\"weather\":[{\"id\":802,\"main\":\"Clouds\",\"description\":\"scattered clouds\",\"icon\":\"03d\"}]},{\"dt\":1620583200,\"temp\":15.83,\"feels_like\":11.43,\"pressure\":1015,\"humidity\":48,\"dew_point\":4.85,\"clouds\":40,\"visibility\":16093,\"wind_speed\":4.63,\"wind_deg\":250,\"wind_gust\":9.77,\"weather\":[{\"id\":802,\"main\":\"Clouds\",\"description\":\"scattered clouds\",\"icon\":\"03d\"}]},{\"dt\":1620586800,\"temp\":12.61,\"feels_like\":8.35,\"pressure\":1009,\"humidity\":47,\"dew_point\":1.61,\"clouds\":90,\"visibility\":16093,\"wind_speed\":3.6,\"wind_deg\":260,\"wind_gust\":9.26,\"weather\":[{\"id\":804,\"main\":\"Clouds\",\"description\":\"overcast clouds\",\"icon\":\"04d\"}]},{\"dt\":1620590400,\"temp\":10.1,\"feels_like\":6.2,\"pressure\":1011,\"humidity\":82,\"dew_point\":7.17,\"clouds\":90,\"visibility\":11265,\"wind_speed\":4.63,\"wind_deg\":250,\"wind_gust\":6.69,\"weather\":[{\"id\":501,\"main\":\"Rain\",\"description\":\"moderate rain\",\"icon\":\"10d\"},{\"id\":701,\"main\":\"Mist\",\"description\":\"mist\",\"icon\":\"50d\"}],\"rain\":{\"1h\":0.76}},{\"dt\":1620594000,\"temp\":9.33,\"feels_like\":7.49,\"pressure\":1009,\"humidity\":93,\"dew_point\":8.26,\"clouds\":90,\"visibility\":12875,\"wind_speed\":2.06,\"wind_deg\":250,\"weather\":[{\"id\":500,\"main\":\"Rain\",\"description\":\"light rain\",\"icon\":\"10d\"}],\"rain\":{\"1h\":0.34}},{\"dt\":1620597600,\"temp\":9.15,\"feels_like\":7.26,\"pressure\":1009,\"humidity\":93,\"dew_point\":8.08,\"clouds\":90,\"visibility\":12875,\"wind_speed\":2.06,\"wind_deg\":250,\"weather\":[{\"id\":501,\"main\":\"Rain\",\"description\":\"moderate rain\",\"icon\":\"10d\"}],\"rain\":{\"1h\":0.83}},{\"dt\":1620601200,\"temp\":8.6,\"feels_like\":6.57,\"pressure\":1014,\"humidity\":100,\"dew_point\":8.6,\"clouds\":90,\"visibility\":12875,\"wind_speed\":2.44,\"wind_deg\":64,\"wind_gust\":4.17,\"weather\":[{\"id\":501,\"main\":\"Rain\",\"description\":\"moderate rain\",\"icon\":\"10d\"}],\"rain\":{\"1h\":1.99}}]}\""

int main( int argc, char **argv ) {

    try {
        // Weather weather( "31525" ) ;

        char s2[sizeof(TEST_JSON2)+1] ;
        strncpy( s2, TEST_JSON2, sizeof(TEST_JSON2) ) ;


        std::set<std::string> keys2 ;
        for( int i=0 ; i<24 ; i++ ) {
            std::ostringstream ss ;
            ss << "hourly[" << i << "].rain.1h" ;
            keys2.emplace( ss.str() ) ;
        }
    
        JsonParser parser2( s2, keys2 ) ;

        double totalRainFall = 0 ;
        for( auto &i : keys2 ) {
            double mmRainfall = parser2.getNumber( i ) ;
            if( !std::isnan(mmRainfall) ) {
                totalRainFall += mmRainfall ;
            }
        }
        if( std::abs( totalRainFall - 4.17 ) < 0.00001 ) {
            std::cout << "Good parse - Rainfall" << std::endl ;
        }

        char s1[sizeof(TEST_JSON1)+1] ;
        strncpy( s1, TEST_JSON1, sizeof(TEST_JSON1) ) ;

        std::set<std::string> keys ;
        keys.emplace( "weather[0].main" ) ;
        JsonParser parser( s1, keys ) ;
        std::string s = parser.getText( "weather[0].main" ) ;
        if( "Clear"==s ) {
            std::cout << "Good parse - Simple" << std::endl ;
        }


    } catch( std::string err ) {
        std::cerr << err << std::endl ;
    }
    
	return 0 ;
}

