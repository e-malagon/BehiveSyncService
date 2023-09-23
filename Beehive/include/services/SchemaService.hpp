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

#include <config/Context.hpp>
#include <dao/EntityDAO.hpp>

#include <memory>
#include <mutex>
#include <utility>
#include <vector>

namespace Beehive {
namespace Services {

class SchemaService {
   public:
    SchemaService() {
    }
    virtual ~SchemaService() {
    }

    std::string postContext(const std::string &body);
    std::string getContext(const std::string &context);
    std::string getContexts();
    std::string putContext(const std::string &body);
    void deleteContext(const std::string &context);
    void linkContext(const std::string &context, const std::string &link);
    void unlinkContext(const std::string &context, const std::string &link);
    std::string getLinkedVersions(const std::string &context);
    std::string getLinkedVersion(const std::string &context, const std::string &version);
    static Config::Context getContext(const std::string &context, int version);

    static Config::Context loadContextSchemas(const std::string schema);


   private:
    DAO::EntityDAO _entityDAO;
};

} /* namespace Services */
} /* namespace Beehive */
