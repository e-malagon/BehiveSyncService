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


#pragma once

#include <string>

namespace Beehive {
namespace Services {
namespace Entities {

class Member {
   public:
    uint32_t idDataset() {
        return _idDataset;
    }

    void idDataset(uint32_t idDataset) {
        _idDataset = idDataset;
    }

    std::string idUser() {
        return _idUser;
    }

    void idUser(std::string idUser) {
        _idUser = idUser;
    }

    std::string role() {
        return _role;
    }

    void role(std::string role) {
        _role = role;
    }

    std::string name() {
        return _name;
    }

    void name(std::string name) {
        _name = name;
    }

    std::string email() {
        return _email;
    }

    void email(std::string email) {
        _email = email;
    }

    uint8_t status() {
        return _status;
    }

    void status(uint8_t status) {
        _status = status;
    }

   private:
    uint32_t _idDataset;
    std::string _idUser;
    std::string _role;
    std::string _name;
    std::string _email;
    uint8_t _status;
};

} /* namespace Entities */
} /* namespace Services */
} /* namespace Beehive */
