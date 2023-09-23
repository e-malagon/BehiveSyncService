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

#include <algorithm>
#include <string>
#include <unordered_map>

namespace Beehive {
namespace Services {
namespace SqLite {

class TextDecoder {
public:
  TextDecoder(const char *data, size_t size, const std::unordered_map<std::string, int, Utils::IHasher, Utils::IEqualsComparator> &mapping, std::string entity) :
      _data(data), _size(size), _mapping(mapping), _entity(entity) {
  }

  virtual ~TextDecoder() {
  }

  class Value {
  public:
    Value(const char *data, size_t size, const std::unordered_map<std::string, int, Utils::IHasher, Utils::IEqualsComparator> &mapping, std::string &entity) :
        _current(data), _data(data), _end(data + size), _mapping(mapping), _entity(entity) {
      _currentId = goForward(false);
    }
    virtual ~Value() {
    }
    uint16_t id();
    std::string column();
    int type();
    long integerValue();
    double realValue();
    std::string textValue();
    std::string blobValue();
    Value& operator++();
    Value operator++(int);
    bool operator==(const Value &rhs) const;
    bool operator!=(const Value &rhs) const;
  private:
    uint16_t goForward(bool skip);
    uint8_t variantLenght(uint32_t &value, const char *data);
    std::string _current;
    const char *_data;
    const char *_end;
    const std::unordered_map<std::string, int, Utils::IHasher, Utils::IEqualsComparator> &_mapping;
    uint16_t _currentId;
    std::string _entity;
    uint32_t _beehive;
  };

  class iterator: public std::iterator<std::input_iterator_tag, Value> {
    Value _current;
  public:
    iterator(const char *data, size_t size, const std::unordered_map<std::string, int, Utils::IHasher, Utils::IEqualsComparator> &mapping, std::string &entity) :
        _current(data, size, mapping, entity) {
    }
    iterator(const iterator &mit) :
        _current(mit._current) {
    }
    iterator& operator++();
    iterator operator++(int);
    bool operator==(const iterator &rhs) const;
    bool operator!=(const iterator &rhs) const;
    Value& operator*();
  };

  iterator begin();
  iterator end();

private:
  const char *_data;
  size_t _size;
  const std::unordered_map<std::string, int, Utils::IHasher, Utils::IEqualsComparator> &_mapping;
  std::string _entity;
};

} /* namespace SqLite */
} /* namespace Services */
} /* namespace Beehive */
