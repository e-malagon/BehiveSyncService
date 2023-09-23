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


#include <validation/TransactionsManager.hpp>

#include <uuid/uuid.h>
#include <nanolog/NanoLog.hpp>

namespace Beehive {
namespace Services {
namespace Utils {

sol::table TransactionsManager::readEntity(std::string entity, sol::table key) {
    sol::table result = _lua.create_table();
    auto idPtr = _context->entitiesName2UUID.find(entity);
    if (idPtr == _context->entitiesName2UUID.end()) {
        LOG_ERROR << "Entity " << entity << " not found";
        return 0;
    }
    const auto &entityPtr = _context->entities.find(idPtr->second);
    if (entityPtr == _context->entities.end()) {
        LOG_ERROR << "Entity " << entity << " not found";
        return result;
    }
    Config::Entity &ent = entityPtr->second;
    if (key.size() == 0) {
        int index = 1;
        std::vector<Entities::KeyData> keyDataV = _entityDAO->read(_lua.get<uint32_t>("idDataset"), key, ent, _context->uuid);
        for (Entities::KeyData keyData : keyDataV) {
            SqLite::BinaryDecoder oldPK(keyData.oldPK().data(), keyData.oldPK().size());
            SqLite::BinaryDecoder oldData(keyData.oldData().data(), keyData.oldData().size());
            result[index] = _lua.create_table();
            for (SqLite::BinaryDecoder::Value &value : oldPK) {
                auto keyPtr = ent.keys.find(value.id());
                if (keyPtr != ent.keys.end())
                    switch (value.type()) {
                        case SqLite::AttributeType::Integer:
                            result[index][keyPtr->second.name] = value.integerValue();
                            break;
                        case SqLite::AttributeType::Text:
                            result[index][keyPtr->second.name] = value.textValue();
                            break;
                        case SqLite::AttributeType::Blob:
                            result[index][keyPtr->second.name] = value.blobValue();
                            break;
                    }
            }
            for (SqLite::BinaryDecoder::Value &value : oldData) {
                auto attributePtr = ent.attributes.find(value.id());
                if (attributePtr != ent.attributes.end())
                    switch (value.type()) {
                        case SqLite::AttributeType::Integer:
                            result[index][attributePtr->second.name] = value.integerValue();
                            break;
                        case SqLite::AttributeType::Real:
                            result[index][attributePtr->second.name] = value.realValue();
                            break;
                        case SqLite::AttributeType::Text:
                            result[index][attributePtr->second.name] = value.textValue();
                            break;
                        case SqLite::AttributeType::Blob:
                            result[index][attributePtr->second.name] = value.blobValue();
                            break;
                        case SqLite::AttributeType::UuidV1:
                            char plain[37];
                            uuid_unparse_lower((const unsigned char *)value.uuidValue().c_str(), plain);
                            result[index][attributePtr->second.name] = std::string(plain, 16);
                            break;
                    }
            }
            index++;
        }
    } else {
        int index = 1;
        for (size_t i = 1; i <= entity.size(); i++) {
            sol::object rdata = key[i];
            if (rdata.get_type() == sol::type::table) {
                sol::table inner = rdata.as<sol::table>();
                std::vector<Entities::KeyData> keyDataV = _entityDAO->read(_lua.get<uint32_t>("idDataset"), inner, ent, _context->uuid);
                for (Entities::KeyData keyData : keyDataV) {
                    SqLite::BinaryDecoder oldPK(keyData.oldPK().data(), keyData.oldPK().size());
                    SqLite::BinaryDecoder oldData(keyData.oldData().data(), keyData.oldData().size());
                    result[index] = _lua.create_table();
                    for (SqLite::BinaryDecoder::Value &value : oldPK) {
                        auto keyPtr = ent.keys.find(value.id());
                        if (keyPtr != ent.keys.end())
                            switch (value.type()) {
                                case SqLite::AttributeType::Integer:
                                    result[index][keyPtr->second.name] = value.integerValue();
                                    break;
                                case SqLite::AttributeType::Text:
                                    result[index][keyPtr->second.name] = value.textValue();
                                    break;
                                case SqLite::AttributeType::Blob:
                                    result[index][keyPtr->second.name] = value.blobValue();
                                    break;
                            }
                    }
                    for (SqLite::BinaryDecoder::Value &value : oldData) {
                        auto attributePtr = ent.attributes.find(value.id());
                        if (attributePtr != ent.attributes.end())
                            switch (value.type()) {
                                case SqLite::AttributeType::Integer:
                                    result[index][attributePtr->second.name] = value.integerValue();
                                    break;
                                case SqLite::AttributeType::Real:
                                    result[index][attributePtr->second.name] = value.realValue();
                                    break;
                                case SqLite::AttributeType::Text:
                                    result[index][attributePtr->second.name] = value.textValue();
                                    break;
                                case SqLite::AttributeType::Blob:
                                    result[index][attributePtr->second.name] = value.blobValue();
                                    break;
                            }
                    }
                    index++;
                }
            }
        }
    }
    return result;
}

int TransactionsManager::saveEntity(std::string entity, sol::table data) {
    auto idPtr = _context->entitiesName2UUID.find(entity);
    if (idPtr == _context->entitiesName2UUID.end()) {
        LOG_ERROR << "Entity " << entity << " not found";
        return 0;
    }
    const auto &entityPtr = _context->entities.find(idPtr->second);
    if (entityPtr == _context->entities.end()) {
        LOG_ERROR << "Entity " << entity << " not found";
        return 0;
    }
    Config::Entity &ent = entityPtr->second;
    if (data.size() == 0) {
        return _entityDAO->save(_lua.get<uint32_t>("idDataset"), data, ent, _context->uuid);
    } else {
        int total = 0;
        for (size_t i = 1; i <= entity.size(); i++) {
            sol::object rdata = data[i];
            if (rdata.get_type() == sol::type::table) {
                sol::table inner = rdata.as<sol::table>();
                total += _entityDAO->save(_lua.get<uint32_t>("idDataset"), inner, ent, _context->uuid);
            }
        }
        return total;
    }
}

int TransactionsManager::updateEntity(std::string entity, sol::table data) {
    auto idPtr = _context->entitiesName2UUID.find(entity);
    if (idPtr == _context->entitiesName2UUID.end()) {
        LOG_ERROR << "Entity " << entity << " not found";
        return 0;
    }
    const auto &entityPtr = _context->entities.find(idPtr->second);
    if (entityPtr == _context->entities.end()) {
        LOG_ERROR << "Entity " << entity << " not found";
        return 0;
    }
    Config::Entity &ent = entityPtr->second;
    if (data.size() == 0) {
        return 0;
    } else if (data.size() == 2) {
        sol::object keyData = data[1];
        sol::object dataData = data[2];
        if (keyData.get_type() == sol::type::table && dataData.get_type() == sol::type::table) {
            sol::table innerKey = keyData.as<sol::table>();
            sol::table innerData = dataData.as<sol::table>();
            if (innerKey.size() == 0 && innerData.size() == 0) {
                return _entityDAO->update(_lua.get<uint32_t>("idDataset"), innerKey, innerData, ent, _context->uuid);
            }
        }
    }
    int total = 0;
    for (size_t i = 1; i <= entity.size(); i++) {
        sol::object rdata = data[i];
        if (rdata.get_type() == sol::type::table) {
            sol::table inner = rdata.as<sol::table>();
            if (inner.size() == 2) {
                sol::object keyData = inner[1];
                sol::object dataData = inner[2];
                if (keyData.get_type() == sol::type::table && dataData.get_type() == sol::type::table) {
                    sol::table innerKey = keyData.as<sol::table>();
                    sol::table innerData = dataData.as<sol::table>();
                    if (innerKey.size() == 0 && innerData.size() == 0) {
                        total += _entityDAO->update(_lua.get<uint32_t>("idDataset"), innerKey, innerData, ent, _context->uuid);
                    } else {
                        LOG_ERROR << "Invalid inner array size index " << i;
                    }
                }
            } else {
                LOG_ERROR << "Invalid inner array size index " << i;
            }
        }
    }
    return total;

    return 0;
}

int TransactionsManager::removeEntity(std::string entity, sol::table key) {
    auto idPtr = _context->entitiesName2UUID.find(entity);
    if (idPtr == _context->entitiesName2UUID.end()) {
        LOG_ERROR << "Entity " << entity << " not found";
        return 0;
    }
    const auto &entityPtr = _context->entities.find(idPtr->second);
    if (entityPtr == _context->entities.end()) {
        LOG_ERROR << "Entity " << entity << " not found";
        return 0;
    }
    Config::Entity &ent = entityPtr->second;
    if (key.size() == 0) {
           return _entityDAO->remove(_lua.get<uint32_t>("idDataset"), key, ent, _context->uuid);
    } else {
        int total = 0;
        for (size_t i = 1; i <= entity.size(); i++) {
            sol::object rdata = key[i];
            if (rdata.get_type() == sol::type::table) {
                sol::table inner = rdata.as<sol::table>();
                total += _entityDAO->remove(_lua.get<uint32_t>("idDataset"), inner, ent, _context->uuid);
            }
        }
        return total;
    }
    return 0;
}

void TransactionsManager::loadValidation(std::string transaction, std::string script) {
    _onValidation[transaction] = _lua.load(script);
    if (!_onValidation[transaction].valid()) {
        sol::error err = _onValidation[transaction];
        LOG_WARN << "Failed to load pre execution script " << transaction << " " << err.what();
        _onValidation.erase(transaction);
    }
}

void TransactionsManager::loadCommit(std::string transaction, std::string script) {
    _onCommit[transaction] = _lua.load(script);
    if (!_onCommit[transaction].valid()) {
        sol::error err = _onCommit[transaction];
        LOG_WARN << "Failed to load post execution script " << transaction << " " << err.what();
        _onCommit.erase(transaction);
    }
}

bool TransactionsManager::executeValidation(Entities::Header &header) {
    auto onValidation = _onValidation.find(header.transactionUUID());
    auto onCommit = _onCommit.find(header.transactionUUID());
    if (onValidation != _onValidation.end() || onCommit != _onCommit.end()) {
        _lua["idDataset"] = header.idDataset();
        _lua.create_named_table("data");
        int index = 1;
        for (Entities::Change &change : header.changes()) {
            auto tbl = _context->entities.find(change.entityName());
            if (tbl != _context->entities.end()) {
                _lua["data"][index] = _lua.create_table();
                _lua["data"][index]["entity"] = tbl->second.name;
                switch (change.operation()) {
                    case SqLite::Operation::Insert: {
                        _lua["data"][index]["operation"] = "Add";
                        SqLite::BinaryDecoder decoderNewPK(change.newPK().data(), change.newPK().size());
                        _lua["data"][index]["new"] = _lua.create_table();
                        for (SqLite::BinaryDecoder::Value &value : decoderNewPK) {
                            auto keyPtr = tbl->second.keys.find(value.id());
                            if (keyPtr != tbl->second.keys.end())
                                switch (value.type()) {
                                    case SqLite::AttributeType::Integer:
                                        _lua["data"][index]["new"][keyPtr->second.name] = value.integerValue();
                                        break;
                                    case SqLite::AttributeType::Text:
                                        _lua["data"][index]["new"][keyPtr->second.name] = value.textValue();
                                        break;
                                    case SqLite::AttributeType::Blob:
                                        _lua["data"][index]["new"][keyPtr->second.name] = value.blobValue();
                                        break;
                                }
                        }
                        SqLite::BinaryDecoder decoderNewData(change.newData().data(), change.newData().size());
                        for (SqLite::BinaryDecoder::Value &value : decoderNewData) {
                            auto attributePtr = tbl->second.attributes.find(value.id());
                            if (attributePtr != tbl->second.attributes.end())
                                switch (value.type()) {
                                    case SqLite::AttributeType::Integer:
                                        _lua["data"][index]["new"][attributePtr->second.name] = value.integerValue();
                                        break;
                                    case SqLite::AttributeType::Real:
                                        _lua["data"][index]["new"][attributePtr->second.name] = value.realValue();
                                        break;
                                    case SqLite::AttributeType::Text:
                                        _lua["data"][index]["new"][attributePtr->second.name] = value.textValue();
                                        break;
                                    case SqLite::AttributeType::Blob:
                                        _lua["data"][index]["new"][attributePtr->second.name] = value.blobValue();
                                        break;
                                }
                        }
                    } break;
                    case SqLite::Operation::Update: {
                        _lua["data"][index]["operation"] = "Update";
                        SqLite::BinaryDecoder decoderNewPK(change.newPK().data(), change.newPK().size());
                        _lua["data"][index]["new"] = _lua.create_table();
                        for (SqLite::BinaryDecoder::Value &value : decoderNewPK) {
                            auto keyPtr = tbl->second.keys.find(value.id());
                            if (keyPtr != tbl->second.keys.end())
                                switch (value.type()) {
                                    case SqLite::AttributeType::Integer:
                                        _lua["data"][index]["new"][keyPtr->second.name] = value.integerValue();
                                        break;
                                    case SqLite::AttributeType::Text:
                                        _lua["data"][index]["new"][keyPtr->second.name] = value.textValue();
                                        break;
                                    case SqLite::AttributeType::Blob:
                                        _lua["data"][index]["new"][keyPtr->second.name] = value.blobValue();
                                        break;
                                }
                        }
                        SqLite::BinaryDecoder decoderNewData(change.newData().data(), change.newData().size());
                        for (SqLite::BinaryDecoder::Value &value : decoderNewData) {
                            auto attributePtr = tbl->second.attributes.find(value.id());
                            if (attributePtr != tbl->second.attributes.end())
                                switch (value.type()) {
                                    case SqLite::AttributeType::Integer:
                                        _lua["data"][index]["new"][attributePtr->second.name] = value.integerValue();
                                        break;
                                    case SqLite::AttributeType::Real:
                                        _lua["data"][index]["new"][attributePtr->second.name] = value.realValue();
                                        break;
                                    case SqLite::AttributeType::Text:
                                        _lua["data"][index]["new"][attributePtr->second.name] = value.textValue();
                                        break;
                                    case SqLite::AttributeType::Blob:
                                        _lua["data"][index]["new"][attributePtr->second.name] = value.blobValue();
                                        break;
                                }
                        }
                        SqLite::BinaryDecoder decoderOldPK(change.oldPK().data(), change.oldPK().size());
                        _lua["data"][index]["old"] = _lua.create_table();
                        for (SqLite::BinaryDecoder::Value &value : decoderOldPK) {
                            auto keyPtr = tbl->second.keys.find(value.id());
                            if (keyPtr != tbl->second.keys.end())
                                switch (value.type()) {
                                    case SqLite::AttributeType::Integer:
                                        _lua["data"][index]["old"][keyPtr->second.name] = value.integerValue();
                                        break;
                                    case SqLite::AttributeType::Text:
                                        _lua["data"][index]["old"][keyPtr->second.name] = value.textValue();
                                        break;
                                    case SqLite::AttributeType::Blob:
                                        _lua["data"][index]["old"][keyPtr->second.name] = value.blobValue();
                                        break;
                                }
                        }
                        SqLite::BinaryDecoder decoderOldData(change.oldData().data(), change.oldData().size());
                        for (SqLite::BinaryDecoder::Value &value : decoderOldData) {
                            auto attributePtr = tbl->second.attributes.find(value.id());
                            if (attributePtr != tbl->second.attributes.end())
                                switch (value.type()) {
                                    case SqLite::AttributeType::Integer:
                                        _lua["data"][index]["old"][attributePtr->second.name] = value.integerValue();
                                        break;
                                    case SqLite::AttributeType::Real:
                                        _lua["data"][index]["old"][attributePtr->second.name] = value.realValue();
                                        break;
                                    case SqLite::AttributeType::Text:
                                        _lua["data"][index]["old"][attributePtr->second.name] = value.textValue();
                                        break;
                                    case SqLite::AttributeType::Blob:
                                        _lua["data"][index]["old"][attributePtr->second.name] = value.blobValue();
                                        break;
                                }
                        }
                    } break;
                    case SqLite::Operation::Delete: {
                        _lua["data"][index]["operation"] = "Remove";
                        SqLite::BinaryDecoder decoderOldPK(change.oldPK().data(), change.oldPK().size());
                        _lua["data"][index]["old"] = _lua.create_table();
                        for (SqLite::BinaryDecoder::Value &value : decoderOldPK) {
                            auto keyPtr = tbl->second.keys.find(value.id());
                            if (keyPtr != tbl->second.keys.end())
                                switch (value.type()) {
                                    case SqLite::AttributeType::Integer:
                                        _lua["data"][index]["old"][keyPtr->second.name] = value.integerValue();
                                        break;
                                    case SqLite::AttributeType::Text:
                                        _lua["data"][index]["old"][keyPtr->second.name] = value.textValue();
                                        break;
                                    case SqLite::AttributeType::Blob:
                                        _lua["data"][index]["old"][keyPtr->second.name] = value.blobValue();
                                        break;
                                }
                        }
                        SqLite::BinaryDecoder decoderOldData(change.oldData().data(), change.oldData().size());
                        for (SqLite::BinaryDecoder::Value &value : decoderOldData) {
                            auto attributePtr = tbl->second.attributes.find(value.id());
                            if (attributePtr != tbl->second.attributes.end())
                                switch (value.type()) {
                                    case SqLite::AttributeType::Integer:
                                        _lua["data"][index]["old"][attributePtr->second.name] = value.integerValue();
                                        break;
                                    case SqLite::AttributeType::Real:
                                        _lua["data"][index]["old"][attributePtr->second.name] = value.realValue();
                                        break;
                                    case SqLite::AttributeType::Text:
                                        _lua["data"][index]["old"][attributePtr->second.name] = value.textValue();
                                        break;
                                    case SqLite::AttributeType::Blob:
                                        _lua["data"][index]["old"][attributePtr->second.name] = value.blobValue();
                                        break;
                                }
                        }
                    } break;
                }
                index++;
            }
        }
        if (onValidation != _onValidation.end()) {
            return _onValidation[header.transactionUUID()]();
        } else
            return true;
    } else
        return true;
}

bool TransactionsManager::executeCommit(Entities::Header &header) {
    bool result = true;
    auto onCommit = _onCommit.find(header.transactionUUID());
    if (onCommit != _onCommit.end())
        result = _onCommit[header.transactionUUID()]();
    _lua.set("idDataset", sol::lua_nil);
    _lua.set("idTransaction", sol::lua_nil);
    _lua.set("data", sol::lua_nil);
    return result;
}

} /* namespace Utils */
} /* namespace Services */
} /* namespace Beehive */
