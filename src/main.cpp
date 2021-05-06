
#include <string.h>
#include <iostream>
#include <math.h>

#include <unistd.h>
#include <stdio.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h> 

#include "Connection.hpp"


/**
 * main
 * 
 * get the initial board state
 * solve the problem
 * if no solution :(
 * 
 */
int main( int argc, char **argv ) {


    Connection con ; 
    con.discover() ;

    sleep(2) ;
    bool on = con.get( "ECO-780C4AA9" ) ;
    std::cout << "Device is " << (on?"ON":"OFF") << std::endl ;
    sleep(2) ;
        con.set( "ECO-780C4AA9", true ) ;

    // on = false ;
    // while( !on ) {
    //     std::cout << "." ;
    //     con.set( "ECO-780C4AA9", true ) ;
    //     on = con.get( "ECO-780C4AA9" ) ;
    // }
    // std::cout << std::endl ;

    sleep(2) ;
    // while( on ) {
    //     std::cout << "." ;
    //     con.set( "ECO-780C4AA9", false ) ;
    //     on = con.get( "ECO-780C4AA9" ) ;
    // }
    // std::cout << std::endl ;

        con.set( "ECO-780C4AA9", false ) ;
	return 0 ;
}

