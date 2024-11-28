

#pragma once

#include <netinet/in.h>
#include <pthread.h>


typedef struct {
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
    uint8_t getReady ;
    bool isOn ;

    void recvLoop() ;
    static void *receiverThread(void *self) ;

  protected:
    void sendMsg( const void *data, size_t length ) ;
    bool reconnect();
  public:
    Device( const MSG408 &deviceInfo ) ;

    bool get() ;
    void set( bool switchOn ) ;
};

