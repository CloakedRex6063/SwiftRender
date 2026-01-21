#include "d3d12/d3d12_buffer.hpp"

Swift::D3D12::Buffer::Buffer(Context* context,
                             DescriptorHeap* cbv_srv_uav_heap,
                             const BufferCreateInfo& info)
    : m_cbv_srv_uav_heap(cbv_srv_uav_heap), m_context(context)
{
    m_resource = info.resource;
    m_size = info.size;
    if (!info.resource)
    {
        m_resource = context->CreateResource(info);
    }
    if (info.data)
    {
        m_resource->Map();
        memcpy(m_resource->GetMapped(), info.data, info.size);
        m_resource->Unmap();
    }
}

void Swift::D3D12::Buffer::Write(const void* data, const uint64_t offset, const uint64_t size, const bool one_time)
{
    m_resource->Map();
    memcpy(static_cast<char*>(m_resource->GetMapped()) + offset, data, size);

    if (one_time)
    {
        m_resource->Unmap();
    }
}

void Swift::D3D12::Buffer::Read(const uint32_t offset, const uint32_t size, void* data)
{
    const BufferCreateInfo readback_info{
        .size = size,
        .type = BufferType::eReadback,
    };

    auto *const buffer = m_context->CreateBuffer(readback_info);
    auto *const queue = m_context->CreateQueue({QueueType::eGraphics, QueuePriority::eHigh});
    auto *const command = m_context->CreateCommand(QueueType::eGraphics);

    command->Begin();

    const auto copy_region = BufferCopyRegion{
        .src_buffer = this,
        .dst_buffer = buffer,
        .src_offset = offset,
        .dst_offset = 0,
        .size = size,
    };

    command->CopyBufferRegion(copy_region);

    command->End();

    const auto value = queue->Execute(command);
    queue->Wait(value);

    m_resource->Map(data, size);
    m_resource->Unmap();
}
