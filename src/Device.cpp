


#include <ctime>
#include <iostream>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <chrono>
#include <thread>
#include <cerrno>

#include "Device.hpp"
#include "Logger.hpp"
#include <netdb.h>
#include <unistd.h>

using namespace std;

// void *Device::receiverThread(void *self) {
//     ((Device *)self)->recvLoop();
//     return nullptr;
// }

// void Device::recvLoop() {
//     uint8_t msg[1024];
//     for (;;) {
//         int n = recvfrom(localSocket, msg, sizeof(msg), 0, nullptr, nullptr);
//         if( n == 130 ) {
//             getReady = true;
//             isOn = msg[129] != 0;
//         }
//         if( n == 0 ) {
//             if (g_logger) {
//                LOG_WARN("Device connection [{}] is closed", deviceInfo.id);
//            }
//         }
//     }
//     cerr << "Exited Device receive loop !!!!" << endl;
// }

void Device::sendMsg(const void *data, size_t length) {
    sequence++;
  
    const auto localSocket = connect();

    size_t sz = sendto(localSocket, data, length,
                    MSG_CONFIRM,
                    (const struct sockaddr *)&remoteAddress,
                    sizeof(remoteAddress));
    if (sz != length) {
        if (g_logger) {
            LOG_ERROR("Device sendto failed: {}", strerror(errno));
        } else {
            perror("sendto");
        }
    } else {
        if (g_logger) {
            char addr_str[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, &remoteAddress.sin_addr, addr_str, sizeof(addr_str));
            LOG_DEBUG("Device sent {} bytes to {}", length, addr_str);
        }
    }

    auto watchdog = 10 ;
    uint8_t msg[1024];

    while( true ) {
        auto n = recv(localSocket, msg, sizeof(msg), MSG_DONTWAIT);
        if( n < 0 ) {
            if( errno == EAGAIN || errno == EWOULDBLOCK ) {
                if( --watchdog == 0 ) {
                    break;
                }
                this_thread::sleep_for(chrono::milliseconds(250));
                continue; // retry reading after delay
            } else {
                perror( "recv" );
                break;
            }
        }
        if( n == 0 ) {
            if (g_logger) {
                LOG_WARN("Device connection [{}] is closed", deviceInfo.id);
                break;
            }
        } else {
            if (g_logger) {
                stringstream ss;  
                for( int i = 0 ; i < n ; i++ ) {
                    ss << hex << setw(2) << setfill('0') << (int)msg[i] << " " ;
                }
                LOG_DEBUG("Device received {} bytes: {}", n, ss.str() );
            }
        }
        if( n == 130 ) {
            isOn = msg[129] != 0;
            getReady = true;
            break;
        }
    }
    close( localSocket );
}

Device::Device(const MSG408 &deviceInfo) : deviceInfo(deviceInfo) {
    sequence = 0x55;
    // Setup a remote address for the device
    memset(&remoteAddress, 0, sizeof(remoteAddress));
    int n = inet_pton(AF_INET, deviceInfo.host, &remoteAddress.sin_addr);
    if( n <= 0 ) {
        if (g_logger) {
            LOG_ERROR("Device inet_pton failed for {}: {}", deviceInfo.host, (n == 0 ? "invalid address" : strerror(errno)));
        } else {
            perror("inet_pton failed");
        }
    } else {
        remoteAddress.sin_port = htons(80);
    }   
}

int Device::connect() {
    if (g_logger) {
        char addr_str[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &remoteAddress.sin_addr, addr_str, sizeof(addr_str));
        LOG_INFO("Opening connection to device {} [{}]", deviceInfo.id, addr_str);
    }

    // Prepare local socket from which we'll send and listen
    int localSocket = ::socket(AF_INET, SOCK_DGRAM, 0);
    if (localSocket < 0) {
        if (g_logger) {
            LOG_ERROR("Device socket creation failed: {}", strerror(errno));
        } else {
            perror("socket failed");
        }
        return -1;
    }

    int reuse_true = 1;
    if (setsockopt(localSocket,SOL_SOCKET,SO_REUSEADDR,&reuse_true, sizeof(reuse_true)) < 0) {
        if (g_logger) {
            LOG_ERROR("Device setsockopt failed: {}", strerror(errno));
        } else {
            perror("setsockopt");
        }
        close(localSocket);
        return -1;
    }

    if (g_logger) {
        char addr_str[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &remoteAddress.sin_addr, addr_str, sizeof(addr_str));
        LOG_DEBUG("Device connection opened to {}", addr_str);
    }

    struct sockaddr_in address;
    memset(&address, 0, sizeof(address));

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = 0;

    if (::bind(localSocket, (struct sockaddr *)&address, sizeof(address)) < 0) {
        if (g_logger) {
            LOG_ERROR("Device bind failed: {}", strerror(errno));
        } else {
            perror("bind failed");
        }
        close(localSocket);
        return -1;
    }

    if (g_logger) {
        char local_addr[INET_ADDRSTRLEN];
        char remote_addr[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &address.sin_addr, local_addr, sizeof(local_addr));
        inet_ntop(AF_INET, &remoteAddress.sin_addr, remote_addr, sizeof(remote_addr));
        LOG_DEBUG("Device bound local[{}] to remote [{}]", local_addr, remote_addr);
    }

    const auto flags = fcntl(localSocket,F_GETFL,0);
    if( flags >= 0 ) {
        fcntl(localSocket, F_SETFL, flags | O_NONBLOCK);
    }

    return localSocket;
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
    unsigned long now = (1000L * time(nullptr)) & 0xffffffff;
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
