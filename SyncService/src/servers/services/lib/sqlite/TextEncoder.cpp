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

#include <sqlite/TextEncoder.h>

#include <uuid/uuid.h>

namespace SyncServer {
namespace Servers {
namespace Services {
namespace SqLite {

int TextEncoder::varintLen(uint64_t v) {
  int i;
  for (i = 1; (v >>= 7) != 0; i++)
    ;
  return i;
}

int TextEncoder::putVarint(unsigned char *p, uint64_t v) {
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

void TextEncoder::addInteger(int attribute, uint64_t data) {
  auto attributePtr = _mapping.find(attribute);
  if (attributePtr != _mapping.end()) {
    int currpos = attributePtr->second.length();
    uint8_t buffer[currpos + 10];
    attributePtr->second.copy((char*) buffer, currpos);
    buffer[currpos++] = 0;
    buffer[currpos++] = AttributeType::Integer;
    buffer[currpos++] = (data >> 56) & 0xFF;
    buffer[currpos++] = (data >> 48) & 0xFF;
    buffer[currpos++] = (data >> 40) & 0xFF;
    buffer[currpos++] = (data >> 32) & 0xFF;
    buffer[currpos++] = (data >> 24) & 0xFF;
    buffer[currpos++] = (data >> 16) & 0xFF;
    buffer[currpos++] = (data >> 8) & 0xFF;
    buffer[currpos++] = (data >> 0) & 0xFF;
    _data[attributePtr->second] = std::string((char*) buffer, currpos);
  }
}

void TextEncoder::addReal(int attribute, double value) {
  auto attributePtr = _mapping.find(attribute);
  if (attributePtr != _mapping.end()) {
    uint64_t data;
    memcpy(&data, &value, 8);
    int currpos = attributePtr->second.length();
    uint8_t buffer[currpos + 10];
    attributePtr->second.copy((char*) buffer, currpos);
    buffer[currpos++] = 0;
    buffer[currpos++] = AttributeType::Real;
    buffer[currpos++] = (data >> 56) & 0xFF;
    buffer[currpos++] = (data >> 48) & 0xFF;
    buffer[currpos++] = (data >> 40) & 0xFF;
    buffer[currpos++] = (data >> 32) & 0xFF;
    buffer[currpos++] = (data >> 24) & 0xFF;
    buffer[currpos++] = (data >> 16) & 0xFF;
    buffer[currpos++] = (data >> 8) & 0xFF;
    buffer[currpos++] = (data >> 0) & 0xFF;
    _data[attributePtr->second] = std::string((char*) buffer, currpos);
  }
}

void TextEncoder::addText(int attribute, std::string data) {
  auto attributePtr = _mapping.find(attribute);
  if (attributePtr != _mapping.end()) {
    uint64_t size = data.size();
    int currpos = attributePtr->second.length();
    uint8_t buffer[currpos + size + 5];
    attributePtr->second.copy((char*) buffer, currpos);
    buffer[currpos++] = 0;
    buffer[currpos++] = AttributeType::Text;
    putVarint(&buffer[currpos], size);
    currpos += varintLen(size);
    data.copy((char*) (buffer + currpos), data.size());
    _data[attributePtr->second] = std::string((char*) buffer, currpos + data.size());
  }
}

void TextEncoder::addBlob(int attribute, std::string data) {
  auto attributePtr = _mapping.find(attribute);
  if (attributePtr != _mapping.end()) {
    uint64_t size = data.size();
    int currpos = attributePtr->second.length();
    uint8_t buffer[currpos + size + 5];
    attributePtr->second.copy((char*) buffer, currpos);
    buffer[currpos++] = 0;
    buffer[currpos++] = AttributeType::Blob;
    putVarint(&buffer[currpos], size);
    currpos += varintLen(size);
    data.copy((char*) (buffer + currpos), data.size());
    _data[attributePtr->second] = std::string((char*) buffer, currpos + data.size());
  }
}

void TextEncoder::addNull(int attribute) {
  auto attributePtr = _mapping.find(attribute);
  if (attributePtr != _mapping.end()) {
    int currpos = attributePtr->second.length();
    uint8_t buffer[currpos + 2];
    attributePtr->second.copy((char*) buffer, currpos);
    buffer[currpos++] = 0;
    buffer[currpos++] = AttributeType::Null;
    _data[attributePtr->second] = std::string((char*) buffer, currpos);
  }
}

void TextEncoder::addValue(BinaryDecoder::Value &value) {
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
    char plain[37];
    uuid_unparse_lower((const unsigned char*) value.uuidValue().c_str(), plain);
    addText(value.id(), std::string(plain, 16));
    break;
  }
}

void TextEncoder::addValue(TextDecoder::Value &value) {
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

std::string TextEncoder::encodedData() {
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
} /* namespace Servers */
} /* namespace SyncServer */
