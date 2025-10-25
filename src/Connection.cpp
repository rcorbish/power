
#include <ctime>
#include <iostream>
#include <iomanip>
#include <map>
#include <pthread.h>
#include <string.h>
#include <sys/socket.h>
#include <cerrno>
#include <unistd.h>

#include "Connection.hpp"
#include "Exceptions.hpp"
#include "Logger.hpp"
#include <arpa/inet.h>

using namespace std;

void *Connection::receiverThread(void *self) {
    ((Connection *)self)->recvLoop();
    return nullptr;
}

// This runs in a thread - since we have no idea
// how many response we get to a broadcast message
void Connection::recvLoop() {
    running.store(true);

    while ( running ) {
        recvMsg( broadcastSocket );   
    }

    if (g_logger) {
        LOG_WARN("Exited Connection receive loop");
    }
}

void Connection::stopDiscovery() {
    running.store(false);

    if (broadcastSocket >= 0) {
        shutdown(broadcastSocket, SHUT_RDWR);
        close(broadcastSocket);
        broadcastSocket = -1;
    }
    pthread_join(threadId, nullptr);
    if (g_logger) {
        LOG_DEBUG("Stopped device discovery");
    }
}

void Connection::recvMsg( int skt ) {
    uint8_t msg[1024];
    struct sockaddr_in remote_addr; 
    socklen_t remote_addr_len = sizeof(remote_addr);

    int n = recvfrom(skt, &msg, sizeof(msg), MSG_WAITALL, (struct sockaddr *)&remote_addr, &remote_addr_len);
    if (n < 0) {
        LOG_WARN("recvfrom failed: {}", strerror(errno));
    } else if ( n == sizeof(MSG408) ) {
        char addr_str[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &remote_addr.sin_addr, addr_str, sizeof(addr_str));
        LOG_INFO("Received MSG408 from {}:{}", addr_str, ntohs(remote_addr.sin_port));

        if (g_logger) {
            stringstream ss;  
            for( int i = 0 ; i < n ; i++ ) {
                ss << hex << setw(2) << setfill('0') << (int)msg[i] << " " ;
            }
            LOG_DEBUG("Device received {} bytes: {}", n, ss.str() );
        }
        MSG408 deviceInfo;
        memcpy(&deviceInfo, msg, sizeof(MSG408));
        std::lock_guard<std::mutex> lock(devicesMutex);

        string deviceName(deviceInfo.id);
        if (devices.find(deviceName) == devices.end()) {
            if (g_logger) {
                LOG_INFO("Discovered new device: {}", deviceName);
            }
            devices.try_emplace(deviceName, 
                                deviceInfo, 
                                [this](const struct sockaddr_in *targetAddress, 
                                       const void *data, 
                                       size_t length) {
                                    sendMsg(targetAddress, data, length);
                                });
        }
    } else if( n == 128) {
        // Ignore discovery responses
        LOG_DEBUG("Received 128 byte discovery response - ignoring");
    } else if( n == 130 ) {
        LOG_DEBUG("Received 130 byte discovery response - processing state update");
        if (g_logger) {
            stringstream ss;  
            for( int i = 0 ; i < n ; i++ ) {
                ss << hex << setw(2) << setfill('0') << (int)msg[i] << " " ;
            }
            LOG_DEBUG("Device received {} bytes: {}", n, ss.str() );
        }        
        const auto isOn = (msg[128] == 1);
        const auto deviceId = string((char *)&msg[16], 32);
        LOG_DEBUG( "Scanning for device [{}], in {} devices", deviceId, devices.size() );
        
        std::lock_guard<std::mutex> lock(devicesMutex);
        auto it = devices.find(deviceId);
        if (it != devices.end()) {
            it->second.updateState(isOn);
            if (g_logger) {
                LOG_INFO("Updated device [{}] state to {}", deviceId, isOn ? "ON" : "OFF");
            }   
        } else {
            if (g_logger) {
                LOG_ERROR("Unknown device [{}] sent state update", deviceId);
            }
        }
    } else {
        if (g_logger) {
            LOG_WARN("Received unexpected message of {} bytes", n);
        }
    }
}

