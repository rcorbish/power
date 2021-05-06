

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
#include <arpa/inet.h>
#include <netdb.h>
#include "iomanip"
#include <pthread.h>


typedef struct {
    // L6s32s32s32sHHBBLl64s64sH10s12s16s16s16sLLLLH30s18s18sL
    uint32_t skip;
    char version[6];
    char id[32];
    char name[32];
    char shortid[32];

    uint8_t time[14];
    char ssid[64];
    char password[64];
    uint16_t data1 ;
    char region[10];
    char areacode[12];
    char ipa[16];
    char ipb[16];
    char ipc[16];
    uint32_t data2 ;
    uint32_t data3 ;
    uint32_t data4 ;
    uint32_t data5 ;
    uint16_t data6 ;
    char pwd[30];
    char mac[18];
    char host[18];
    uint32_t data7 ;
} MSG408;



class Device {

  friend std::ostream & operator<<( std::ostream &os, const Device &con ) ;

  private:
    const MSG408 deviceInfo ;

    int localSocket;
    struct sockaddr_in remoteAddress;
    uint32_t sequence ;
    pthread_t threadId ;
    uint8_t ready ;
    uint8_t getReady ;
    bool isOn ;

    static void * receiverThread( void *self ) {
        ((Device*)self)->recvLoop() ;
        return nullptr ;
    }

    void recvLoop() {
        // std::cout << "Listening" << std::endl ;
        uint8_t msg[1024] ;
        for( ; ; ) {
            int n = recvfrom(localSocket, msg, sizeof(msg), 0, nullptr, nullptr ) ;
            // std::cout << "Received " << std::dec << n << " bytes" << std::endl ;

            // uint8_t* p = (uint8_t*)msg ;
            // for( int i=0 ; i<n ; i++ ) {
            //     std::cout << std::setw(2) << std::hex << (int)(*p++) << " ";
            //     if( (i%16) == 15 ) {
            //         std::cout << std::endl ; 
            //     }
            // }
            // std::cout << std::endl ;        

            getReady = (n==130) ;
            isOn = msg[129] != 0 ;
        }
    }  

  protected:
    void sendMsg( const void *data, size_t length ) {
        sequence++ ;

        size_t sz = sendto(localSocket, data, length,
                        MSG_CONFIRM, 
                        (const struct sockaddr *)&remoteAddress,
                        sizeof(remoteAddress));
        if (sz != length ) {
            perror("sendto");
            exit(EXIT_FAILURE);
        }
    }
    
  public:
    Device( const MSG408 &deviceInfo ) : deviceInfo( deviceInfo) {
        sequence = 0x55 ;

        // Setup a remote address for the device
        memset( &remoteAddress, 0, sizeof(remoteAddress) ) ;        
        int n = inet_pton(AF_INET, deviceInfo.host, &remoteAddress.sin_addr ); 
        remoteAddress.sin_port = htons(80);
        
        // Prepare local socket from which we'll send and listen
        localSocket = ::socket(AF_INET, SOCK_DGRAM, 0);
        if (localSocket == 0) {
            perror("socket failed");
            exit(EXIT_FAILURE);
        }

        struct sockaddr_in address ;                
        memset( &address, 0, sizeof(address) ) ;

        address.sin_family = AF_INET;
        address.sin_addr.s_addr = INADDR_ANY;
        address.sin_port = 0 ; 

        if (::bind(localSocket, (struct sockaddr *)&address, sizeof(address)) < 0) {
            perror("bind failed");
            exit(EXIT_FAILURE);
        }

        int err = pthread_create( &threadId, nullptr, &receiverThread, this ) ;
    }


