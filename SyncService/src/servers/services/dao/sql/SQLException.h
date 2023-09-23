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

#ifndef SQLEXCEPTION_H_
#define SQLEXCEPTION_H_

#include <iostream>
#include <stdexcept>
#include <string>

namespace SyncServer {
namespace Servers {
namespace Services {
namespace DAO {
namespace SQL {

class SQLException: public std::runtime_error {
public:
  SQLException(const SQLException &e) :
      SQLException(e.what(), e._errorCode) {
  }

  SQLException(const std::string &what, unsigned errorCode) :
      runtime_error(what), _errorCode(errorCode), _what(what) {
  }

  virtual ~SQLException() throw () {
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

class OutOfBoundsException: public SQLException {
public:
  OutOfBoundsException(const OutOfBoundsException &e) :
      SQLException(e.what(), 0) {
  }

  OutOfBoundsException(const std::string &what) :
      SQLException(what, 0) {
  }

  virtual ~OutOfBoundsException() throw () {
  }
};

class MissingParameterException: public SQLException {
public:
  MissingParameterException(const MissingParameterException &e) :
      SQLException(e.what(), 0) {
  }

  MissingParameterException(const std::string &what) :
      SQLException(what, 0) {
  }

  virtual ~MissingParameterException() throw () {
  }
};

class ConnectionUnavailable: public SQLException {
public:
  ConnectionUnavailable(const ConnectionUnavailable &e) :
      SQLException(e.what(), 0) {
  }

  ConnectionUnavailable(const std::string &what) :
      SQLException(what, 0) {
  }

  virtual ~ConnectionUnavailable() throw () {
  }
};

class NullColumnException: public SQLException {
public:
  NullColumnException(const NullColumnException &e) :
      SQLException(e.what(), 0) {
  }

  NullColumnException(const std::string &what) :
      SQLException(what, 0) {
  }

  virtual ~NullColumnException() throw () {
  }
};

class NoRightsException: public SQLException {
public:
  NoRightsException(const NoRightsException &e) :
      SQLException(e.what(), 0) {
  }

  NoRightsException(const std::string &what) :
      SQLException(what, 0) {
  }

  virtual ~NoRightsException() throw () {
  }
};

} /* namespace SQL */
} /* namespace DAO */
} /* namespace Services */
} /* namespace Servers */
} /* namespace SyncServer */

#endif /* SQLEXCEPTION_H_ */
