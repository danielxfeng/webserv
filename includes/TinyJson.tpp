#pragma once

#include "TinyJson.hpp"

template <typename T>
T TinyJson::as(const JsonValue &jsonValue)
{
    if constexpr (std::is_arithmetic_v<T>)
    {
        const double *ptr = std::get_if<double>(&jsonValue);
        if (ptr)
        {
            if constexpr (std::is_integral_v<T>)
            {
                if (std::trunc(*ptr) != *ptr)
                    throw std::bad_variant_access();
                if (*ptr < static_cast<double>(std::numeric_limits<T>::lowest()) ||
                    *ptr > static_cast<double>(std::numeric_limits<T>::max()))
                    throw std::out_of_range("Number out of range for integral type");
            }
            if constexpr (std::is_floating_point_v<T>)
            {
                if (*ptr < static_cast<double>(std::numeric_limits<T>::lowest()) ||
                    *ptr > static_cast<double>(std::numeric_limits<T>::max()))
                    throw std::out_of_range("Number out of range for floating-point type");
            }
            return static_cast<T>(*ptr);
        }
        throw std::bad_variant_access();
    }

    const T *ptr = std::get_if<T>(&jsonValue);
    if (ptr)
        return *ptr;
    throw std::bad_variant_access();
}

template <typename T>
T TinyJson::as(const JsonValue &jsonValue, T defaultValue)
{
    try
    {
        return as<T>(jsonValue);
    }
    catch (const std::bad_variant_access &)
    {
        return defaultValue;
    }
    catch (const std::out_of_range &)
    {
        return defaultValue;
    }
}
