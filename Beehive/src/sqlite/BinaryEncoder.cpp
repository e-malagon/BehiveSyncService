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


#include <sqlite/BinaryEncoder.hpp>

namespace Beehive {
namespace Services {
namespace SqLite {

int BinaryEncoder::varintLen(uint64_t v) {
  int i;
  for (i = 1; (v >>= 7) != 0; i++)
    ;
  return i;
}

int BinaryEncoder::putVarint(unsigned char *p, uint64_t v) {
  if (v <= 0x7f) {
    p[0] = v & 0x7f;
    return 1;
  }
  if (v <= 0x3fff) {
    p[0] = ((v >> 7) & 0x7f) | 0x80;
    p[1] = v & 0x7f;
    return 2;
  }
  return 0;
}

void BinaryEncoder::addInteger(int attribute, uint64_t data) {
  int currpos = varintLen(attribute);
  uint8_t buffer[currpos + 9];
  putVarint(buffer, attribute);
  buffer[currpos++] = AttributeType::Integer;
  buffer[currpos++] = (data >> 56) & 0xFF;
  buffer[currpos++] = (data >> 48) & 0xFF;
  buffer[currpos++] = (data >> 40) & 0xFF;
  buffer[currpos++] = (data >> 32) & 0xFF;
  buffer[currpos++] = (data >> 24) & 0xFF;
  buffer[currpos++] = (data >> 16) & 0xFF;
  buffer[currpos++] = (data >> 8) & 0xFF;
  buffer[currpos++] = (data >> 0) & 0xFF;
  _data[attribute] = std::string((char*) buffer, currpos);
}

void BinaryEncoder::addReal(int attribute, double value) {
  uint64_t data;
  memcpy(&data, &value, 8);
  int currpos = varintLen(attribute);
  uint8_t buffer[currpos + 9];
  putVarint(buffer, attribute);
  buffer[currpos++] = AttributeType::Real;
  buffer[currpos++] = (data >> 56) & 0xFF;
  buffer[currpos++] = (data >> 48) & 0xFF;
  buffer[currpos++] = (data >> 40) & 0xFF;
  buffer[currpos++] = (data >> 32) & 0xFF;
  buffer[currpos++] = (data >> 24) & 0xFF;
  buffer[currpos++] = (data >> 16) & 0xFF;
  buffer[currpos++] = (data >> 8) & 0xFF;
  buffer[currpos++] = (data >> 0) & 0xFF;
  _data[attribute] = std::string((char*) buffer, currpos);
}

void BinaryEncoder::addText(int attribute, std::string data) {
  uint64_t size = data.size();
  int currpos = varintLen(attribute);
  uint8_t buffer[currpos + size + 4];
  putVarint(buffer, attribute);
  buffer[currpos++] = AttributeType::Text;
  putVarint(&buffer[currpos], size);
  currpos += varintLen(size);
  data.copy((char*) (buffer + currpos), data.size());
  _data[attribute] = std::string((char*) buffer, currpos + data.size());
}

void BinaryEncoder::addBlob(int attribute, std::string data) {
  uint64_t size = data.size();
  int currpos = varintLen(attribute);
  uint8_t buffer[currpos + size + 4];
  putVarint(buffer, attribute);
  buffer[currpos++] = AttributeType::Blob;
  putVarint(&buffer[currpos], size);
  currpos += varintLen(size);
  data.copy((char*) (buffer + currpos), data.size());
  _data[attribute] = std::string((char*) buffer, currpos + data.size());
}

void BinaryEncoder::addNull(int attribute) {
  int currpos = varintLen(attribute);
  uint8_t buffer[currpos + 2];
  putVarint(buffer, attribute);
  buffer[currpos++] = AttributeType::Null;
  _data[attribute] = std::string((char*) buffer, currpos);
}

void BinaryEncoder::addUUID(int attribute, std::string data) {
  int currpos = varintLen(attribute);
  uint8_t buffer[currpos + 17];
  putVarint(buffer, attribute);
  buffer[currpos++] = AttributeType::UuidV1;
  data.copy((char*) (buffer + currpos), 16);
  _data[attribute] = std::string((char*) buffer, currpos + 16);
}

void BinaryEncoder::addValue(BinaryDecoder::Value &value) {
  switch (value.type()) {
  case SqLite::AttributeType::Integer:
    addInteger(value.id(), value.integerValue());
    break;
  case SqLite::AttributeType::Real:
    addReal(value.id(), value.realValue());
    break;
  case SqLite::AttributeType::Text:
    addText(value.id(), value.textValue());
    break;
  case SqLite::AttributeType::Blob:
    addBlob(value.id(), value.blobValue());
    break;
  case SqLite::AttributeType::Null:
    addNull(value.id());
    break;
  case SqLite::AttributeType::UuidV1:
    addUUID(value.id(), value.uuidValue());
    break;
  }
}

void BinaryEncoder::addValue(TextDecoder::Value &value) {
  switch (value.type()) {
  case SqLite::AttributeType::Integer:
    addInteger(value.id(), value.integerValue());
    break;
  case SqLite::AttributeType::Real:
    addReal(value.id(), value.realValue());
    break;
  case SqLite::AttributeType::Text:
    addText(value.id(), value.textValue());
    break;
  case SqLite::AttributeType::Blob:
    addBlob(value.id(), value.blobValue());
    break;
  case SqLite::AttributeType::Null:
    addNull(value.id());
    break;
  }
}

std::string BinaryEncoder::encodedData() {
  int size = 0;
  for (auto &attribute : _data) {
    size += attribute.second.size();
  }
  uint8_t buffer[size];
  size = 0;
  for (auto &attribute : _data) {
    attribute.second.copy((char*) (buffer + size), attribute.second.size());
    size += attribute.second.size();
  }
  return std::string((char*) buffer, size);
}

} /* namespace SqLite */
} /* namespace Services */
} /* namespace Beehive */
