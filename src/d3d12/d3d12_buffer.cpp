#include "d3d12/d3d12_buffer.hpp"

Swift::D3D12::Buffer::Buffer(const std::shared_ptr<Context>& context,
                             const std::shared_ptr<DescriptorHeap>& cbv_srv_uav_heap,
                             const BufferCreateInfo& info)
    : m_cbv_srv_uav_heap(cbv_srv_uav_heap), m_descriptor(cbv_srv_uav_heap->Allocate()), m_context(context)
{
    m_resource = info.resource;
    m_element_size = info.element_size;
    m_num_elements = info.num_elements;
    if (!info.resource)
    {
        m_resource = context->CreateResource(info);
    }
    auto* device = static_cast<ID3D12Device14*>(context->GetDevice());
    if (info.type == BufferType::eConstantBuffer)
    {
        const D3D12_CONSTANT_BUFFER_VIEW_DESC desc{
            .BufferLocation = m_resource->GetVirtualAddress() + info.first_element * info.element_size,
            .SizeInBytes = info.num_elements * info.element_size,
        };
        device->CreateConstantBufferView(&desc, m_descriptor.cpu_handle);
    }
    if (info.type == BufferType::eStructuredBuffer)
    {
        D3D12_SHADER_RESOURCE_VIEW_DESC desc{.Format = DXGI_FORMAT_UNKNOWN,
                                             .ViewDimension = D3D12_SRV_DIMENSION_BUFFER,
                                             .Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING,
                                             .Buffer = {
                                                 .FirstElement = info.first_element,
                                                 .NumElements = info.num_elements,
                                                 .StructureByteStride = info.element_size,
                                             }};
        const auto resource = static_cast<ID3D12Resource*>(m_resource->GetResource());
        device->CreateShaderResourceView(resource, &desc, m_descriptor.cpu_handle);
    }
    if (info.data)
    {
        m_resource->Map();
        memcpy(m_resource->GetMapped(), info.data, info.num_elements * info.element_size);
        m_resource->Unmap();
    }
}

Swift::D3D12::Buffer::~Buffer() { m_cbv_srv_uav_heap->Free(m_descriptor); }

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
        .num_elements = static_cast<uint32_t>(static_cast<float>(size) / static_cast<float>(m_element_size)),
        .element_size = m_element_size,
        .type = BufferType::eReadback,
    };

    const auto buffer = m_context->CreateBuffer(readback_info);
    const auto queue = m_context->CreateQueue({QueueType::eGraphics, QueuePriority::eHigh});
    auto command = m_context->CreateCommand(QueueType::eGraphics);

    command->Begin();

    const auto copy_region = BufferCopyRegion{
        .src_buffer = shared_from_this(),
        .dst_buffer = buffer,
        .src_offset = offset,
        .dst_offset = 0,
        .size = size,
    };

    command->CopyBufferRegion(copy_region);

    command->End();

    const auto value = queue->Execute(std::array{command});
    queue->Wait(value);

    auto resource = buffer->GetResource();

    m_resource->Map(data, size);
    m_resource->Unmap();
}
