#pragma once
#include "type_traits"

// https://github.com/KhronosGroup/Vulkan-Hpp/blob/main/VulkanHppGenerator.hpp
template <typename BitType>
class EnumFlags
{
public:
    using MaskType = std::underlying_type_t<BitType>;

    constexpr EnumFlags() noexcept : m_mask(0) {}

    constexpr EnumFlags(BitType bit) noexcept : m_mask(static_cast<MaskType>(bit)) {}

    constexpr explicit EnumFlags(MaskType flags) noexcept : m_mask(flags) {}

    constexpr EnumFlags& operator|=(EnumFlags const& rhs) noexcept
    {
        m_mask |= rhs.m_mask;
        return *this;
    }

    constexpr bool operator!() const noexcept { return !m_mask; }

    constexpr bool operator&(BitType const& rhs) const noexcept { return m_mask & static_cast<MaskType>(rhs); }

    constexpr EnumFlags operator&(EnumFlags const& rhs) const noexcept { return EnumFlags(m_mask & rhs.m_mask); }

    constexpr EnumFlags operator|(EnumFlags const& rhs) const noexcept { return EnumFlags(m_mask | rhs.m_mask); }

    constexpr MaskType operator*() { return m_mask; }

private:
    MaskType m_mask;
};
