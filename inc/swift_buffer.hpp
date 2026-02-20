#pragma once
#include "swift_macros.hpp"
#include "swift_structs.hpp"

namespace Swift
{
    class IBuffer
    {
    public:
        SWIFT_DESTRUCT(IBuffer);
        SWIFT_NO_MOVE(IBuffer);
        SWIFT_NO_COPY(IBuffer);

        virtual void Write(const void* data, uint64_t offset, uint64_t size, bool one_time = false) = 0;
        virtual void Read(uint32_t offset, uint32_t size, void* data) = 0;
        [[nodiscard]] uint64_t GetSize() const { return m_size; }
        virtual void Map(void* readback = nullptr, uint32_t size = 0) = 0;
        virtual void Unmap() = 0;
        [[nodiscard]] virtual void* GetResource() = 0;
        [[nodiscard]] void* GetMapped() const { return m_data; }
        [[nodiscard]] virtual uint64_t GetVirtualAddress() = 0;
        [[nodiscard]] ResourceState GetState() const { return m_state; }
        void SetState(const ResourceState state) { m_state = state; }

    protected:
        SWIFT_CONSTRUCT(IBuffer);
        ResourceState m_state = ResourceState::eCommon;
        void* m_data = nullptr;
        uint64_t m_size = 0;
        bool m_mapped = false;
    };
}  // namespace Swift
