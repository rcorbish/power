
#include "History.hpp"

#include <fstream>

void appendHistory(const HistoryEntry &entry) {

    std::fstream logFile;
    logFile.open(HistoryLogName, std::ios::out | std::fstream::app);

    if (logFile.is_open()) {
        logFile << entry;
        logFile.close();
    }
}

std::ostream &operator<<(std::ostream &os, const HistoryEntry &e) {
    os << e.eventTimes << ' ' << e.rainFall << ' '
       << e.forecastRain << ' ' << e.sprinkleTime << std::endl;
    return os;
}

constexpr int HistoryLength = 15 ;

std::list<HistoryEntry> loadHistory( const char *historyLogName ) {

    std::list<HistoryEntry> rc;

    std::fstream logFile;
    logFile.open(historyLogName, std::ios::in);

    if (logFile.is_open() ) {
        for( ;; ) {
            time_t eventTimes;
            float rainFall;
            float forecastRain;
            int sprinkleTime;


            logFile >> eventTimes >> rainFall >> forecastRain >> sprinkleTime ;
            if( logFile.eof() ) break ; 
            if( rc.size() == HistoryLength ) {
                rc.pop_front() ;
            }
            rc.emplace_back(eventTimes, rainFall, forecastRain, sprinkleTime);
        } 
        logFile.close();
    }

    return rc ;
}
