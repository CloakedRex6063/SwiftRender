#pragma once
#include "swift_macros.hpp"
#include "swift_resource.hpp"

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
        [[nodiscard]] std::shared_ptr<IResource> GetResource() const { return m_resource; }

    protected:
        SWIFT_CONSTRUCT(IBuffer);
        std::shared_ptr<IResource> m_resource;
        uint64_t m_size = 0;
    };
}  // namespace Swift
