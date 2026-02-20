#include "d3d12/d3d12_buffer.hpp"

Swift::D3D12::Buffer::Buffer(Context* context, const BufferCreateInfo& info) : m_context(context)
{
    m_size = info.size;

    m_resource = CreateCommittedResource(static_cast<ID3D12Device14*>(context->GetDevice()), info);
    const auto name = std::wstring{info.name.begin(), info.name.end()};
    m_resource->SetName(name.c_str());

    if (info.data)
    {
        Map();
        memcpy(GetMapped(), info.data, info.size);
        Unmap();
    }
}

Swift::D3D12::Buffer::~Buffer()
{
    m_resource->Release();
}

void Swift::D3D12::Buffer::Write(const void* data, const uint64_t offset, const uint64_t size, const bool one_time)
{
    Map();
    memcpy(static_cast<char*>(GetMapped()) + offset, data, size);

    if (one_time)
    {
        Unmap();
    }
}

void Swift::D3D12::Buffer::Read(const uint32_t offset, const uint32_t size, void* data)
{
    const BufferCreateInfo readback_info{
        .size = size,
        .type = BufferType::eReadback,
    };

    auto* const buffer = m_context->CreateBuffer(readback_info);
    auto* const queue = m_context->CreateQueue({QueueType::eGraphics, QueuePriority::eHigh});
    auto* const command = m_context->CreateCommand(QueueType::eGraphics);

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

    Map(data, size);
    Unmap();
}

void Swift::D3D12::Buffer::Map(void* readback, const uint32_t size)
{
    if (m_mapped) return;
    if (readback)
    {
        const D3D12_RANGE range{.Begin = 0, .End = size};
        m_resource->Map(0, &range, &m_data);
    }
    else
    {
        m_resource->Map(0, nullptr, &m_data);
    }
    m_mapped = true;
}

void Swift::D3D12::Buffer::Unmap()
{
    m_resource->Unmap(0, nullptr);
    m_mapped = false;
}

D3D12_RESOURCE_DESC Swift::D3D12::Buffer::GetResourceDesc(const BufferCreateInfo& info)
{
    return {
        .Dimension = D3D12_RESOURCE_DIMENSION_BUFFER,
        .Alignment = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT,
        .Width = info.size,
        .Height = 1,
        .DepthOrArraySize = 1,
        .MipLevels = 1,
        .Format = DXGI_FORMAT_UNKNOWN,
        .SampleDesc = {1, 0},
        .Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR,
        .Flags = D3D12_RESOURCE_FLAG_NONE,
    };
}

ID3D12Resource* Swift::D3D12::Buffer::CreateCommittedResource(ID3D12Device14* device, const BufferCreateInfo& info)
{
    const auto resource_info = GetResourceDesc(info);
    D3D12_HEAP_PROPERTIES heap_properties = {
        .Type = D3D12_HEAP_TYPE_GPU_UPLOAD,
        .CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN,
        .MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN,
    };

    auto resource_state = D3D12_RESOURCE_STATE_COMMON;
    if (info.type == BufferType::eReadback)
    {
        heap_properties.Type = D3D12_HEAP_TYPE_READBACK;
        resource_state = D3D12_RESOURCE_STATE_COPY_DEST;
    }

    if (info.type == BufferType::eUpload)
    {
        heap_properties.Type = D3D12_HEAP_TYPE_UPLOAD;
        resource_state = D3D12_RESOURCE_STATE_GENERIC_READ;
    }

    ID3D12Resource* resource = nullptr;
    device->CreateCommittedResource(&heap_properties,
                                    D3D12_HEAP_FLAG_NONE,
                                    &resource_info,
                                    resource_state,
                                    nullptr,
                                    IID_PPV_ARGS(&resource));
    return resource;
}
