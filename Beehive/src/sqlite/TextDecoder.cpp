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


#include <sqlite/TextDecoder.hpp>

#include <sqlite/DecoderException.hpp>
#include <nanolog/NanoLog.hpp>

#include <cstring>

namespace Beehive {
namespace Services {
namespace SqLite {

uint16_t TextDecoder::Value::id() {
  return _currentId;
}

std::string TextDecoder::Value::column() {
  return _current;
}

int TextDecoder::Value::type() {
  return *((char*) (_data + _current.length() + 1));
}

long TextDecoder::Value::integerValue() {
  if (*((char*) (_data + _current.length() + 1)) == AttributeType::Integer) {
    const char *data = _data + _current.length() + 2;
    uint64_t x = *((uint64_t*) data);
    return ((((x) & 0xff00000000000000ull) >> 56) //
    | (((x) & 0x00ff000000000000ull) >> 40) //
        | (((x) & 0x0000ff0000000000ull) >> 24) //
        | (((x) & 0x000000ff00000000ull) >> 8)  //
        | (((x) & 0x00000000ff000000ull) << 8)  //
        | (((x) & 0x0000000000ff0000ull) << 24) //
        | (((x) & 0x000000000000ff00ull) << 40) //
        | (((x) & 0x00000000000000ffull) << 56));
    return (long) x;
  } else {
    throw NavigationException("Invalid data type request");
  }
}

double TextDecoder::Value::realValue() {
  if (*((char*) (_data + _current.length() + 1)) == AttributeType::Real) {
    double value;
    const char *data = _data + _current.length() + 2;
    uint64_t x = *((uint64_t*) data);
    return ((((x) & 0xff00000000000000ull) >> 56) //
    | (((x) & 0x00ff000000000000ull) >> 40) //
        | (((x) & 0x0000ff0000000000ull) >> 24) //
        | (((x) & 0x000000ff00000000ull) >> 8)  //
        | (((x) & 0x00000000ff000000ull) << 8)  //
        | (((x) & 0x0000000000ff0000ull) << 24) //
        | (((x) & 0x000000000000ff00ull) << 40) //
        | (((x) & 0x00000000000000ffull) << 56));
    memcpy(&value, &x, 8);
    return value;
  } else {
    throw NavigationException("Invalid data type request");
  }
}

std::string TextDecoder::Value::textValue() {
  if (*((char*) (_data + _current.length() + 1)) == AttributeType::Text) {
    uint32_t lenght;
    const char *data = _data + _current.length() + 2 + variantLenght(lenght, _data + _current.length() + 2);
    return std::string(data, lenght);
  } else {
    throw NavigationException("Invalid data type request");
  }
}

std::string TextDecoder::Value::blobValue() {
  if (*((char*) (_data + _current.length() + 1)) == AttributeType::Blob) {
    uint32_t lenght;
    const char *data = _data + _current.length() + 2 + variantLenght(lenght, _data + _current.length() + 2);
    return std::string(data, lenght);
  } else {
    throw NavigationException("Invalid data type request");
  }
}

TextDecoder::Value& TextDecoder::Value::operator++() {
  if (_end <= _data)
    throw NavigationException("End of iteration reached");
  _currentId = goForward(true);
  if (_end < _data)
    throw NavigationException("Invalid length of change");
  return *this;
}

TextDecoder::Value TextDecoder::Value::operator++(int) {
  Value tmp(*this);
  operator++();
  return tmp;
}

bool TextDecoder::Value::operator==(const Value &rhs) const {
  return _data == rhs._data;
}

bool TextDecoder::Value::operator!=(const Value &rhs) const {
  return _data != rhs._data;
}

uint16_t TextDecoder::Value::goForward(bool skip) {
  while (_data < _end) {
    auto columnPtr = _mapping.find(_current);
    if (columnPtr != _mapping.end() && !skip) {
      return columnPtr->second;
    } else if (!skip) {
      LOG_WARN << "Skipping field '" << _entity << "." << _current << "'.";
    }
    const char *data = _data + _current.length() + 1;
    switch (*data) {
    case AttributeType::Null:
      data += 1;
      break;
    case AttributeType::Integer:
    case AttributeType::Real:
      data += 9;
      break;
    case AttributeType::Text:
    case AttributeType::Blob:
      uint32_t lenght;
      data += 1 + variantLenght(lenght, data + 1);
      data += lenght;
      break;
    default:
      throw NavigationException("Invalid data type");
    }
    _data = data;
    if (_data < _end)
      _current = data;
    skip = false;
  }
  return -1;
}

uint8_t TextDecoder::Value::variantLenght(uint32_t &value, const char *data) {
  uint32_t a, b;
  a = *data;
  if (!(a & 0x80)) {
    value = a;
    return 1;
  }
  data++;
  b = *data;
  if (!(b & 0x80)) {
    a &= 0x7f;
    a = a << 7;
    value = a | b;
    return 2;
  }
  data++;
  a = a << 14;
  a |= *data;
  if (!(a & 0x80)) {
    a &= (0x7f << 14) | (0x7f);
    b &= 0x7f;
    b = b << 7;
    value = a | b;
    return 3;
  } else {
    throw NavigationException("Invalid data length for field " + id());
  }
}

TextDecoder::iterator& TextDecoder::iterator::operator++() {
  ++_current;
  return *this;
}

TextDecoder::iterator TextDecoder::iterator::operator++(int) {
  iterator tmp(*this);
  operator++();
  return tmp;
}

bool TextDecoder::iterator::operator==(const iterator &rhs) const {
  return _current == rhs._current;
}

bool TextDecoder::iterator::operator!=(const iterator &rhs) const {
  return _current != rhs._current;
}

TextDecoder::Value& TextDecoder::iterator::operator*() {
  return _current;
}

TextDecoder::iterator TextDecoder::begin() {
  return iterator(_data, _size, _mapping, _entity);
}

TextDecoder::iterator TextDecoder::end() {
  return iterator(_data + _size, 0, _mapping, _entity);
}

} /* namespace SqLite */
} /* namespace Services */
} /* namespace Beehive */
