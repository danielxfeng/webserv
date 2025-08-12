#include "TinyJson.hpp"

bool isJsonWhitespace(char c)
{
    return c == ' ' || c == '\t' || c == '\n' || c == '\r';
}

std::string_view TinyJson::skipWhitespace(std::string_view sv)
{
    size_t i = 0;
    while (i < sv.size() && isJsonWhitespace(sv[i]))
        ++i;
    sv.remove_prefix(i);
    return sv;
}

JsonValue TinyJson::parse(const std::string &jsonString)
{
    std::string_view sv(jsonString);
    sv = skipWhitespace(sv);

    if (sv.empty())
        throw std::runtime_error("Empty JSON string");

    JsonValue value{};
    std::string_view rest;
    std::tie(value, rest) = parseJson(sv);

    rest = skipWhitespace(rest);
    if (!rest.empty())
        throw std::runtime_error("Invalid JSON format");

    return value;
}

std::pair<JsonValue, std::string_view> TinyJson::parseJson(const std::string_view sv)
{
    JsonValue value{};
    std::string_view rest;

    if (sv.empty())
        throw std::runtime_error("Empty JSON string");

    if (sv.front() == '{')
        return parseObject(sv);
    else if (sv.front() == '[')
        return parseArray(sv);
    else if (sv.front() == '"')
        return parseString(sv);
    else if (isdigit(sv.front()) || sv.front() == '-')
        return parseNumber(sv);
    else if (sv.front() == 't' || sv.front() == 'f')
        return parseBool(sv);
    else if (sv.front() == 'n')
        return parseNull(sv);
    else
        throw std::runtime_error("Invalid JSON format");
}

std::pair<JsonValue, std::string_view> TinyJson::parseBool(const std::string_view sv)
{
    if (sv.empty())
        throw std::runtime_error("Empty JSON string");

    if (sv.starts_with("true"))
        return {JsonValue(true), sv.substr(4)};
    if (sv.starts_with("false"))
        return {JsonValue(false), sv.substr(5)};
    throw std::runtime_error("Invalid JSON boolean value");
}

std::pair<JsonValue, std::string_view> TinyJson::parseNull(const std::string_view sv)
{
    if (sv.empty())
        throw std::runtime_error("Empty JSON string");

    if (sv.starts_with("null"))
        return {JsonValue(nullptr), sv.substr(4)};
    throw std::runtime_error("Invalid JSON null value");
}

bool isLeadingZeros(const std::string_view sv)
{
    std::string_view new_sv = sv;

    if (new_sv.empty())
        return false;

    if (new_sv.front() == '-')
        new_sv.remove_prefix(1); // Skip the negative sign

    if (new_sv.empty() || new_sv.front() != '0')
        return false;

    if (new_sv.size() == 1)
        return false;

    new_sv.remove_prefix(1); // Skip the leading zero

    // Check if the string is just "0" or has leading zeros
    return isdigit(new_sv[0]);
}

std::pair<JsonValue, std::string_view> TinyJson::parseNumber(const std::string_view sv)
{
    if (sv.empty())
        throw std::runtime_error("Empty JSON string");

    if (isLeadingZeros(sv))
        throw std::runtime_error("Invalid JSON number format: leading zeros are not allowed");

    double result{};

    const auto end_pos = sv.data() + sv.size();
    auto [ptr, ec] = std::from_chars(sv.data(), end_pos, result, std::chars_format::general);

    if (ec != std::errc())
    {
        if (ec == std::errc::result_out_of_range)
            throw std::out_of_range("Number out of range");
        throw std::runtime_error("Invalid number format");
    }

    return {JsonValue(result), std::string_view(ptr, end_pos - ptr)};
}

