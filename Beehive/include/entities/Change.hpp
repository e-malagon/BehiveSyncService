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

class Change {
   public:
    uint32_t idDataset() {
        return _idDataset;
    }

    void idDataset(uint32_t idDataset) {
        _idDataset = idDataset;
    }

    uint32_t idHeader() {
        return _idHeader;
    }

    void idHeader(uint32_t idHeader) {
        _idHeader = idHeader;
    }

    uint16_t idChange() {
        return _idChange;
    }

    void idChange(uint16_t idChange) {
        _idChange = idChange;
    }

    uint8_t operation() const {
        return _operation;
    }

    void operation(uint8_t operation) {
        _operation = operation;
    }

    const std::string& entityName() const {
        return _entityName;
    }

    void entityName(std::string entityName) {
        _entityName = entityName;
    }

    const std::string& entityUUID() const {
        return _entityUUID;
    }

    void entityUUID(std::string entityUUID) {
        _entityUUID = entityUUID;
    }

    const std::string& newPK() const {
        return _newPK;
    }

    void newPK(std::string newPK) {
        _newPK = newPK;
    }

    const std::string& oldPK() const {
        return _oldPK;
    }

    void oldPK(std::string oldPK) {
        _oldPK = oldPK;
    }

    const std::string& newData() const {
        return _newData;
    }

    void newData(std::string newData) {
        _newData = newData;
    }

    const std::string& oldData() const {
        return _oldData;
    }

    void oldData(std::string oldData) {
        _oldData = oldData;
    }

   private:
    uint32_t _idDataset;
    uint32_t _idHeader;
    uint16_t _idChange;
    uint8_t _operation;
    std::string _entityName;
    std::string _entityUUID;
    std::string _newPK;
    std::string _oldPK;
    std::string _newData;
    std::string _oldData;
};

} /* namespace Entities */
} /* namespace Services */
} /* namespace Beehive */
