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
#include <crypto/Crypto.hpp>
#include <json/json.hpp>
#include <nanolog/NanoLog.hpp>
#include <services/OutboundHTTP.hpp>
#include <services/UserService.hpp>
#include <string>
#include <thread>
#include <vector>

namespace Beehive {
namespace Services {

static size_t WriteMemoryCallback(void *contents, size_t size, size_t nmemb, void *userp) {
    std::string *data = (std::string *)userp;
    size_t realsize = size * nmemb;
    data->append((const char *)contents, realsize);
    return realsize;
}

void OutboundHTTP::start() {
    LOG_INFO << "Starting Auth Server";
    std::chrono::system_clock::time_point now;
    std::chrono::system_clock::time_point expiresOn;
    sleepyWorker.start();
    do {
        CURL *curl_handle;
        CURLcode res;
        std::string headers;
        std::string response;

        sleepyWorker.bussy();
        curl_handle = curl_easy_init();
        curl_easy_setopt(curl_handle, CURLOPT_URL, "https://www.googleapis.com/oauth2/v1/certs");
        curl_easy_setopt(curl_handle, CURLOPT_HEADERFUNCTION, WriteMemoryCallback);
        curl_easy_setopt(curl_handle, CURLOPT_HEADERDATA, &headers);
        curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);
        curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, (void *)&response);
        curl_easy_setopt(curl_handle, CURLOPT_TIMEOUT, 5);
        res = curl_easy_perform(curl_handle);
        now = std::chrono::system_clock::now();
        if (res != CURLE_OK) {
            LOG_ERROR << "curl_easy_perform() failed: " << curl_easy_strerror(res);
            expiresOn = now + std::chrono::seconds(60);
        } else {
            std::size_t found = headers.find("max-age=");
            if (found != std::string::npos) {
                headers = headers.substr(found += 8, 10);
                expiresOn = now + std::chrono::seconds(std::stoi(headers));
            } else {
                expiresOn = now + std::chrono::seconds(3600);
            }
            std::vector<std::string> keys;
            auto jResponse = nlohmann::json::parse(response);
            for (auto &certificate : jResponse) {
                keys.push_back(certificate.get<std::string>());
            }
            Services::UserService::setGoogleRSARS256PubKeys(keys);
        }
        curl_easy_cleanup(curl_handle);
    } while (sleepyWorker.sleep(expiresOn - now));
    LOG_INFO << "Stoping Auth Server";
}

bool OutboundHTTP::finish() {
    return sleepyWorker.finish(10s);
}

} /* namespace Services */
} /* namespace Beehive */
