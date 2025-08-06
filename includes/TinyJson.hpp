#pragma once

#include <variant>
#include <string>
#include <vector>
#include <unordered_map>

using JsonValue = std::variant<
    std::nullptr_t,
    bool,
    double,
    std::string,
    std::vector<JsonValue>,
    std::unordered_map<std::string, JsonValue>>;

class TinyJson
{
public:
    TinyJson() = default;
    TinyJson(const TinyJson &other) = default;
    TinyJson &operator=(const TinyJson &other) = default;
    ~TinyJson() = default;

    static JsonValue parse(const std::string &jsonString);
    static std::string stringify(const JsonValue &jsonValue, int indent = 2);

    template <typename T>
    static T as(const JsonValue &jsonValue, T defaultValue = T());

    template <typename T>
    static T deserialize(const std::string &jsonString);

    template <typename T>
    static std::string serialize(const T &value, int indent = 2);
};