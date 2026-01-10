#pragma once
#include "swift_structs.hpp"

namespace Swift
{
    inline uint32_t GetBytesPerPixel(const Format format)
    {
        switch (format)
        {
            case Format::eRGBA8_UNORM:
                return 4;
            case Format::eRGBA16F:
                return 8;
            case Format::eRGBA32F:
                return 16;
            default:
                return 4;
        }
    }

    inline uint32_t GetTextureSize(const TextureCreateInfo& texture)
    {
        uint32_t total_size = 0;
        uint32_t mip_width  = texture.width;
        uint32_t mip_height = texture.height;
        const uint32_t bpp       = GetBytesPerPixel(texture.format);

        for (uint16_t i = 0; i < texture.mip_levels; ++i)
        {
            total_size += mip_width * mip_height * bpp;

            mip_width  = std::max(1u, mip_width  / 2);
            mip_height = std::max(1u, mip_height / 2);
        }

        total_size *= texture.array_size;
        return total_size;
    }

    inline std::array<uint32_t, 3> CalculateDispatchGroups(const uint32_t total_groups)
    {
        constexpr uint32_t MAX_DISPATCH = 65535;
    
        std::array<uint32_t, 3> result = {1, 1, 1};
    
        if (total_groups <= MAX_DISPATCH) {
            result[0] = total_groups;
            return result;
        }
    
        result[0] = MAX_DISPATCH;
        uint32_t remaining = (total_groups + MAX_DISPATCH - 1) / MAX_DISPATCH; 
    
        if (remaining <= MAX_DISPATCH) {
            result[1] = remaining;
            return result;
        }

        result[1] = MAX_DISPATCH;
        result[2] = (remaining + MAX_DISPATCH - 1) / MAX_DISPATCH;
    
        return result;
    }
}
