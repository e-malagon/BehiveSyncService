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

#include "Crypto.h"

#include <crypto/base64.h>

#include <random>
#include <argon2.h>
#include <iomanip>
#include <memory.h>

namespace SyncServer {
namespace Servers {
namespace Services {
namespace Crypto {

std::string getCookie(int len) {
  std::random_device rd;
  std::mt19937 mt(rd());
  std::uniform_real_distribution<double> dist(0, 256);
  unsigned char rawCookie[len];
  for (int i = 0; i < len; ++i)
    rawCookie[i] = dist(mt);
  return base64_encode(std::string((char*) rawCookie, SALTLEN), true);
}

std::string getSalt(int len) {
  std::random_device rd;
  std::mt19937 mt(rd());
  std::uniform_real_distribution<double> dist(0, 256);
  unsigned char rawSalt[len];
  for (int i = 0; i < len; ++i)
    rawSalt[i] = dist(mt);
  return base64_encode(std::string((char*) rawSalt, SALTLEN));
}

std::string getPasswordHash(const std::string &password, const std::string &salt) {
  uint8_t hash[HASHLEN];
  uint32_t t_cost = 80;
  uint32_t m_cost = 1024;
  uint32_t parallelism = 4;
  std::string rawSalt = base64_decode(salt);
  argon2i_hash_raw(t_cost, m_cost, parallelism, (const void*) password.data(), password.size(), (const void*) rawSalt.data(), rawSalt.length(), hash, HASHLEN);
  return base64_encode(std::string((char*) hash, HASHLEN));
}

uint32_t getBeehiveHash(const std::string owner) {
  size_t len = (owner.size() / 2) + 1;
  uint16_t rowData[len];
  memset((void*) rowData, 0, sizeof(rowData));
  memcpy((void*) rowData, owner.data(), owner.size());
  uint16_t *data = rowData;
  uint32_t c0, c1;
  unsigned int i;
  for (c0 = c1 = 0; len >= 360; len -= 360) {
    for (i = 0; i < 360; ++i) {
      c0 = c0 + *data++;
      c1 = c1 + c0;
    }
    c0 = c0 % 65535;
    c1 = c1 % 65535;
  }
  for (i = 0; i < len; ++i) {
    c0 = c0 + *data++;
    c1 = c1 + c0;
  }
  c0 = c0 % 65535;
  c1 = c1 % 65535;
  uint32_t c2 = (c1 << 16 | c0);
  return c2;
}

} /* namespace Crypto */
} /* namespace Services */
} /* namespace Servers */
} /* namespace SyncServer */
