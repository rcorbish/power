


#include <ctime>
#include <iostream>
#include <iomanip>
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


Device::Device(const MSG408 &deviceInfo, sender_function _sender) : sender(_sender), deviceInfo(deviceInfo), isReady(false), isOn(false) {
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
        remoteAddress.sin_port = htons(80); // Standard ECO-plugs port
        LOG_DEBUG( "Device {} address set to {}:80", deviceInfo.id, deviceInfo.host);
    }
}


// void Device::sendMsg(const void *data, size_t length) {
//     sequence++;
  
//     const auto localSocket = connect();

//     auto sz = ::sendto(localSocket, data, length,
//                     MSG_CONFIRM,
//                     (const struct sockaddr *)&remoteAddress,
//                     sizeof(remoteAddress));
//     if (sz != length) {
//         if (g_logger) {
//             LOG_ERROR("Device sendto failed: {}", strerror(errno));
//         } else {
//             perror("sendto");
//         }
//     } else {
//         if (g_logger) {
//             char addr_str[INET_ADDRSTRLEN];
//             inet_ntop(AF_INET, &remoteAddress.sin_addr, addr_str, sizeof(addr_str));
//             LOG_DEBUG("Device sent {} bytes to {}:{}", length, addr_str, ntohs(remoteAddress.sin_port));
//         }
//     }
//     close( localSocket );
// }


// int Device::connect() const {
//     // if (g_logger) {
//     //     char addr_str[INET_ADDRSTRLEN];
//     //     inet_ntop(AF_INET, &remoteAddress.sin_addr, addr_str, sizeof(addr_str));
//     //     LOG_INFO("Opening connection to device {} [{}]", deviceInfo.id, addr_str);
//     // }

//     // Prepare local socket from which we'll send and listen
//     int localSocket = ::socket(AF_INET, SOCK_DGRAM, 0);
//     if (localSocket < 0) {
//         if (g_logger) {
//             LOG_ERROR("Device socket creation failed: {}", strerror(errno));
//         } else {
//             perror("socket failed");
//         }
//         return -1;
//     }

//     // int reuse_true = 1;
//     // if (setsockopt(localSocket,SOL_SOCKET,SO_REUSEADDR,&reuse_true, sizeof(reuse_true)) < 0) {
//     //     if (g_logger) {
//     //         LOG_ERROR("Device setsockopt failed: {}", strerror(errno));
//     //     } else {
//     //         perror("setsockopt");
//     //     }
//     //     close(localSocket);
//     //     return -1;
//     // }

//     // if (g_logger) {
//     //     char addr_str[INET_ADDRSTRLEN];
//     //     inet_ntop(AF_INET, &remoteAddress.sin_addr, addr_str, sizeof(addr_str));
//     //     LOG_DEBUG("Device connection opened to {}", addr_str);
//     // }

//     struct sockaddr_in address;
//     memset(&address, 0, sizeof(address));

//     address.sin_family = AF_INET;
//     address.sin_addr.s_addr = INADDR_ANY;
//     address.sin_port = htons(localPort);

//     if (::bind(localSocket, (struct sockaddr *)&address, sizeof(address)) < 0) {
//         if (g_logger) {
//             LOG_ERROR("Device bind failed: {}", strerror(errno));
//         } else {
//             perror("bind failed");
//         }
//         close(localSocket);
//         return -1;
//     }

//     if (g_logger) {
//         char local_addr[INET_ADDRSTRLEN];
//         char remote_addr[INET_ADDRSTRLEN];
//         inet_ntop(AF_INET, &address.sin_addr, local_addr, sizeof(local_addr));
//         inet_ntop(AF_INET, &remoteAddress.sin_addr, remote_addr, sizeof(remote_addr));
//         LOG_DEBUG("Device bound local[{}:{}] to remote [{}:{}]", local_addr, ntohs(address.sin_port), remote_addr, ntohs(remoteAddress.sin_port));
//     }

//     // const auto flags = fcntl(localSocket,F_GETFL,0);
//     // if( flags >= 0 ) {
//     //     fcntl(localSocket, F_SETFL, flags | O_NONBLOCK);
//     // }

//     return localSocket;
// }

void Device::get() {
    isReady = false;
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

    sender(remoteAddress, msg, sizeof(msg));
}

void Device::set(bool switchOn) {
    uint8_t msg[130];

    uint8_t *p = msg;

    *p++ = 0x16;    // command: set state
    *p++ = 0x00;
    *p++ = 0x05;
    *p++ = 0x00;

    *p++ = 0x00;    // sequence
    *p++ = 0x00;
    *p++ = (sequence & 0xff00) >> 8;
    *p++ = sequence & 0x00ff;

    *p++ = 0x02;    // payload length
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

    sender(remoteAddress, msg, sizeof(msg));
}

bool Device::state() const {
    int count = 50 ; // wait up to 5 seconds
    while( isReady.load() == false ) {
        struct timespec req = {0, 100000000L}; // 100ms
        nanosleep( &req, nullptr );
        if( --count == 0 ) {
            throw std::runtime_error("Device state timeout") ;
        }
    }
    return isOn.load();
}

std::ostream &operator<<(std::ostream &os, const Device &dev) {
    os << dev.deviceInfo.name << '\n'
       << dev.deviceInfo.id << '\n'
       << dev.deviceInfo.host << ' ' << dev.deviceInfo.mac;
    return os;
}
