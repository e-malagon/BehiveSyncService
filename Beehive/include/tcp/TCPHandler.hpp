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

#include <cstdint>
#include <unistd.h>

namespace Beehive {
namespace Services {
namespace TCP {

class TCPHandler {
public:
  TCPHandler(int socket) :
      _socket(socket) {
  }
  virtual ~TCPHandler() {
  }

  uint8_t readOperation();
  uint8_t readUInt8(uint8_t max = 0);
  uint8_t readUInt8C(uint16_t &crc, uint8_t max = 0);
  uint16_t readUInt16(uint16_t max = 0);
  uint16_t readUInt16C(uint16_t &crc, uint16_t max = 0);
  uint32_t readUInt32(uint32_t max = 0);
  uint32_t readUInt32C(uint16_t &crc, uint32_t max = 0);
  uint64_t readUInt64(uint64_t max = 0);
  uint64_t readUInt64C(uint16_t &crc, uint64_t max = 0);
  void readChar(char *ptr, ssize_t size);
  void readCharC(char *ptr, ssize_t size, uint16_t &crc);
  void readUUID(char *ptr);
  void readUUIDC(char *ptr, uint16_t &crc);

  void writeUInt8(uint8_t value);
  void writeUInt8C(uint8_t value, uint16_t &crc);
  void writeUInt16(uint16_t value);
  void writeUInt16C(uint16_t value, uint16_t &crc);
  void writeUInt32(uint32_t value);
  void writeUInt32C(uint32_t value, uint16_t &crc);
  void writeUInt64(uint64_t value);
  void writeUInt64C(uint64_t value, uint16_t &crc);
  void writeChar(const char *ptr, ssize_t size);
  void writeCharC(const char *ptr, ssize_t size, uint16_t &crc);
  void writeUUID(const char *ptr);
  void writeUUIDC(const char *ptr, uint16_t &crc);

private:
  int _socket;

};

} /* namespace TCP */
} /* namespace Services */
} /* namespace Beehive */
