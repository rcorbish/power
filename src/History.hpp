
#pragma once

#include <iostream>
#include <list>


constexpr const char *HistoryLogName = "sprinkler.dat" ;


class HistoryEntry {
    friend std::ostream & operator<<( std::ostream &os, const HistoryEntry &con ) ;

    public:
        const time_t eventTimes ;
        const float rainFall ;
        const float forecastRain ;
        const int sprinkleTime ;

        HistoryEntry( float rainFall ,
                      float forecastRain ,
                      int sprinkleTime ) :
            rainFall( rainFall ), forecastRain( forecastRain ), 
            sprinkleTime( sprinkleTime ), eventTimes( time( nullptr ) ) {
        }

        HistoryEntry( time_t eventTimes, 
                      float rainFall ,
                      float forecastRain ,
                      int sprinkleTime ) :
            rainFall( rainFall ), forecastRain( forecastRain ), 
            sprinkleTime( sprinkleTime ), eventTimes( eventTimes ) {
        }
} ;

void appendHistory( const HistoryEntry &entry ) ;
std::list<HistoryEntry> loadHistory( const char *historyFileName ) ;

