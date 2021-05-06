

#pragma once

#include "string"
#include <ctime>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <iostream>
// #include <netinet/in.h>
// #include <arpa/inet.h>
#include <netdb.h>
#include "iomanip"
#include <pthread.h>
#include <map>

#include "Device.hpp"

// typedef struct {
//     // L6s32s32s32sHHBBLl64s64sH10s12s16s16s16sLLLLH30s18s18sL
//     uint32_t skip;
//     char version[6];
//     char id[32];
//     char name[32];
//     char shortid[32];

//     uint8_t time[14];
//     char ssid[64];
//     char password[64];
//     uint16_t data1 ;
//     char region[10];
//     char areacode[12];
//     char ipa[16];
//     char ipb[16];
//     char ipc[16];
//     uint32_t data2 ;
//     uint32_t data3 ;
//     uint32_t data4 ;
//     uint32_t data5 ;
//     uint16_t data6 ;
//     char pwd[30];
//     char mac[18];
//     char host[18];
//     uint32_t data7 ;
// } MSG408;



class Connection {

  friend std::ostream & operator<<( std::ostream &os, const Connection &con ) ;

  private:
    int localSocket;
    uint32_t sequence ;
    pthread_t threadId ;

    std::map<std::string, Device> devices ;

    static void * receiverThread( void *self ) {
        ((Connection*)self)->recvLoop() ;
        return nullptr ;
    }

    void recvLoop() {
        // std::cout << "Listening" << std::endl ;
        MSG408 deviceInfo ;
        for( ; ; ) {
            int n = recvfrom(localSocket, &deviceInfo, sizeof(deviceInfo), 0, nullptr, nullptr ) ;
            if( n < 0 ) {
                perror( "recvfrom" ) ;
                abort() ;
            }
            // std::cout << "Received " << std::dec << n << " bytes" << std::endl ;
            std::string deviceName( deviceInfo.id ) ;

            if( devices.find( deviceName ) == devices.end() ) {
                std::cout << "Adding new device " << deviceName << std::endl ;
                devices.emplace( deviceName, deviceInfo ) ;
            }
        }
    }  

  protected:
    void sendMsg( const void *data, size_t length) {
        sequence++ ;

        struct sockaddr_in remoteAddress ;
        memset( &remoteAddress, 0, sizeof(remoteAddress) ) ;
        // inet_pton(AF_INET, "192.168.1.153", &remoteAddress.sin_addr ) ; 
        inet_pton(AF_INET, "255.255.255.255", &remoteAddress.sin_addr ) ; 
        remoteAddress.sin_port = htons(25) ;

        size_t sz = sendto(localSocket, data, length,
                        0, 
                        (const struct sockaddr *)&remoteAddress,
                        sizeof(remoteAddress));
        if (sz != length ) {
            perror("sendto");
            exit(EXIT_FAILURE);
        }
    }
    
  public:
    Connection() {
        sequence = 0x55 ;

        // Prepare local socket from which we'll send and listen
        localSocket = ::socket(AF_INET, SOCK_DGRAM, 0);
        if (localSocket == 0) {
            perror("socket failed");
            exit(EXIT_FAILURE);
        }

        int opt = 1 ;
        if (::setsockopt(localSocket, SOL_SOCKET, SO_BROADCAST, &opt, sizeof(opt))) {
            perror("setsockopt");
            exit(EXIT_FAILURE);
        }

        struct sockaddr_in address ;                
        memset( &address, 0, sizeof(address) ) ;
        // Bind the local socket to listen on any address
        address.sin_family = AF_INET;
        address.sin_addr.s_addr = INADDR_ANY;
        address.sin_port = 0 ; //htons(PORT);

        if (::bind(localSocket, (struct sockaddr *)&address, sizeof(address)) < 0) {
            perror("bind failed");
            exit(EXIT_FAILURE);
        }

        int err = pthread_create( &threadId, nullptr, &receiverThread, this ) ;
    }

    void discover() {
        uint8_t discover[128];

        memset(discover, 0, sizeof(discover));
        uint8_t *p = (discover + 23);
        *p++ = 0x00;
        *p++ = 0xe0;
        *p++ = 0x07;
        *p++ = 0x0b;

        *p++ = 0x11;
        *p++ = 0xf7;
        *p++ = 0x9d;
        *p++ = 0x00;

        sendMsg( discover, sizeof(discover));
    }

    bool get( const std::string &deviceName ) {
        auto it = devices.find( deviceName ) ;
        if( it == devices.end() ) {
            std::cerr << "Device " << deviceName << " not connected" << std::endl ;
        } else {
            return it->second.get() ;
        }
        return false ;
    }

    void set( const std::string &deviceName, const bool on ) {
        auto it = devices.find( deviceName ) ;
        if( it == devices.end() ) {
            std::cerr << "Device " << deviceName << " not connected" << std::endl ;
        } else {
            it->second.set( on ) ;
        }
    }

};

std::ostream & operator<<( std::ostream &os, const Connection &con ) {
    os << "Device not initialized" ;
    return os ;
}
