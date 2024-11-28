


#include <ctime>
#include <iostream>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <chrono>
#include <thread>

#include "Device.hpp"

using namespace std;

void *Device::receiverThread(void *self) {
    ((Device *)self)->recvLoop();
    return nullptr;
}

void Device::recvLoop() {
    uint8_t msg[1024];
    for (;;) {
        int n = recvfrom(localSocket, msg, sizeof(msg), 0, nullptr, nullptr);
        if( n == 130 ) {
            getReady = true;
            isOn = msg[129] != 0;
        }
        if( n == 0 ) {
            cerr << "Device connection [" << deviceInfo.id << "] is closed !!!!" << endl;
        }
    }
    cerr << "Exited Device receive loop !!!!" << endl;
}

void Device::sendMsg(const void *data, size_t length) {
    sequence++;

    size_t sz = sendto(localSocket, data, length,
                       MSG_CONFIRM,
                       (const struct sockaddr *)&remoteAddress,
                       sizeof(remoteAddress));
    if (sz != length) {
        perror("sendto");
        reconnect();
    } else {
        cout << "Sent " << length << " bytes to " << inet_ntoa( remoteAddress.sin_addr ) << endl;
    }
}

Device::Device(const MSG408 &deviceInfo) : deviceInfo(deviceInfo) {
    sequence = 0x55;
    // Setup a remote address for the device
    memset(&remoteAddress, 0, sizeof(remoteAddress));
    int n = inet_pton(AF_INET, deviceInfo.host, &remoteAddress.sin_addr);
    remoteAddress.sin_port = htons(80);

    reconnect();
    int err = pthread_create(&threadId, nullptr, &receiverThread, this);
}

bool Device::reconnect() {
    cout << "Opening connection to " << deviceInfo.id << "["  << inet_ntoa( remoteAddress.sin_addr ) << "]"<< endl;

    // Prepare local socket from which we'll send and listen
    localSocket = ::socket(AF_INET, SOCK_DGRAM, 0);
    if (localSocket == 0) {
        perror("socket failed");
        return false;
    }

    cout << "Opened " << inet_ntoa( remoteAddress.sin_addr ) << endl;

    struct sockaddr_in address;
    memset(&address, 0, sizeof(address));

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = 0;

    if (::bind(localSocket, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("bind failed");
        return false;
    }

    cout << "Bound " << inet_ntoa( remoteAddress.sin_addr ) << endl;
    return true;
}

bool Device::get() {
    getReady = false;
    uint8_t msg[128];

    uint8_t *p = msg;
    *p++ = 0x17;
    *p++ = 0x00;

    *p++ = 0x05;
    *p++ = 0x00;
    *p++ = 0x00;
    *p++ = 0x00;

    *p++ = (sequence & 0xff00) >> 8;
    *p++ = sequence & 0x00ff;

    *p++ = 0x00;
    *p++ = 0x00;

    memcpy(p, deviceInfo.version, sizeof(deviceInfo.version));
    p += sizeof(deviceInfo.version);
    memcpy(p, deviceInfo.id, sizeof(deviceInfo.id));
    p += sizeof(deviceInfo.id);
    memcpy(p, deviceInfo.name, sizeof(deviceInfo.name));
    p += sizeof(deviceInfo.name);
    memcpy(p, deviceInfo.shortid, sizeof(deviceInfo.shortid));
    p += sizeof(deviceInfo.shortid);

    *p++ = 0x00;
    *p++ = 0x00;
    *p++ = 0x00;
    *p++ = 0x00;

    long now = (1000L * time(nullptr)) & 0xffffffff;
    *p++ = now & 0xff;
    *p++ = (now & 0x0000ff00) >> 8;
    *p++ = (now & 0x00ff0000) >> 16;
    *p++ = (now & 0xff000000) >> 24;

    *p++ = 0x00;
    *p++ = 0x00;
    *p++ = 0x00;
    *p++ = 0x00;

    *p++ = 0xde; // server ident
    *p++ = 0xad;
    *p++ = 0xbe;
    *p++ = 0xef;

    sendMsg(msg, sizeof(msg));
    uint16_t watchdog = 300;  // wait for 300 x 200mS = 1min.
    while ( !getReady ) {
        this_thread::sleep_for(std::chrono::milliseconds(200));
        if( --watchdog == 0 ) {
            cerr << "Timeout waiting for device response" << endl;
            reconnect();
        }
    }
    return isOn;
}

void Device::set(bool switchOn) {
    uint8_t msg[130];

    uint8_t *p = msg;
    *p++ = 0x16;
    *p++ = 0x00;

    *p++ = 0x05;
    *p++ = 0x00;
    *p++ = 0x00;
    *p++ = 0x00;

    *p++ = (sequence & 0xff00) >> 8;
    *p++ = sequence & 0x00ff;

    *p++ = 0x02;
    *p++ = 0x00;

    memcpy(p, deviceInfo.version, sizeof(deviceInfo.version));
    p += sizeof(deviceInfo.version);
    memcpy(p, deviceInfo.id, sizeof(deviceInfo.id));
    p += sizeof(deviceInfo.id);
    memcpy(p, deviceInfo.name, sizeof(deviceInfo.name));
    p += sizeof(deviceInfo.name);
    memcpy(p, deviceInfo.shortid, sizeof(deviceInfo.shortid));
    p += sizeof(deviceInfo.shortid);

    *p++ = 0x00;
    *p++ = 0x00;
    *p++ = 0x00;
    *p++ = 0x00;

    //long t = time( nullptr ) ;
    long now = (1000L * time(nullptr)) & 0xffffffff;
    *p++ = now & 0xff;
    *p++ = (now & 0x0000ff00) >> 8;
    *p++ = (now & 0x00ff0000) >> 16;
    *p++ = (now & 0xff000000) >> 24;

    *p++ = 0x00;
    *p++ = 0x00;
    *p++ = 0x00;
    *p++ = 0x00;

    *p++ = 0xde;
    *p++ = 0xad;
    *p++ = 0xbe;
    *p++ = 0xef;

    *p++ = 0x01;
    *p++ = (switchOn ? 0x01 : 0x00);

    sendMsg(msg, sizeof(msg));
}

std::ostream &operator<<(std::ostream &os, const Device &dev) {
    os << dev.deviceInfo.name << '\n'
       << dev.deviceInfo.id << '\n'
       << dev.deviceInfo.host << ' ' << dev.deviceInfo.mac;
    return os;
}
