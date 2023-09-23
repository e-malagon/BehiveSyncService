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


#include <fcgiapp.h>
#include <unistd.h>

#include <csignal>
#include <dao/Storage.hpp>
#include <iostream>
#include <nanolog/NanoLog.hpp>
#include <services/InboundHTTP.hpp>
#include <services/InboundTCP.hpp>
#include <services/OutboundHTTP.hpp>
#include <services/UserService.hpp>
#include <thread>

Beehive::Services::InboundHTTP inboundHTTP;
Beehive::Services::InboundTCP inboundTCP;
Beehive::Services::OutboundHTTP outboundHTTP;

void signalHandler(int signal) {
    inboundHTTP.finish();
    inboundTCP.finish();
    outboundHTTP.finish();
}

int main(int, char **) {
    try {
        std::string version = "1.0 - " + std::string(__DATE__) + " " + std::string(__TIME__);
        nanolog::initialize(nanolog::GuaranteedLogger(), "/var/log/beehive", "beehive", 1);

        LOG_DEBUG << "Starting...";

        signal(SIGINT, signalHandler);
        signal(SIGTERM, signalHandler);

        Beehive::Services::DAO::Storage::open();

        Beehive::Services::UserService::checkAdmin();
        std::thread inboundHTTPThread([] {
            try {
                inboundHTTP.start();
            } catch (std::system_error &e) {
                LOG_ERROR << e.what();
            }
        });
        std::thread inboundTCPThread([] {
            try {
                inboundTCP.start();
            } catch (std::system_error &e) {
                LOG_ERROR << e.what();
            }
        });
        std::thread outboundHTTPThread([] {
            try {
                outboundHTTP.start();
            } catch (std::system_error &e) {
                LOG_ERROR << e.what();
            }
        });

        inboundHTTPThread.join();
        inboundTCPThread.join();
        outboundHTTPThread.join();

        Beehive::Services::DAO::Storage::close();
    } catch (std::system_error &e) {
        std::cerr << e.what();
    }

    return EXIT_SUCCESS;
}
