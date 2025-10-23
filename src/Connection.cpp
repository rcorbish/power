
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
    running = true;
    while ( running ) {
        recvMsg();   
    }
    if (g_logger) {
        LOG_WARN("Exited Connection receive loop");
    }
}

void Connection::stopDiscovery() {
    running = false;
    if (localSocket >= 0) {
        shutdown(localSocket, SHUT_RDWR);
        close(localSocket);
        localSocket = -1;
    }
    pthread_join(threadId, nullptr);
    if (g_logger) {
        LOG_DEBUG("Stopped device discovery");
    }
}

void Connection::recvMsg() {
    uint8_t msg[1024];
    struct sockaddr_in remote_addr; 
    socklen_t remote_addr_len = sizeof(remote_addr);

    int n = recvfrom(localSocket, &msg, sizeof(msg), MSG_WAITALL, (struct sockaddr *)&remote_addr, &remote_addr_len);
    if (n < 0) {
        LOG_WARN("recvfrom failed: {}", strerror(errno));
    } else if ( n == sizeof(MSG408) ) {
        MSG408 deviceInfo;
        memcpy(&deviceInfo, msg, sizeof(MSG408));
        std::lock_guard<std::mutex> lock(devicesMutex);

        string deviceName(deviceInfo.id);
        if (devices.find(deviceName) == devices.end()) {
            if (g_logger) {
                LOG_INFO("Discovered new device: {}", deviceName);
            }
            devices.try_emplace(deviceName, deviceInfo);
        }
    } else if( n == 128) {
        // Ignore discovery responses
    } else if( n == 130 ) {
        uint8_t msg[130];
        if (g_logger) {
            stringstream ss;  
            for( int i = 0 ; i < n ; i++ ) {
                ss << hex << setw(2) << setfill('0') << (int)msg[i] << " " ;
            }
            LOG_DEBUG("Device received {} bytes: {}", n, ss.str() );
        }        
        if( n == 130 ) {
            const auto isOn = (msg[128] == 1) && (msg[129] != 0);
            const auto deviceId = string((char *)&msg[4], 32);
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
        }
        
    } else {
        if (g_logger) {
            LOG_WARN("Received unexpected message of {} bytes", n);
        }
    }
}

void Connection::sendMsg(const void *data, size_t length) {
    sequence++;

    struct sockaddr_in remoteAddress;
    memset(&remoteAddress, 0, sizeof(remoteAddress));
    remoteAddress.sin_family = AF_INET;
    remoteAddress.sin_port = htons(25);
    remoteAddress.sin_addr.s_addr = INADDR_BROADCAST;

    ssize_t sz = sendto(localSocket, data, length,
                       0,
                       (const struct sockaddr *)&remoteAddress,
                       sizeof(remoteAddress));
    if (sz != length) {
        if (g_logger) {
            LOG_ERROR("Connection sendto sent {} of {}: {}", sz, length, strerror(errno));
        } else {
            perror("sendto");
        }
    } else {
        if (g_logger) {
            char addr_str[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, &remoteAddress.sin_addr, addr_str, sizeof(addr_str));
            LOG_DEBUG("Connection sent {} bytes to {}", length, addr_str);
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

    // Prepare local socket from which we'll send and listen
    localSocket = ::socket(AF_INET, SOCK_DGRAM, 0);
    if (localSocket < 0) {
        if (g_logger) {
            LOG_FATAL("Connection socket creation failed: {}", strerror(errno));
        }
        throw NetworkException(errno, "Socket creation failed");
    }

    int opt = 1;
    if (::setsockopt(localSocket, SOL_SOCKET, SO_BROADCAST, &opt, sizeof(opt))) {
        if (g_logger) {
            LOG_ERROR("Connection setsockopt failed: {}", strerror(errno));
        }
        close(localSocket);
        throw NetworkException(errno, "setsockopt failed");
    }

    struct sockaddr_in address;
    memset(&address, 0, sizeof(address));
    // Bind the local socket to listen on any address
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = 0; //9000;

    if (::bind(localSocket, (struct sockaddr *)&address, sizeof(address)) < 0) {
        if (g_logger) {
            LOG_FATAL("Connection bind failed: {}", strerror(errno));
        }
        close(localSocket);
        throw NetworkException(errno, "bind failed");
    }

    if (g_logger) {
        char addr_str[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &address.sin_addr, addr_str, sizeof(addr_str));
        LOG_INFO("Listening for device broadcasts on {}", addr_str);
    }
    int err = pthread_create(&threadId, nullptr, &receiverThread, this);
    if (err != 0) {
        close(localSocket);
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

    sendMsg(discover, sizeof(discover));
}

bool Connection::get(const std::string &deviceName) {
    return getDevice(deviceName).state();
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
