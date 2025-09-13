#pragma once

#include <variant>
#include <string>
#include <cmath>
#include <vector>
#include <unordered_map>
#include <string_view>
#include <charconv>
#include <stdexcept>
#include <type_traits>

struct JsonValue;

/**
 * @brief A type alias for a JSON array, represented as a vector of JsonValue.
 */
using JsonArray = std::vector<JsonValue>;
/**
 * @brief A type alias for a JSON object, represented as an unordered map from strings to JsonValue.
 */
using JsonObject = std::unordered_map<std::string, JsonValue>;

/**
 * @brief A variant type that can hold any valid JSON value.
 * @details This includes null, boolean, number, string, array, and object types.
 */
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

/**
 * @brief A minimalistic JSON parser and serializer.
 * @details This class provides static methods to parse JSON strings into a variant-based representation
 *          and to convert that representation back into JSON strings. It supports basic JSON types including
 *          null, boolean, number, string, array, and object.
 */
class TinyJson
{
public:
    TinyJson() = default;
    TinyJson(const TinyJson &other) = default;
    TinyJson &operator=(const TinyJson &other) = default;
    ~TinyJson() = default;

    /**
     * @brief Parse a JSON string into a JsonValue variant.
     * @param jsonString The JSON string to parse.
     * @return A JsonValue representing the parsed JSON structure.
     * @throws std::runtime_error if the JSON string is malformed.
     */
    static JsonValue parse(const std::string &jsonString);

    /**
     * @brief Get the value of a JsonValue as a specific type, without a default value.
     * @param jsonValue The JsonValue to convert.
     * @return The converted value.
     * @throws std::bad_variant_access if the conversion fails.
     */
    template <typename T>
    static T as(const JsonValue &jsonValue);

    /**
     * @brief Get the value of a JsonValue as a specific type, with a default value.
     * @param jsonValue The JsonValue to convert.
     * @param defaultValue The default value to return if the conversion fails.
     * @return The converted value or the default value.
     */
    template <typename T>
    static T as(const JsonValue &jsonValue, T defaultValue);
};

#include "TinyJson.tpp"