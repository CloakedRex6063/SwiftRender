#pragma once
#include "swift_macros.hpp"
#include "swift_structs.hpp"

namespace Swift
{
    class IResource
    {
    public:
        SWIFT_DESTRUCT(IResource);
        SWIFT_NO_MOVE(IResource);
        SWIFT_NO_COPY(IResource);

        virtual void Map(void* readback = nullptr, uint32_t size = 0) = 0;
        virtual void Unmap() = 0;
        [[nodiscard]] virtual void* GetResource() = 0;
        [[nodiscard]] void* GetMapped() const { return m_data; }
        [[nodiscard]] virtual uint64_t GetVirtualAddress() = 0;
        [[nodiscard]] ResourceState GetState() const { return m_state; }
        void SetState(const ResourceState state) { m_state = state; }

    protected:
        SWIFT_CONSTRUCT(IResource);
        ResourceState m_state = ResourceState::eCommon;
        void* m_data = nullptr;
        bool m_mapped = false;
    };
}  // namespace Swift
