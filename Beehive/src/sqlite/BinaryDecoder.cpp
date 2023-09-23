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


#include <sqlite/BinaryDecoder.hpp>
#include <sqlite/DecoderException.hpp>

#include <cstring>

namespace Beehive {
namespace Services {
namespace SqLite {

uint16_t BinaryDecoder::Value::id() {
  uint32_t id;
  variantLenght(id, _data);
  return (uint16_t) id;
}

int BinaryDecoder::Value::type() {
  uint32_t id;
  return *((char*) (_data + variantLenght(id, _data)));
}

long BinaryDecoder::Value::integerValue() {
  uint32_t id;
  uint32_t idSize = variantLenght(id, _data);
  if (*((char*) (_data + idSize)) == AttributeType::Integer) {
    const char *data = _data + idSize + 1;
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

double BinaryDecoder::Value::realValue() {
  uint32_t id;
  uint32_t idSize = variantLenght(id, _data);
  if (*((char*) (_data + idSize)) == AttributeType::Real) {
    double value;
    const char *data = _data + idSize + 1;
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

std::string BinaryDecoder::Value::textValue() {
  uint32_t id;
  uint32_t idSize = variantLenght(id, _data);
  if (*((char*) (_data + idSize)) == AttributeType::Text) {
    uint32_t lenght;
    const char *data = _data + idSize + 1 + variantLenght(lenght, _data + idSize + 1);
    return std::string(data, lenght);
  } else {
    throw NavigationException("Invalid data type request");
  }
}

std::string BinaryDecoder::Value::blobValue() {
  uint32_t id;
  uint32_t idSize = variantLenght(id, _data);
  if (*((char*) (_data + idSize + 1)) == AttributeType::Blob) {
    uint32_t lenght;
    const char *data = _data + idSize + 1 + variantLenght(lenght, _data + idSize + 1);
    return std::string(data, lenght);
  } else {
    throw NavigationException("Invalid data type request");
  }
}

std::string BinaryDecoder::Value::uuidValue() {
  uint32_t id;
  uint32_t idSize = variantLenght(id, _data);
  if (*((char*) (_data + idSize)) == AttributeType::UuidV1) {
    const char *data = _data + idSize + 1;
    return std::string(data, 16);
  } else {
    throw NavigationException("Invalid data type request");
  }
}

BinaryDecoder::Value& BinaryDecoder::Value::operator++() {
  if (_end == _data)
    throw NavigationException("End of iteration reached");
  uint32_t id;
  uint32_t idSize = variantLenght(id, _data);
  const char *data = _data + idSize;
  switch (*data) {
  case AttributeType::Null:
    _data = data + 1;
    break;
  case AttributeType::Integer:
  case AttributeType::Real:
    _data = data + 9;
    break;
  case AttributeType::Text:
  case AttributeType::Blob:
    uint32_t lenght;
    data += 1 + variantLenght(lenght, _data + idSize + 1);
    _data = data + lenght;
    break;
  case AttributeType::UuidV1:
    _data = data + 17;
    break;
  default:
    throw NavigationException("Invalid data type");
  }
  if (_end < _data)
    throw NavigationException("Invalid length of change");
  return *this;
}

BinaryDecoder::Value BinaryDecoder::Value::operator++(int) {
  Value tmp(*this);
  operator++();
  return tmp;
}

bool BinaryDecoder::Value::operator==(const Value &rhs) const {
  return _data == rhs._data;
}

bool BinaryDecoder::Value::operator!=(const Value &rhs) const {
  return _data != rhs._data;
}

uint8_t BinaryDecoder::Value::variantLenght(uint32_t &value, const char *data) {
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

BinaryDecoder::iterator& BinaryDecoder::iterator::operator++() {
  ++_current;
  return *this;
}

BinaryDecoder::iterator BinaryDecoder::iterator::operator++(int) {
  iterator tmp(*this);
  operator++();
  return tmp;
}

bool BinaryDecoder::iterator::operator==(const iterator &rhs) const {
  return _current == rhs._current;
}

bool BinaryDecoder::iterator::operator!=(const iterator &rhs) const {
  return _current != rhs._current;
}

BinaryDecoder::Value& BinaryDecoder::iterator::operator*() {
  return _current;
}

BinaryDecoder::iterator BinaryDecoder::begin() {
  return iterator(_data, _size);
}

BinaryDecoder::iterator BinaryDecoder::end() {
  return iterator(_data + _size, 0);
}

} /* namespace SqLite */
} /* namespace Services */
} /* namespace Beehive */
