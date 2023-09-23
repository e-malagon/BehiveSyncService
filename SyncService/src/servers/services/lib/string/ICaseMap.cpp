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

#include <string/ICaseMap.h>
#include <algorithm>

namespace SyncServer {
namespace Servers {
namespace Services {
namespace Utils {

struct nocaseLessCompare {
  bool operator()(const unsigned char &c1, const unsigned char &c2) const {
    return ::tolower(c1) < ::tolower(c2);
  }
};

struct nocaseEqualsCompare {
  bool operator()(const unsigned char &c1, const unsigned char &c2) const {
    return ::tolower(c1) == ::tolower(c2);
  }
};

bool ILessComparator::operator()(const std::string &str1, const std::string &str2) const {
  return std::lexicographical_compare(str1.begin(), str1.end(), str2.begin(), str2.end(), nocaseLessCompare());
}

bool IEqualsComparator::operator()(const std::string &str1, const std::string &str2) const {
  return std::lexicographical_compare(str1.begin(), str1.end(), str2.begin(), str2.end(), nocaseEqualsCompare());
}

size_t IHasher::operator()(std::string const &key) const {
  size_t hash = 0;
  std::for_each(key.begin(), key.end(), [&hash](const char &c) {
    hash = hash * 31 + static_cast<int>(::tolower(c));
  });
  return hash;
}

} /* namespace Utils */
} /* namespace Services */
} /* namespace Servers */
} /* namespace SyncServer */
