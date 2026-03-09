#pragma once
#include "swift_structs.hpp"
#include "swift_texture.hpp"

namespace Swift
{
    class IBufferView
    {
    public:
        SWIFT_DESTRUCT(IBufferView);
        SWIFT_NO_COPY(IBufferView);
        SWIFT_NO_MOVE(IBufferView);
        [[nodiscard]] virtual uint32_t GetDescriptorIndex() = 0;
        [[nodiscard]] IBuffer* GetBuffer() const { return m_buffer; }

    protected:
        IBufferView(IBuffer* buffer, const BufferViewType view_type) : m_buffer(buffer), m_type(view_type) {}
        IBuffer* m_buffer;
        BufferViewType m_type;
    };
}