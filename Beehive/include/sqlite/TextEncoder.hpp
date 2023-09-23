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

#include <sqlite/Types.hpp>
#include <string/ICaseMap.hpp>
#include <sqlite/BinaryDecoder.hpp>
#include <sqlite/TextDecoder.hpp>

#include <cstring>
#include <string>
#include <unordered_map>
#include <map>

namespace Beehive {
namespace Services {
namespace SqLite {

class TextEncoder {
public:
  TextEncoder(const std::unordered_map<int, std::string> &mapping) :
      _mapping(mapping) {
  }
  virtual ~TextEncoder() {
  }
  int varintLen(uint64_t v);
  int putVarint(unsigned char *p, uint64_t v);

  void addInteger(int attribute, uint64_t data);
  void addReal(int attribute, double value);
  void addText(int attribute, std::string data);
  void addBlob(int attribute, std::string data);
  void addNull(int attribute);
  void addValue(BinaryDecoder::Value &value);
  void addValue(TextDecoder::Value &value);
  std::string encodedData();
private:
  std::map<std::string, std::string, Utils::ILessComparator> _data;
  const std::unordered_map<int, std::string> &_mapping;
};

} /* namespace SqLite */
} /* namespace Services */
} /* namespace Beehive */