void Connection::broadcastMsg(const void *data, size_t length) {
    struct sockaddr_in broadcastAddress;

    memset(&broadcastAddress, 0, sizeof(broadcastAddress));
    broadcastAddress.sin_family = AF_INET;
    broadcastAddress.sin_port = htons(25);
    broadcastAddress.sin_addr.s_addr = INADDR_BROADCAST;

    sendMsg(&broadcastAddress, data, length);
    // sequence++;

    // ssize_t sz = sendto(broadcastSocket, data, length,
    //                    0,
    //                    (const struct sockaddr *)&broadcastAddress,
    //                    sizeof(broadcastAddress));
    // if (sz != length) {
    //     if (g_logger) {
    //         LOG_ERROR("Connection sendto sent {} of {}: {}", sz, length, strerror(errno));
    //     } else {
    //         perror("sendto");
    //     }
    // } else {
    //     if (g_logger) {
    //         char addr_str[INET_ADDRSTRLEN];
    //         inet_ntop(AF_INET, &broadcastAddress.sin_addr, addr_str, sizeof(addr_str));
    //         LOG_DEBUG("Connection sent {} bytes to {}:{}", length, addr_str, ntohs(broadcastAddress.sin_port));
    //     }
    // }
}

void Connection::sendMsg( const struct sockaddr_in *targetAddress, const void *data, size_t length ) {
    sequence++;

    ssize_t sz = ::sendto(broadcastSocket, data, length,
                    MSG_CONFIRM,
                    (const struct sockaddr *)targetAddress,
                    sizeof(*targetAddress));
    if (sz != length) {
        if (g_logger) {
            LOG_ERROR("Connection sendto failed: {}", strerror(errno));
        } else {
            perror("sendto");
        }
    } else {
        if (g_logger) {
            char addr_str[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, &targetAddress->sin_addr, addr_str, sizeof(addr_str));
            LOG_INFO("Connection sent {} bytes to {}:{}", length, addr_str, ntohs(targetAddress->sin_port));
            stringstream ss;  
            for( int i = 0 ; i < length ; i++ ) {
                ss << hex << setw(2) << setfill('0') << (int)((uint8_t *)data)[i] << " " ;
            }
            LOG_DEBUG("Data: {}", ss.str());    
        }
    }
}

Device &Connection::getDevice(const std::string &deviceName) {
    std::lock_guard<std::mutex> lock(devicesMutex);
    auto it = devices.find(deviceName);
    if (it == devices.end()) {
        throw DeviceException(deviceName, "not connected");
    }
    return it->second;
}

Connection::Connection() {
    sequence = 0x55;

    // Prepare local socket from which we'll send broadcasts
    broadcastSocket = ::socket(AF_INET, SOCK_DGRAM, 0);
    if (broadcastSocket < 0) {
        if (g_logger) {
            LOG_FATAL("Connection socket creation failed: {}", strerror(errno));
        }
        throw NetworkException(errno, "Socket creation failed");
    }

    int opt = 1;
    if (::setsockopt(broadcastSocket, SOL_SOCKET, SO_BROADCAST, &opt, sizeof(opt))) {
        if (g_logger) {
            LOG_ERROR("Connection setsockopt failed: {}", strerror(errno));
        }
        close(broadcastSocket);
        throw NetworkException(errno, "setsockopt failed");
    }

    struct sockaddr_in localAddress;
    memset(&localAddress, 0, sizeof(localAddress));
    // Bind the local socket to listen on any address
    localAddress.sin_family = AF_INET;
    localAddress.sin_addr.s_addr = INADDR_ANY;
    localAddress.sin_port = 0; // Let OS pick the port

    if (::bind(broadcastSocket, (struct sockaddr *)&localAddress, sizeof(localAddress)) < 0) {
        if (g_logger) {
            LOG_FATAL("Connection bind failed: {}", strerror(errno));
        }
        close(broadcastSocket);
        throw NetworkException(errno, "bind failed");
    }

    socklen_t addrlen = sizeof(localAddress);
    getsockname(broadcastSocket, (struct sockaddr *)&localAddress, &addrlen);

    char addr_str[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &localAddress.sin_addr, addr_str, sizeof(addr_str));
    LOG_INFO("Listening for device broadcasts on port {}", ntohs(localAddress.sin_port));

    int err = pthread_create(&threadId, nullptr, &receiverThread, this);
    if (err != 0) {
        close(broadcastSocket);
        if (g_logger) {
            LOG_FATAL("Failed to create receiver thread: {}", strerror(err));
        }
        throw NetworkException(err, "Thread creation failed");
    }
}

void Connection::startDiscovery() {
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

    broadcastMsg(discover, sizeof(discover));
}

bool Connection::get(const std::string &deviceName) {
    auto &device = getDevice(deviceName);
    device.get();
    return device.state();
}

bool Connection::found(const std::string &deviceName) {
    std::lock_guard<std::mutex> lock(devicesMutex);
    auto it = devices.find(deviceName);
    return (it != devices.end()) ;
}

void Connection::set(const std::string &deviceName, const bool on) {
    getDevice(deviceName).set(on);
}

std::ostream &operator<<(std::ostream &os, const Connection &con) {
    os << "Device not initialized";
    return os;
}
