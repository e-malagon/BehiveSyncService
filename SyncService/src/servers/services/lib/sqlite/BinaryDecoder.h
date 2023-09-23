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

#ifndef BINARYDECODER_H_
#define BINARYDECODER_H_

#include <sqlite/Types.h>

#include <algorithm>
#include <string>

namespace SyncServer {
namespace Servers {
namespace Services {
namespace SqLite {

class BinaryDecoder {
public:
  BinaryDecoder(const char *data, size_t size) :
      _data(data), _size(size) {
  }

  virtual ~BinaryDecoder() {
  }

  class Value {
  public:
    Value(const char *data, size_t size) :
        _data(data), _end(data + size) {
    }
    virtual ~Value() {
    }
    uint16_t id();
    int type();
    long integerValue();
    double realValue();
    std::string textValue();
    std::string blobValue();
    std::string uuidValue();
    Value& operator++();
    Value operator++(int);
    bool operator==(const Value &rhs) const;
    bool operator!=(const Value &rhs) const;
  private:
    uint8_t variantLenght(uint32_t &value, const char *data);
    const char *_data;
    const char *_end;
  };

  class iterator: public std::iterator<std::input_iterator_tag, Value> {
    Value _current;
  public:
    iterator(const char *data, size_t size) :
        _current(data, size) {
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
};

} /* namespace SqLite */
} /* namespace Services */
} /* namespace Servers */
} /* namespace SyncServer */

#endif /* BINARYDECODER_H_ */
