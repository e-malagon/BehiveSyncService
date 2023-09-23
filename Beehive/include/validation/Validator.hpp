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
#include <exprtk/exprtk.hpp>
#include <services/ServiceException.hpp>

namespace Beehive {
namespace Services {
namespace Utils {

class Validator {
public:
  Validator(std::string &attribute, std::string &expression, int type) :
      _numericValue(0.0) {
    if (type == SqLite::AttributeType::Integer || type == SqLite::AttributeType::Real) {
      _symbol_table.add_variable("value", _numericValue);
      _expression.register_symbol_table(_symbol_table);
    } else if (type == SqLite::AttributeType::Text) {
      _symbol_table.add_stringvar("value", _stringValue);
      _expression.register_symbol_table(_symbol_table);
    }
    exprtk::parser<double> parser;
    if (!parser.compile(expression, _expression))
      throw Beehive::Services::InvalidSchemaException("Invalid expression check on attributes " + attribute);
  }

  virtual ~Validator() {
  }

  bool isValidValue(double value) {
    _numericValue = value;
    return _expression.value();
  }

  bool isValidValue(std::string value) {
    _stringValue = value;
    return _expression.value();
  }

private:
  std::string _stringValue;
  double _numericValue;
  exprtk::symbol_table<double> _symbol_table;
  exprtk::expression<double> _expression;
};

} /* namespace Utils */
} /* namespace Services */
} /* namespace Beehive */
