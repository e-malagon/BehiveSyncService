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

#include <tcp/TCPHandler.h>
#include <tcp/SocketWatcher.h>
#include <tcp/TCPException.h>

#include <unistd.h>
#include <stdint.h>
#include <netinet/in.h>

namespace SyncServer {
namespace Servers {
namespace TCP {

uint64_t htonll(uint64_t x) {
#if BYTE_ORDER == BIG_ENDIAN
  return x;
#elif BYTE_ORDER == LITTLE_ENDIAN
  return ((((x) & 0xff00000000000000ull) >> 56) //
  | (((x) & 0x00ff000000000000ull) >> 40) //
      | (((x) & 0x0000ff0000000000ull) >> 24) //
      | (((x) & 0x000000ff00000000ull) >> 8)  //
      | (((x) & 0x00000000ff000000ull) << 8)  //
      | (((x) & 0x0000000000ff0000ull) << 24) //
      | (((x) & 0x000000000000ff00ull) << 40) //
      | (((x) & 0x00000000000000ffull) << 56));
#else
# error "What kind of system is this?"
#endif
}

uint64_t ntohll(uint64_t x) {
#if BYTE_ORDER == BIG_ENDIAN
  return x;
#elif BYTE_ORDER == LITTLE_ENDIAN
  return htonll(x);
#else
# error "What kind of system is this?"
#endif
}

static bool crc_tab16_init = false;
static uint16_t crc_tab16[256];

void init_crc16_tab(void) {
  uint16_t i;
  uint16_t j;
  uint16_t crc;
  uint16_t c;
  for (i = 0; i < 256; i++) {
    crc = 0;
    c = i;
    for (j = 0; j < 8; j++) {
      if ((crc ^ c) & 0x0001)
        crc = (crc >> 1) ^ 0xA001;
      else
        crc = crc >> 1;
      c = c >> 1;
    }
    crc_tab16[i] = crc;
  }
  crc_tab16_init = true;
}

uint16_t update_crc_16(uint16_t crc, unsigned char c) {
  uint16_t tmp;
  uint16_t short_c;
  short_c = 0x00ff & (uint16_t) c;
  if (!crc_tab16_init)
    init_crc16_tab();
  tmp = crc ^ short_c;
  crc = (crc >> 8) ^ crc_tab16[tmp & 0xff];
  return crc;
}

uint8_t TCPHandler::readOperation() {
  SocketWatcher::Watcher watcher(_socket);
  uint8_t value;
  if (sizeof(uint8_t) != read(_socket, &value, sizeof(uint8_t)))
    return 0;
  else
    return value;
}

uint8_t TCPHandler::readUInt8(uint8_t max) {
  SocketWatcher::Watcher watcher(_socket);
  uint8_t value;
  if (sizeof(uint8_t) != read(_socket, &value, sizeof(uint8_t))) {
    throw TransmissionErrorException("Not enough data in the buffer", 0);
  }
  if (max && max < value) {
    throw TransmissionErrorException("Message size to big wanted " + std::to_string(max) + " readed " + std::to_string(value), 0);
  }
  return value;
}

uint8_t TCPHandler::readUInt8C(uint16_t &crc, uint8_t max) {
  SocketWatcher::Watcher watcher(_socket);
  uint8_t value;
  if (sizeof(uint8_t) != read(_socket, &value, sizeof(uint8_t))) {
    throw TransmissionErrorException("Not enough data in the buffer", 0);
  }
  crc = update_crc_16(crc, value);
  if (max && max < value) {
    throw TransmissionErrorException("Message size to big wanted " + std::to_string(max) + " readed " + std::to_string(value), 0);
  }
  return value;
}

uint16_t TCPHandler::readUInt16(uint16_t max) {
  SocketWatcher::Watcher watcher(_socket);
  uint16_t value;
  if (sizeof(uint16_t) != read(_socket, &value, sizeof(uint16_t))) {
    throw TransmissionErrorException("Not enough data in the buffer", 0);
  }
  value = ntohs(value);
  if (max && max < value) {
    throw TransmissionErrorException("Message size to big wanted " + std::to_string(max) + " readed " + std::to_string(value), 0);
  }
  return value;
}

uint16_t TCPHandler::readUInt16C(uint16_t &crc, uint16_t max) {
  SocketWatcher::Watcher watcher(_socket);
  uint16_t value;
  if (sizeof(uint16_t) != read(_socket, &value, sizeof(uint16_t))) {
    throw TransmissionErrorException("Not enough data in the buffer", 0);
  }
  uint8_t *ptr = (uint8_t*) &value;
  for (unsigned int i = 0; i < sizeof(value); i++)
    crc = update_crc_16(crc, (unsigned char) *ptr++);
  value = ntohs(value);
  if (max && max < value) {
    throw TransmissionErrorException("Message size to big wanted " + std::to_string(max) + " readed " + std::to_string(value), 0);
  }
  return value;
}

uint32_t TCPHandler::readUInt32(uint32_t max) {
  SocketWatcher::Watcher watcher(_socket);
  uint32_t value;
  if (sizeof(uint32_t) != read(_socket, &value, sizeof(uint32_t))) {
    throw TransmissionErrorException("Not enough data in the buffer", 0);
  }
  value = ntohl(value);
  if (max && max < value) {
    throw TransmissionErrorException("Message size to big wanted " + std::to_string(max) + " readed " + std::to_string(value), 0);
  }
  return value;
}

uint32_t TCPHandler::readUInt32C(uint16_t &crc, uint32_t max) {
  SocketWatcher::Watcher watcher(_socket);
  uint32_t value;
  if (sizeof(uint32_t) != read(_socket, &value, sizeof(uint32_t))) {
    throw TransmissionErrorException("Not enough data in the buffer", 0);
  }
  uint8_t *ptr = (uint8_t*) &value;
  for (unsigned int i = 0; i < sizeof(value); i++)
    crc = update_crc_16(crc, (unsigned char) *ptr++);
  value = ntohl(value);
  if (max && max < value) {
    throw TransmissionErrorException("Message size to big wanted " + std::to_string(max) + " readed " + std::to_string(value), 0);
  }
  return value;
}

uint64_t TCPHandler::readUInt64(uint64_t max) {
  SocketWatcher::Watcher watcher(_socket);
  uint64_t value;
  if (sizeof(uint64_t) != read(_socket, &value, sizeof(uint64_t))) {
    throw TransmissionErrorException("Not enough data in the buffer", 0);
  }
  value = ntohll(value);
  if (max && max < value) {
    throw TransmissionErrorException("Message size to big wanted " + std::to_string(max) + " readed " + std::to_string(value), 0);
  }
  return value;
}

uint64_t TCPHandler::readUInt64C(uint16_t &crc, uint64_t max) {
  SocketWatcher::Watcher watcher(_socket);
  uint64_t value;
  if (sizeof(uint64_t) != read(_socket, &value, sizeof(uint64_t))) {
    throw TransmissionErrorException("Not enough data in the buffer", 0);
  }
  uint8_t *ptr = (uint8_t*) &value;
  for (unsigned int i = 0; i < sizeof(value); i++)
    crc = update_crc_16(crc, (unsigned char) *ptr++);
  value = ntohll(value);
  if (max && max < value) {
    throw TransmissionErrorException("Message size to big wanted " + std::to_string(max) + " readed " + std::to_string(value), 0);
  }
  return value;
}

void TCPHandler::readChar(char *ptr, ssize_t size) {
  SocketWatcher::Watcher watcher(_socket);
  if (size != 0 && size != read(_socket, ptr, size)) {
    throw TransmissionErrorException("Not enough data in the buffer", 0);
  }
}

void TCPHandler::readCharC(char *ptr, ssize_t size, uint16_t &crc) {
  SocketWatcher::Watcher watcher(_socket);
  if (size != 0 && size != read(_socket, ptr, size)) {
    throw TransmissionErrorException("Not enough data in the buffer", 0);
  }
  for (unsigned int i = 0; i < size; i++)
    crc = update_crc_16(crc, (unsigned char) *ptr++);
}

void TCPHandler::readUUID(char *ptr) {
  SocketWatcher::Watcher watcher(_socket);
  if (36 != read(_socket, ptr, 36)) {
    throw TransmissionErrorException("Not enough data in the buffer", 0);
  }
}

void TCPHandler::readUUIDC(char *ptr, uint16_t &crc) {
  SocketWatcher::Watcher watcher(_socket);
  if (36 != read(_socket, ptr, 36)) {
    throw TransmissionErrorException("Not enough data in the buffer", 0);
  }
  for (unsigned int i = 0; i < 36; i++)
    crc = update_crc_16(crc, (unsigned char) *ptr++);
}

void TCPHandler::writeUInt8(uint8_t value) {
  if (sizeof(uint8_t) != write(_socket, &value, sizeof(uint8_t))) {
    throw TransmissionErrorException("Network error while writing output data", 0);
  }
}

void TCPHandler::writeUInt8C(uint8_t value, uint16_t &crc) {
  if (sizeof(uint8_t) != write(_socket, &value, sizeof(uint8_t))) {
    throw TransmissionErrorException("Network error while writing output data", 0);
  }
  crc = update_crc_16(crc, value);
}

void TCPHandler::writeUInt16(uint16_t value) {
  value = htons(value);
  if (sizeof(uint16_t) != write(_socket, &value, sizeof(uint16_t))) {
    throw TransmissionErrorException("Network error while writing output data", 0);
  }
}

void TCPHandler::writeUInt16C(uint16_t value, uint16_t &crc) {
  value = htons(value);
  if (sizeof(uint16_t) != write(_socket, &value, sizeof(uint16_t))) {
    throw TransmissionErrorException("Network error while writing output data", 0);
  }
  uint8_t *ptr = (uint8_t*) &value;
  for (unsigned int i = 0; i < sizeof(value); i++)
    crc = update_crc_16(crc, (unsigned char) *ptr++);
}

void TCPHandler::writeUInt32(uint32_t value) {
  value = htonl(value);
  if (sizeof(uint32_t) != write(_socket, &value, sizeof(uint32_t))) {
    throw TransmissionErrorException("Network error while writing output data", 0);
  }
}

void TCPHandler::writeUInt32C(uint32_t value, uint16_t &crc) {
  value = htonl(value);
  if (sizeof(uint32_t) != write(_socket, &value, sizeof(uint32_t))) {
    throw TransmissionErrorException("Network error while writing output data", 0);
  }
  uint8_t *ptr = (uint8_t*) &value;
  for (unsigned int i = 0; i < sizeof(value); i++)
    crc = update_crc_16(crc, (unsigned char) *ptr++);
}

void TCPHandler::writeUInt64(uint64_t value) {
  value = htonll(value);
  if (sizeof(uint64_t) != write(_socket, &value, sizeof(uint64_t))) {
    throw TransmissionErrorException("Network error while writing output data", 0);
  }
}

void TCPHandler::writeUInt64C(uint64_t value, uint16_t &crc) {
  value = htonll(value);
  if (sizeof(uint64_t) != write(_socket, &value, sizeof(uint64_t))) {
    throw TransmissionErrorException("Network error while writing output data", 0);
  }
  uint8_t *ptr = (uint8_t*) &value;
  for (unsigned int i = 0; i < sizeof(value); i++)
    crc = update_crc_16(crc, (unsigned char) *ptr++);
}

void TCPHandler::writeChar(const char *ptr, ssize_t size) {
  if (size != 0 && size != write(_socket, ptr, size)) {
    throw TransmissionErrorException("Network error while writing output data", 0);
  }
}

void TCPHandler::writeCharC(const char *ptr, ssize_t size, uint16_t &crc) {
  if (size != 0 && size != write(_socket, ptr, size)) {
    throw TransmissionErrorException("Network error while writing output data", 0);
  }
  for (unsigned int i = 0; i < size; i++)
    crc = update_crc_16(crc, (unsigned char) *ptr++);
}

void TCPHandler::writeUUID(const char *ptr) {
  if (36 != write(_socket, ptr, 36)) {
    throw TransmissionErrorException("Network error while writing output data", 0);
  }
}

void TCPHandler::writeUUIDC(const char *ptr, uint16_t &crc) {
  if (36 != write(_socket, ptr, 36)) {
    throw TransmissionErrorException("Network error while writing output data", 0);
  }
  for (unsigned int i = 0; i < 36; i++)
    crc = update_crc_16(crc, (unsigned char) *ptr++);
}

} /* namespace ASIO */
} /* namespace Servers */
} /* namespace SyncServer */
