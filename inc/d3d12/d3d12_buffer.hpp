#pragma once
#include "d3d12_context.hpp"
#include "swift_buffer.hpp"

namespace Swift::D3D12
{
    class Buffer final : public IBuffer
    {
    public:
        Buffer(Context* context, DescriptorHeap* cbv_srv_uav_heap, const BufferCreateInfo& info);
        SWIFT_NO_COPY(Buffer);
        SWIFT_NO_MOVE(Buffer);
        SWIFT_DESTRUCT(Buffer);
        void Write(const void* data, uint64_t offset, uint64_t size, bool one_time = false) override;
        void Read(uint32_t offset, uint32_t size, void* data) override;

    private:
        DescriptorHeap* m_cbv_srv_uav_heap;
        Context* m_context;
    };
}  // namespace Swift::D3D12
