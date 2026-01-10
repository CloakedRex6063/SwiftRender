#pragma once
#include "swift_macros.hpp"

namespace Swift
{
    class IBuffer : public std::enable_shared_from_this<IBuffer>
    {
    public:
        SWIFT_DESTRUCT(IBuffer);
        SWIFT_NO_MOVE(IBuffer);
        SWIFT_NO_COPY(IBuffer);

        virtual void Write(const void* data, uint64_t offset, uint64_t size, bool one_time = false) = 0;
        virtual void Read(uint32_t offset, uint32_t size, void *data) = 0;
        [[nodiscard]] uint64_t GetNumElements() const { return m_num_elements; }
        [[nodiscard]] uint64_t GetElementSize() const { return m_element_size; }
        [[nodiscard]] uint64_t GetSize() const { return m_element_size * m_num_elements; }
        [[nodiscard]] virtual uint32_t GetDescriptorIndex() const = 0;
        [[nodiscard]] std::shared_ptr<IResource> GetResource() { return m_resource; }

    protected:
        SWIFT_CONSTRUCT(IBuffer);
        std::shared_ptr<IResource> m_resource;
        uint64_t m_num_elements = 0;
        uint32_t m_element_size = 0;
    };
}
