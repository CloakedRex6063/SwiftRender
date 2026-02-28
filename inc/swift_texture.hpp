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

        [[nodiscard]] TextureCreateInfo GetCreateInfo() const { return m_create_info; }
        [[nodiscard]] UInt2 GetSize() const { return m_size; }
        [[nodiscard]] uint32_t GetMipLevels() const { return m_mip_levels; }
        [[nodiscard]] uint32_t GetArraySize() const { return m_array_size; }
        [[nodiscard]] Format GetFormat() const { return m_format; }
        [[nodiscard]] virtual void* GetResource() = 0;
        [[nodiscard]] virtual uint64_t GetVirtualAddress() = 0;
        [[nodiscard]] ResourceState GetState() const { return m_state; }
        void SetState(const ResourceState state) { m_state = state; }

    protected:
        explicit ITexture(const TextureCreateInfo& create_info) : m_create_info(create_info), m_format(create_info.format) {}
        ResourceState m_state = ResourceState::eCommon;
        TextureCreateInfo m_create_info;
        void* m_data = nullptr;
        bool m_mapped = false;
        Format m_format;
        UInt2 m_size;
        uint32_t m_mip_levels = 1;
        uint32_t m_array_size = 1;
    };
}  // namespace Swift
