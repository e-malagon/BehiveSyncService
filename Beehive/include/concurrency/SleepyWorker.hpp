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

#include <chrono>
#include <condition_variable>
#include <future>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

namespace Beehive {
namespace Services {

using namespace std::chrono_literals;

class SleepyWorker {
   public:
    SleepyWorker() : _alive(false), _bussy(false) {
    }

    void start() {
        std::unique_lock<std::mutex> lockAlive(_aliveMutex);
        _alive = true;
        _aliveCV.notify_all();
    }

    void bussy() {
        std::unique_lock<std::mutex> lockBussy(_bussyMutex);
        _bussy = true;
        _bussyCV.notify_all();
    }

    void wakeUp() {
        std::unique_lock<std::mutex> lockAlive(_aliveMutex);
        std::unique_lock<std::mutex> lockBussy(_bussyMutex);
        if(_alive && !_bussy)
            _aliveCV.notify_all();
    }

    template <class Rep, class Period>
    bool sleep(std::chrono::duration<Rep, Period> const &time) {
        std::unique_lock<std::mutex> lockBussy(_bussyMutex);
        _bussy = false;
        _bussyCV.notify_all();
        lockBussy.unlock();
        if (_alive) {
            std::unique_lock<std::mutex> lockAlive(_aliveMutex);
            return !_aliveCV.wait_for(lockAlive, time, [&] { return !_alive; });
        } else {
            return _alive;
        }
    }

    template <class Rep, class Period>
    bool finish(std::chrono::duration<Rep, Period> const &time) {
        std::unique_lock<std::mutex> lockAlive(_aliveMutex);
        _alive = false;
        _aliveCV.notify_all();
        lockAlive.unlock();
        if (_bussy) {
            std::unique_lock<std::mutex> lockBussy(_bussyMutex);
            return !_bussyCV.wait_for(lockBussy, time, [&] { return !_bussy; });
        } else
            return _bussy;
    }

   private:
    std::condition_variable _aliveCV;
    std::mutex _aliveMutex;
    bool _alive;

    std::condition_variable _bussyCV;
    std::mutex _bussyMutex;
    bool _bussy;
};

} /* namespace Services */
} /* namespace Beehive */