    bool get() {
        getReady = 0 ;
        uint8_t msg[128];

        uint8_t *p = msg ;
        *p++ = 0x17 ;    
        *p++ = 0x00 ;

        *p++ = 0x05 ;
        *p++ = 0x00 ;
        *p++ = 0x00 ;
        *p++ = 0x00 ;

        *p++ = ( sequence & 0xff00 ) >> 8 ;
        *p++ = sequence & 0x00ff ;

        *p++ = 0x00 ;
        *p++ = 0x00 ;
        
        memcpy( p, deviceInfo.version, sizeof(deviceInfo.version) ) ;
        p+=sizeof( deviceInfo.version ) ;
        memcpy( p, deviceInfo.id, sizeof(deviceInfo.id) ) ;
        p+=sizeof( deviceInfo.id ) ;
        memcpy( p, deviceInfo.name, sizeof(deviceInfo.name) ) ;
        p+=sizeof( deviceInfo.name ) ;
        memcpy( p, deviceInfo.shortid, sizeof(deviceInfo.shortid) ) ;
        p+=sizeof( deviceInfo.shortid ) ;

        *p++ = 0x00 ;
        *p++ = 0x00 ;
        *p++ = 0x00 ;
        *p++ = 0x00 ;

        long now = ( 1000L * time( nullptr ) ) & 0xffffffff ;
        *p++ = now & 0xff ;
        *p++ = ( now & 0x0000ff00 ) >> 8 ;
        *p++ = ( now & 0x00ff0000 ) >> 16 ; 
        *p++ = ( now & 0xff000000 ) >> 24 ;

        *p++ = 0x00;
        *p++ = 0x00;
        *p++ = 0x00;
        *p++ = 0x00;

        *p++ = 0xae ; 
        *p++ = 0x49 ; 
        *p++ = 0x52 ; 
        *p++ = 0x0d ; 

        sendMsg( msg, sizeof(msg) );       
        while( getReady == 0 ) {
        }
        return isOn ;
    }

    void set( bool switchOn ) {
        uint8_t msg[130];

        uint8_t *p = msg ;
        *p++ = 0x16 ;    
        *p++ = 0x00 ;

        *p++ = 0x05 ;
        *p++ = 0x00 ;
        *p++ = 0x00 ;
        *p++ = 0x00 ;

        *p++ = ( sequence & 0xff00 ) >> 8 ;
        *p++ = sequence & 0x00ff ;

        *p++ = 0x02 ;
        *p++ = 0x00 ;
        
        memcpy( p, deviceInfo.version, sizeof(deviceInfo.version) ) ;
        p+=sizeof( deviceInfo.version ) ;
        memcpy( p, deviceInfo.id, sizeof(deviceInfo.id) ) ;
        p+=sizeof( deviceInfo.id ) ;
        memcpy( p, deviceInfo.name, sizeof(deviceInfo.name) ) ;
        p+=sizeof( deviceInfo.name ) ;
        memcpy( p, deviceInfo.shortid, sizeof(deviceInfo.shortid) ) ;
        p+=sizeof( deviceInfo.shortid ) ;

        *p++ = 0x00 ;
        *p++ = 0x00 ;
        *p++ = 0x00 ;
        *p++ = 0x00 ;

        //long t = time( nullptr ) ;
        long now = ( 1000L * time( nullptr ) ) & 0xffffffff ;
        *p++ = now & 0xff ;
        *p++ = ( now & 0x0000ff00 ) >> 8 ;
        *p++ = ( now & 0x00ff0000 ) >> 16 ; 
        *p++ = ( now & 0xff000000 ) >> 24 ;

        *p++ = 0x00;
        *p++ = 0x00;
        *p++ = 0x00;
        *p++ = 0x00;

        *p++ = 0xae ; 
        *p++ = 0x49 ; 
        *p++ = 0x52 ; 
        *p++ = 0x0d ; 

        *p++ = 0x01 ; 
        *p++ = (switchOn ? 0x01 : 0x00) ; 

        sendMsg( msg, sizeof(msg) );
    }
};

std::ostream & operator<<( std::ostream &os, const Device &dev ) {
    os << dev.deviceInfo.name << '\n' 
        << dev.deviceInfo.id << '\n' 
        << dev.deviceInfo.host << ' ' << dev.deviceInfo.mac ;
    return os ;
}
