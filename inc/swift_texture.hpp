#pragma once
#include "swift_macros.hpp"

namespace Swift
{
    class ITexture
    {
    public:
        SWIFT_DESTRUCT(ITexture);
        SWIFT_NO_MOVE(ITexture);
        SWIFT_NO_COPY(ITexture);

        [[nodiscard]] std::array<uint32_t, 2> GetSize() const { return m_size; }
        [[nodiscard]] uint32_t GetMipLevels() const { return m_mip_levels; }
        [[nodiscard]] uint32_t GetArraySize() const { return m_array_size; }
        [[nodiscard]] Format GetFormat() const { return m_format; }
        [[nodiscard]] virtual void* GetResource() = 0;
        [[nodiscard]] virtual uint64_t GetVirtualAddress() = 0;
        [[nodiscard]] ResourceState GetState() const { return m_state; }
        void SetState(const ResourceState state) { m_state = state; }

    protected:
        SWIFT_CONSTRUCT(ITexture);
        ResourceState m_state = ResourceState::eCommon;
        void* m_data = nullptr;
        bool m_mapped = false;
        Format m_format;
        std::array<uint32_t, 2> m_size;
        uint32_t m_mip_levels = 1;
        uint32_t m_array_size = 1;
    };
}  // namespace Swift
