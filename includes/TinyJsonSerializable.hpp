#pragma once

#include <vector>
#include <string>

/**
 * @brief An interface for classes that can be serialized from JSON strings.
 */
class TinyJsonSerializable
{
public:
    TinyJsonSerializable() = default;
    TinyJsonSerializable(const TinyJsonSerializable &) = default;
    TinyJsonSerializable &operator=(const TinyJsonSerializable &) = default;
    TinyJsonSerializable &operator=(TinyJsonSerializable &&) = default;
    virtual ~TinyJsonSerializable() = default;
    virtual void fromJson(const std::string &jsonString) = 0;
};
