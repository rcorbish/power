
#pragma once

#include <string>
#include <stack>
#include <map>
#include <set>

enum JsonType {
    NONE,
    STRING,
    NUMBER
};

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
        bool has( std::string key ) ;
        std::string getText( std::string key ) ;
        double getNumber( std::string key ) ;
} ;

