

#pragma once

#include <string>
#include <pthread.h>
#include <map>

#include "Device.hpp"


class Connection {

  friend std::ostream & operator<<( std::ostream &os, const Connection &con ) ;

  private:
    int localSocket;
    uint32_t sequence ;
    pthread_t threadId ;
    std::map<std::string, Device> devices ;
    volatile bool running ;

    static void *receiverThread(void *self) ;
    void recvLoop() ;

  protected:
    void sendMsg( const void *data, size_t length) ;
    Device & getDevice( const std::string &deviceName ) ;

  public:
    Connection() ;
    void startDiscovery() ;
    void stopDiscovery() ;
    bool get( const std::string &deviceName ) ;
    void set( const std::string &deviceName, const bool on ) ;
    bool found( const std::string &deviceName ) ;
    std::map<std::string, Device> list() const { return devices ; } 
} ;