std::pair<JsonValue, std::string_view> TinyJson::parseString(const std::string_view sv)
{
    if (sv.empty() || sv.front() != '"')
        throw std::runtime_error("Invalid JSON string format");

    size_t end_pos = 1; // Skip the opening quote
    std::string buf;

    // Escape handling
    while (end_pos < sv.size())
    {
        unsigned char c = sv[end_pos];

        if (c == '"')
            return {JsonValue(std::move(buf)), sv.substr(end_pos + 1)};

        if (c < 0x20)
            throw std::runtime_error("Control characters not allowed in JSON strings");

        if (c != '\\')
            buf.push_back(c);
        else
        {
            ++end_pos;
            if (end_pos >= sv.size())
                throw std::runtime_error("Unterminated JSON string");

            unsigned char e = sv[end_pos];
            switch (e)
            {
            case '"':
            case '\\':
            case '/':
                buf.push_back(e);
                break;
            case 'b':
                buf.push_back('\b');
                break;
            case 'f':
                buf.push_back('\f');
                break;
            case 'n':
                buf.push_back('\n');
                break;
            case 'r':
                buf.push_back('\r');
                break;
            case 't':
                buf.push_back('\t');
                break;
            case 'u':
                throw std::runtime_error("Unicode escapes not implemented yet"); // TODO
            default:
                throw std::runtime_error("Invalid escape sequence");
            }
        }

        ++end_pos;
    }

    throw std::runtime_error("Unterminated JSON string");
}

std::pair<JsonValue, std::string_view> TinyJson::parseArray(const std::string_view sv)
{
    JsonArray array;

    if (sv.empty() || sv.front() != '[')
        throw std::runtime_error("Invalid JSON array format");

    std::string_view rest = sv.substr(1); // Skip the opening bracket

    while (true)
    {
        rest = skipWhitespace(rest);
        if (rest.empty())
            throw std::runtime_error("Unterminated JSON array");

        if (rest.front() == ']')
            return {JsonValue(std::move(array)), rest.substr(1)};

        JsonValue value;

        std::tie(value, rest) = parseJson(rest);
        array.push_back(std::move(value));
        rest = skipWhitespace(rest);

        if (rest.empty())
            throw std::runtime_error("Unterminated JSON array");
        else if (rest.front() == ',')
        {
            rest.remove_prefix(1); // Skip the comma
            rest = skipWhitespace(rest);
            if (rest.empty() || rest.front() == ']')
                throw std::runtime_error("Invalid JSON array format");
            continue;
        }
        else if (rest.front() == ']')
            return {JsonValue(std::move(array)), rest.substr(1)};
        else
            throw std::runtime_error("Invalid JSON array format");
    }
}

std::pair<JsonValue, std::string_view> TinyJson::parseObject(const std::string_view sv)
{
    JsonObject object;

    if (sv.empty() || sv.front() != '{')
        throw std::runtime_error("Invalid JSON object format");

    std::string_view rest = sv.substr(1); // Skip the opening brace

    while (true)
    {
        rest = skipWhitespace(rest);
        if (rest.empty())
            throw std::runtime_error("Unterminated JSON object");

        if (rest.front() == '}')
            return {JsonValue(std::move(object)), rest.substr(1)}; // Empty object

        if (rest.front() != '"')
            throw std::runtime_error("Invalid JSON object key format");

        JsonValue key;
        std::tie(key, rest) = parseString(rest);

        rest = skipWhitespace(rest);
        if (rest.empty() || rest.front() != ':')
            throw std::runtime_error("Invalid JSON object format");

        rest.remove_prefix(1); // Skip the colon
        rest = skipWhitespace(rest);

        JsonValue value;
        std::tie(value, rest) = parseJson(rest);
        object.emplace(std::get<std::string>(key), std::move(value));

        rest = skipWhitespace(rest);
        if (rest.empty())
            throw std::runtime_error("Unterminated JSON object");
        else if (rest.front() == ',')
        {
            rest.remove_prefix(1); // Skip the comma
            rest = skipWhitespace(rest);
            if (rest.empty() || rest.front() == '}')
                throw std::runtime_error("Invalid JSON object format");
            continue;
        }
        else if (rest.front() == '}')
            return {JsonValue(std::move(object)), rest.substr(1)};
        else
            throw std::runtime_error("Invalid JSON object format");
    }
}
