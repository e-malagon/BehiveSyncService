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

#include <json/json.hpp>
#include <type_traits>

template <typename E>
constexpr auto to_underlying(E e) noexcept
{
    return static_cast<std::underlying_type_t<E>>(e);
}

namespace nlohmann {
template <typename T>
struct adl_serializer<std::shared_ptr<T>> {
    static void to_json(json &j, const std::shared_ptr<T> &opt) {
        if (!opt)
            j = nullptr;
        else
            j = *opt;
    }

    static std::shared_ptr<T> from_json(const json &j) {
        if (j.is_null())
            return std::unique_ptr<T>();
        else
            return std::unique_ptr<T>(new T(j.get<T>()));
    }
};
}  // namespace nlohmann

namespace Beehive {
namespace Services {
namespace Config {

inline nlohmann::json get_untyped(const nlohmann::json &j, const char *property) {
    if (j.find(property) != j.end()) {
        return j.at(property).get<nlohmann::json>();
    }
    return nlohmann::json();
}

inline nlohmann::json get_untyped(const nlohmann::json &j, std::string property) {
    return get_untyped(j, property.data());
}

template <typename T>
inline std::shared_ptr<T> get_optional(const nlohmann::json &j, const char *property) {
    if (j.find(property) != j.end()) {
        return j.at(property).get<std::shared_ptr<T>>();
    }
    return std::shared_ptr<T>();
}

template <typename T>
inline std::shared_ptr<T> get_optional(const nlohmann::json &j, std::string property) {
    return get_optional<T>(j, property.data());
}

template <typename T>
inline T get_optional(const nlohmann::json &j, std::string property, T default_value) {
    std::shared_ptr<T> current = get_optional<T>(j, property.data());
    if(!current)
        return default_value;
    else
        return *current;
}

} /* namespace Config */
} /* namespace Services */
} /* namespace Beehive */

namespace Beehive {
namespace Services {
namespace Entities {

inline nlohmann::json get_untyped(const nlohmann::json &j, const char *property) {
    if (j.find(property) != j.end()) {
        return j.at(property).get<nlohmann::json>();
    }
    return nlohmann::json();
}

inline nlohmann::json get_untyped(const nlohmann::json &j, std::string property) {
    return get_untyped(j, property.data());
}

template <typename T>
inline std::shared_ptr<T> get_optional(const nlohmann::json &j, const char *property) {
    if (j.find(property) != j.end()) {
        return j.at(property).get<std::shared_ptr<T>>();
    }
    return std::shared_ptr<T>();
}

template <typename T>
inline std::shared_ptr<T> get_optional(const nlohmann::json &j, std::string property) {
    return get_optional<T>(j, property.data());
}

} /* namespace Entities */
} /* namespace Services */
} /* namespace Beehive */
