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

#ifndef SERVICEEXCEPTION_H_
#define SERVICEEXCEPTION_H_

#include <stdexcept>
#include <string>

namespace SyncServer {
namespace Servers {
namespace Services {

class ServiceException: public std::runtime_error {
public:
  ServiceException(const ServiceException &e) :
      ServiceException(e.what(), e._errorCode) {
  }

  ServiceException(const std::string &what, unsigned errorCode) :
      runtime_error(what), _errorCode(errorCode), _what(what) {
  }

  virtual ~ServiceException() throw () {
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

class InvalidSchemaException: public ServiceException {
public:
  InvalidSchemaException(const InvalidSchemaException &e) :
      ServiceException(e.what(), 0) {
  }

  InvalidSchemaException(const std::string &what) :
      ServiceException(what, 0) {
  }

  virtual ~InvalidSchemaException() throw () {
  }
};

class AuthenticationException: public ServiceException {
public:
  AuthenticationException(const AuthenticationException &e) :
      ServiceException(e.what(), 0) {
  }

  AuthenticationException(const std::string &what) :
      ServiceException(what, 0) {
  }

  virtual ~AuthenticationException() throw () {
  }
};

class NotEnoughRightsException: public ServiceException {
public:
  NotEnoughRightsException(const NotEnoughRightsException &e) :
      ServiceException(e.what(), 0) {
  }

  NotEnoughRightsException(const std::string &what) :
      ServiceException(what, 0) {
  }

  virtual ~NotEnoughRightsException() throw () {
  }
};

class NotExistException: public ServiceException {
public:
  NotExistException(const NotExistException &e) :
      ServiceException(e.what(), 0) {
  }

  NotExistException(const std::string &what) :
      ServiceException(what, 0) {
  }

  virtual ~NotExistException() throw () {
  }
};

class DataValidationException: public ServiceException {
public:
  DataValidationException(const DataValidationException &e) :
      ServiceException(e.what(), 0) {
  }

  DataValidationException(const std::string &what) :
      ServiceException(what, 0) {
  }

  virtual ~DataValidationException() throw () {
  }
};

class OperationNotAllowedException: public ServiceException {
public:
  OperationNotAllowedException(const OperationNotAllowedException &e) :
      ServiceException(e.what(), 0) {
  }

  OperationNotAllowedException(const std::string &what) :
      ServiceException(what, 0) {
  }

  virtual ~OperationNotAllowedException() throw () {
  }
};

class SchemaDefinitionException: public ServiceException {
public:
  SchemaDefinitionException(const SchemaDefinitionException &e) :
      ServiceException(e.what(), 0) {
  }

  SchemaDefinitionException(const std::string &what) :
      ServiceException(what, 0) {
  }

  virtual ~SchemaDefinitionException() throw () {
  }
};


} /* namespace Services */
} /* namespace Servers */
} /* namespace SyncServer */

#endif /* SERVICEEXCEPTION_H_ */
