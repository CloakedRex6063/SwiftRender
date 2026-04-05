#include "d3d12/d3d12_buffer.hpp"

Swift::D3D12::Buffer::Buffer(Context* context, const BufferCreateInfo& info) : m_context(context)
{
    m_size = info.size;

    CreateCommittedResource(info);
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
    if (m_allocation)
    {
        m_allocation->Release();
    }
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
        .Alignment = 0,
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

void Swift::D3D12::Buffer::CreateCommittedResource(const BufferCreateInfo& info)
{
    auto resource_info = GetResourceDesc(info);

    if (info.flags & BufferFlags::eUnorderedAccess)
    {
        resource_info.Flags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
    }

    constexpr D3D12MA::ALLOCATION_DESC alloc_desc = {
        .HeapType = D3D12_HEAP_TYPE_GPU_UPLOAD,
    };
    auto* allocator = m_context->GetAllocator();
    allocator->CreateResource(&alloc_desc,
                              &resource_info,
                              D3D12_RESOURCE_STATE_COMMON,
                              nullptr,
                              &m_allocation,
                              IID_PPV_ARGS(&m_resource));
}
