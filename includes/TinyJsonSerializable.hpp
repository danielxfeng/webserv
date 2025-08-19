#pragma once

#include <vector>
#include <string>

class TinyJsonSerializable {
public:
    TinyJsonSerializable() = default;
    TinyJsonSerializable(const TinyJsonSerializable&) = default;
    TinyJsonSerializable& operator=(const TinyJsonSerializable&) = default;
    TinyJsonSerializable& operator=(TinyJsonSerializable&&) = default;
    virtual ~TinyJsonSerializable() = default;
    virtual void fromJson(const std::string& jsonString) = 0;
};
