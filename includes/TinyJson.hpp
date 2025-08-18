#pragma once

#include <variant>
#include <string>
#include <vector>
#include <unordered_map>
#include <string_view>
#include <charconv>
#include <stdexcept>
#include <type_traits>

struct JsonValue;

using JsonArray = std::vector<JsonValue>;
using JsonObject = std::unordered_map<std::string, JsonValue>;

struct JsonValue : std::variant<
                       std::nullptr_t,
                       bool,
                       double,
                       std::string,
                       JsonArray,
                       JsonObject>
{
    using variant::variant;
    JsonValue() : variant(nullptr) {}
};

class TinyJson
{
public:
    TinyJson() = default;
    TinyJson(const TinyJson &other) = default;
    TinyJson &operator=(const TinyJson &other) = default;
    ~TinyJson() = default;

    static JsonValue parse(const std::string &jsonString);

    template <typename T>
    static T as(const JsonValue &jsonValue);

    template <typename T>
    static T as(const JsonValue &jsonValue, T defaultValue);
};