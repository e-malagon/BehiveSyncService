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

#ifndef INVALIDDATAEXCEPTION_H_
#define INVALIDDATAEXCEPTION_H_

#include <stdexcept>
#include <string>

namespace SyncServer {
namespace Servers {
namespace Services {
namespace SqLite {

class DecoderException: public std::runtime_error {
public:
  DecoderException(const DecoderException &e) :
      DecoderException(e.what(), e._errorCode) {
  }
  DecoderException(const std::string &what, unsigned errorCode) :
    std::runtime_error(what), _errorCode(errorCode), _what(what) {
  }
  virtual ~DecoderException() throw () {
  }
  unsigned errorCode() const {
    return _errorCode;
  }
  virtual const char* what() const noexcept override {
    return _what.c_str();
  }
private:
  unsigned _errorCode;
  const std::string _what;
};

class NavigationException: public DecoderException {
public:
  NavigationException(const NavigationException &e) :
    DecoderException(e.what(), 0) {
  }

  NavigationException(const std::string &what) :
    DecoderException(what, 0) {
  }

  virtual ~NavigationException() throw () {
  }
};

} /* namespace SqLite */
} /* namespace Services */
} /* namespace Servers */
} /* namespace SyncServer */

#endif /* INVALIDDATAEXCEPTION_H_ */
