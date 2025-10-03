
#include <ctime>
#include <iostream>
#include <map>
#include <pthread.h>
#include <string.h>
#include <sys/socket.h>
#include <cerrno>
#include <unistd.h>

#include "Connection.hpp"
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
    MSG408 deviceInfo;
    running = true;
    while ( running ) {
        int n = recvfrom(localSocket, &deviceInfo, sizeof(deviceInfo), 0, nullptr, nullptr);
        if (n < 0) {
            perror("recvfrom");
            break;
        }
        string deviceName(deviceInfo.id);

        if (devices.find(deviceName) == devices.end()) {
            if (g_logger) {
                LOG_INFO("Discovered new device: {}", deviceName);
            }
            devices.emplace(deviceName, deviceInfo);
        }
    }
    if (g_logger) {
        LOG_WARN("Exited Connection receive loop");
    }
}

void Connection::stopDiscovery() {
    running = false;
    pthread_join(threadId, nullptr);
    if (localSocket >= 0) {
        shutdown(localSocket, SHUT_RDWR);
        close(localSocket);
        localSocket = -1;
    }
    if (g_logger) {
        LOG_DEBUG("Stopped device discovery");
    }
}

void Connection::sendMsg(const void *data, size_t length) {
    sequence++;

    struct sockaddr_in remoteAddress;
    memset(&remoteAddress, 0, sizeof(remoteAddress));
    remoteAddress.sin_family = AF_INET;
    remoteAddress.sin_port = htons(25);
    remoteAddress.sin_addr.s_addr = INADDR_BROADCAST;

    size_t sz = sendto(localSocket, data, length,
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
            LOG_DEBUG("Connection sent {} bytes to {}", length, inet_ntoa(remoteAddress.sin_addr));
        }
    }
}

Device &Connection::getDevice(const std::string &deviceName) {
    auto it = devices.find(deviceName);
    if (it == devices.end()) {
        throw(deviceName + " not connected");
    }
    return it->second;
}

Connection::Connection() {
    sequence = 0x55;

    // Prepare local socket from which we'll send and listen
    localSocket = ::socket(AF_INET, SOCK_DGRAM, 0);
    if (localSocket == 0) {
        if (g_logger) {
            LOG_FATAL("Connection socket creation failed: {}", strerror(errno));
        } else {
            perror("socket failed");
        }
        exit(EXIT_FAILURE);
    }

    int opt = 1;
    if (::setsockopt(localSocket, SOL_SOCKET, SO_BROADCAST, &opt, sizeof(opt))) {
        if (g_logger) {
            LOG_ERROR("Connection setsockopt failed: {}", strerror(errno));
        } else {
            perror("setsockopt");
        }
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in address;
    memset(&address, 0, sizeof(address));
    // Bind the local socket to listen on any address
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = 0; //htons(PORT);

    if (::bind(localSocket, (struct sockaddr *)&address, sizeof(address)) < 0) {
        if (g_logger) {
            LOG_FATAL("Connection bind failed: {}", strerror(errno));
        } else {
            perror("bind failed");
        }
        exit(EXIT_FAILURE);
    }

    if (g_logger) {
        LOG_INFO("Listening for device broadcasts on {}", inet_ntoa(address.sin_addr));
    }
    int err = pthread_create(&threadId, nullptr, &receiverThread, this);

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
    return getDevice(deviceName).get();
}

bool Connection::found(const std::string &deviceName) {
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
