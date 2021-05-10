
#pragma once

#include <stack>
#include <map>
#include <set>

class JsonParser {
    private:
        std::set<std::string> keysToFind ;
        std::map<std::string,std::string> parsed ;
        std::stack<std::string> tags ;

    protected:
        char * parseArray( char *p ) ;
        char * parseValue( char *p ) ;
        char * parseObject( char *p ) ;

    public:
        JsonParser( char * json, std::set<std::string> keysToFind ) ;

        std::string getText( std::string key ) ;
        double getNumber( std::string key ) ;
} ;

