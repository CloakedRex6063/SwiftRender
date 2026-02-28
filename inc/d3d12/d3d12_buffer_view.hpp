#pragma once
#include "d3d12_descriptor.hpp"
#include "swift_buffer_view.hpp"

namespace Swift::D3D12
{
    class Context;
    class BufferView : public IBufferView
    {
    public:
        SWIFT_NO_COPY(BufferView);
        SWIFT_NO_MOVE(BufferView);
        BufferView(Context* m_context, IBuffer* buffer, const BufferViewCreateInfo& create_info);
        ~BufferView() override;
        [[nodiscard]] uint32_t GetDescriptorIndex() override { return m_data.index; }

    private:
        Context* m_context;
        DescriptorData m_data;
    };
}
