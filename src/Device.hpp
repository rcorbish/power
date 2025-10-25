

#pragma once

#include <netinet/in.h>
#include <pthread.h>
#include <functional> 

typedef struct {
    uint32_t skip;      // 0-3
    char version[6];  // 4-9
    char id[32];      // 10-41
    char name[32];    // 42-73
    char shortid[32]; // 74-105 

    uint8_t time[14];  // 106-119
    char ssid[64];  // 120-183
    char password[64];  // 184-247
    uint16_t data1 ;  // 248-249
    char region[10];    // 250-259
    char areacode[12];  // 260-271
    char ipa[16];   // 272-287
    char ipb[16];   // 288-303
    char ipc[16];   // 304-319
    uint32_t data2 ;  // 320-323
    uint32_t data3 ;  // 324-327
    uint32_t data4 ;  // 328-331  
    uint32_t data5 ;  // 332-335
    uint16_t data6 ;  // 336-337
    char pwd[30];     // 338-367
    char mac[18];     // 368-385
    char host[18];    // 386-403
    uint32_t data7 ;  // 404-407
} MSG408;


typedef std::function<void(const struct sockaddr_in &targetAddress, 
                           const void *data, 
                           size_t length)> sender_function;

class Device {

  friend std::ostream & operator<<( std::ostream &os, const Device &con ) ;

  private:
    MSG408 deviceInfo ;

    struct sockaddr_in remoteAddress;
    uint32_t sequence ;
    std::atomic<bool> isReady ;
    std::atomic<bool> isOn ;

    sender_function sender;
  protected:
    // void sendMsg( const void *data, size_t length ) ;
    // int connect() const ;
  public:
    Device( const MSG408 &deviceInfo, sender_function sender ) ;

    void get() ;
    void set( bool switchOn ) ;

    void updateState( const bool on ) {
        isOn.store(on);
        isReady.store(true);
    }    
    
    bool state() const ;

    Device &operator =( const Device &other ) { 
        if (this != &other) {
            memcpy(&(this->deviceInfo), &other.deviceInfo, sizeof(deviceInfo));
            memcpy(&(this->remoteAddress), &other.remoteAddress, sizeof(remoteAddress));
            this->sequence = other.sequence;
            this->isReady.store(other.isReady.load());
            this->isOn.store(other.isOn.load());
        }
        return *this;
    }
  };
