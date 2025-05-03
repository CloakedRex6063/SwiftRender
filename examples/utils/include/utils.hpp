#pragma once
#include "iostream"
#include "expected"
#include "string_view"

namespace Swift
{
    template<typename T, typename Error>
    T ThrowOnFail(const std::expected<T, Error>& value, std::string_view error_msg)
    {
        if (!value)
        {
            std::cerr << error_msg << std::endl;
            exit(-1);
        }
        return std::move(value.value());
    }

    template<typename Error>
    void ThrowOnFail(const std::expected<void, Error>& value, std::string_view error_msg)
    {
        if (!value)
        {
            std::cerr << error_msg << std::endl;
            exit(-1);
        }
    }
}  // namespace Swift