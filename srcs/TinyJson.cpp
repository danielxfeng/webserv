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

    if (sv.front() == '{')
        std::tie(value, rest) = parseObject(sv);
    else if (sv.front() == '[')
        std::tie(value, rest) = parseArray(sv);
    else if (sv.front() == '"')
        std::tie(value, rest) = parseString(sv);
    else if (isdigit(sv.front()) || sv.front() == '-')
        std::tie(value, rest) = parseNumber(sv);
    else if (sv.front() == 't' || sv.front() == 'f')
        std::tie(value, rest) = parseBool(sv);
    else if (sv.front() == 'n')
        std::tie(value, rest) = parseNull(sv);
    else
        throw std::runtime_error("Invalid JSON format");

    rest = skipWhitespace(rest);
    if (!rest.empty())
        throw std::runtime_error("Invalid JSON format");

    return value;
}

std::pair<JsonValue, std::string_view> TinyJson::parseBool(const std::string_view sv)
{
    if (sv.starts_with("true"))
        return {JsonValue(true), sv.substr(4)};
    if (sv.starts_with("false"))
        return {JsonValue(false), sv.substr(5)};
    throw std::runtime_error("Invalid JSON boolean value");
}

std::pair<JsonValue, std::string_view> TinyJson::parseNull(const std::string_view sv)
{
    if (sv.starts_with("null"))
        return {JsonValue(nullptr), sv.substr(4)};
    throw std::runtime_error("Invalid JSON null value");
}

// TODO: prevent leading zeros
std::pair<JsonValue, std::string_view> TinyJson::parseNumber(const std::string_view sv)
{
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
