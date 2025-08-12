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
    using variant::variant; // inherit ctors (so Json j = true; works)
    JsonValue() : variant(nullptr) {}
};

class TinyJson
{
private:
    static std::pair<JsonValue, std::string_view> parseObject(const std::string_view sv);
    static std::pair<JsonValue, std::string_view> parseArray(const std::string_view sv);
    static std::pair<JsonValue, std::string_view> parseString(const std::string_view sv);
    static std::pair<JsonValue, std::string_view> parseNumber(const std::string_view sv);
    static std::pair<JsonValue, std::string_view> parseBool(const std::string_view sv);
    static std::pair<JsonValue, std::string_view> parseNull(const std::string_view sv);
    static std::pair<JsonValue, std::string_view> parseJson(const std::string_view sv);
    static std::string_view skipWhitespace(std::string_view s);

public:
    TinyJson() = default;
    TinyJson(const TinyJson &other) = default;
    TinyJson &operator=(const TinyJson &other) = default;
    ~TinyJson() = default;

    static JsonValue parse(const std::string &jsonString);
    static std::string stringify(const JsonValue &jsonValue, int indent = 2);

    template <typename T>
    static T as(const JsonValue &jsonValue);

    template <typename T>
    static T as(const JsonValue &jsonValue, T &defaultValue);

    template <typename T>
    static T deserialize(const std::string &jsonString);

    template <typename T>
    static std::string serialize(const T &value, int indent = 2);
};