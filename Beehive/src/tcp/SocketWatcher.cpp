/*
Beehive - SQLite synchronization server.

MIT License

Copyright (c) 2021 Edgar Malagón Calderón

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/


#include <errno.h>
#include <string.h>
#include <sys/socket.h>
#include <tcp/SocketWatcher.hpp>

#include <nanolog/NanoLog.hpp>
#include <thread>

namespace Beehive {
namespace Services {
namespace TCP {

SocketWatcher globalTcpCheckerLong;
SocketWatcher globalTcpCheckerShort;

SocketWatcher::Watcher::Watcher(int clientSocket, Wait wait) : _clientSocket(clientSocket), _next(nullptr), _prev(nullptr) {
    if (wait == Long)
        _expires = std::chrono::system_clock::now() + std::chrono::seconds(15);
    else
        _expires = std::chrono::system_clock::now() + std::chrono::seconds(5);
    globalTcpCheckerLong.put(this);
}

SocketWatcher::Watcher::~Watcher() {
    globalTcpCheckerLong.drop(this);
}

SocketWatcher::SocketWatcher() : _head(nullptr), _tail(nullptr) {
    std::thread([&]() {
        while (true) {
            std::unique_lock<std::mutex> ulock(_mutex);
            if (_head == nullptr) {
                _wait.wait(ulock, [&]() {
                    return _head != nullptr;
                });
            }
            if (_head != nullptr) {
                std::chrono::system_clock::time_point expires = _head->_expires;
                ulock.unlock();
                std::this_thread::sleep_until(expires);
                ulock.lock();
                Watcher *current = _head;
                std::chrono::system_clock::time_point now = std::chrono::system_clock::now();
                while (current && current->_expires <= now) {
                    if (shutdown(current->_clientSocket, SHUT_RD) != 0) {
                        LOG_ERROR << "An error occurred when closing the socket" << strerror(errno);
                    }
                    current = current->_next;
                }
            }
        }
    }).detach();
}

void SocketWatcher::put(Watcher *client) {
    std::unique_lock<std::mutex> lock(_mutex);
    if (_tail != nullptr) {
        _tail->_next = client;
        client->_prev = _tail;
        _tail = client;
    } else {
        _head = client;
        _tail = client;
    }
    _wait.notify_all();
}

void SocketWatcher::drop(Watcher *client) {
    std::unique_lock<std::mutex> lock(_mutex);
    if (_head == _tail) {
        _head = nullptr;
        _tail = nullptr;
    } else {
        if (_head == client) {
            _head = _head->_next;
            _head->_prev = nullptr;
        } else if (_tail == client) {
            _tail = _tail->_prev;
            _tail->_next = nullptr;
        } else {
            client->_next->_prev = client->_prev;
            client->_prev->_next = client->_next;
        }
    }
}

} /* namespace TCP */
} /* namespace Services */
} /* namespace Beehive */
