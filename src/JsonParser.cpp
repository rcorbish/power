
#include "JsonParser.hpp"

#include <iostream>
#include <sstream>
#include <cmath>

#define DEBUG_PRINT

JsonParser::JsonParser( char *json, std::set<std::string> _keysToFind ) :
            keysToFind( _keysToFind ) {

    parseObject( json ) ;
}

std::string JsonParser::getText( std::string key ) {
    auto search = parsed.find( key ) ;
    if( search == parsed.end() ) {
        return "" ;
    }
    return search->second ;
}

double JsonParser::getNumber( std::string key ) {
    auto search = parsed.find( key ) ;
    if( search == parsed.end() ) {
        return std::nan( "" ) ;
    }
    return ::atof( search->second.c_str() ) ;
}



char * JsonParser::parseArray( char *p ) {
    if( *p++ != '[' ) {
        throw std::string( "Malformed JSON, arrays must start with '['." ) ;
    }

    std::string arrayElement = tags.top() ;

    int item = 0 ;
    while( *p != ']' ) {
        std::ostringstream indexedArrayElement;
        indexedArrayElement << arrayElement << '[' << item << ']' ;
        item++ ;
        tags.push( indexedArrayElement.str() ) ;
        p = parseValue( p ) ;
        while( isspace( *p ) ) p++ ;      // skip whitespace
        if( *p == ',' ) {                 // skip commas between values
            p++ ;            
            while( isspace( *p ) ) p++ ;  // skip whitespace
        }
        tags.pop() ;                      // dummy tag includes index
    }

    return ++p ;
}

char * JsonParser::parseValue( char *p ) {
    while( isspace( *p ) ) p++ ;     // skip whitespace
    
    if( *p == '"' ) {   // value is a text

        char *textStart = ++p ;

        // skip to end of text - ignore escaped quotes
        while( *p != '"' ) if( *p++ == '\\' && *p=='"' ) p++ ;

        *p++ = 0 ;
        auto match = keysToFind.find( tags.top() ) ;
        if( match != keysToFind.end() ) {
            parsed[tags.top()] = textStart ;
        }
#ifdef DEBUG_PRINT
        std::cout << tags.top() << " = " << textStart << std::endl ;
#endif
    } else if ( *p == '{' ) {            // value is an object
        p = parseObject( p ) ;
    } else if ( *p == '[' ) {            // value is an array
        p = parseArray( p ) ;
    } else {
        char *numberStart = p ;
        if( *p == '-' || *p == '+' ) p++ ;   // skip number sign

        while( (*p>='0' && *p<='9') || *p == '.' ) p++ ;
        auto match = keysToFind.find( tags.top() ) ;
        if( match != keysToFind.end() ) {
            char c = *p ;
            *p = 0 ;
            parsed[tags.top()] = numberStart ;
            *p = c ;
        }
#ifdef DEBUG_PRINT
        char c = *p ;
        *p = 0 ;
        std::cout << tags.top() << " = " << numberStart << std::endl ;
        *p = c ;
#endif        
    }
    return p ;
}

char * JsonParser::parseObject( char *p ) {
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

        p = parseValue( p ) ;
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


