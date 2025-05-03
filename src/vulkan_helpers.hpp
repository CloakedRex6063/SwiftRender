#pragma once
#include "expected"
#include "vulkan/vulkan_core.h"

namespace Swift::Vulkan
{
    template<typename T, typename Error>
    std::expected<T, Error> HandleResult(const VkResult result, Error error, T& value)
    {
        std::expected<T, Error> err;
        if (result != VK_SUCCESS)
        {
            return std::unexpected(error);
        }
        return value;
    }

    template<typename Error>
    std::expected<void, Error> HandleResult(const VkResult result, Error error)
    {
        std::expected<void, Error> err;
        if (result != VK_SUCCESS)
        {
            return std::unexpected(error);
        }
        return {};
    }
}  // namespace Swift::Vulkan