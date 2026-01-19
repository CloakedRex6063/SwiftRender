#include "d3d12/d3d12_resource.hpp"

#include "d3d12_helpers.hpp"

Swift::D3D12::Resource::Resource(ID3D12Resource* resource) : m_resource(resource) { m_mapped = false; }

Swift::D3D12::Resource::Resource(ID3D12Device14* device, const TextureCreateInfo& info)
{
    auto flags = D3D12_RESOURCE_FLAG_NONE;
    if (info.flags & TextureFlags::eRenderTarget)
    {
        flags |= D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
    }
    if (info.flags & TextureFlags::eDepthStencil)
    {
        flags |= D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
    }
    const auto sample_desc = info.msaa ? DXGI_SAMPLE_DESC{info.msaa->samples, info.msaa->quality} : DXGI_SAMPLE_DESC{1, 0};
    const D3D12_RESOURCE_DESC resource_info = {
        .Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D,
        .Alignment = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT,
        .Width = info.width,
        .Height = info.height,
        .DepthOrArraySize = info.array_size,
        .MipLevels = info.mip_levels,
        .Format = ToDXGIFormat(info.format),
        .SampleDesc = sample_desc,
        .Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN,
        .Flags = flags,
    };

    constexpr D3D12_HEAP_PROPERTIES heap_properties = {
        .Type = D3D12_HEAP_TYPE_GPU_UPLOAD,
        .CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN,
        .MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN,
    };
    D3D12_CLEAR_VALUE clear_value = {
        .Format = ToDXGIFormat(info.format),
    };
    if (info.flags & TextureFlags::eRenderTarget)
    {
        clear_value.Color[0] = 0.0f;
        clear_value.Color[1] = 0.0f;
        clear_value.Color[2] = 0.0f;
        clear_value.Color[3] = 0.0f;
    }
    if (info.flags & TextureFlags::eDepthStencil)
    {
        clear_value.DepthStencil.Depth = 1.0f;
        clear_value.DepthStencil.Stencil = 0;
    }

    const D3D12_CLEAR_VALUE* p_clear_value = &clear_value;
    if (!(info.flags & TextureFlags::eDepthStencil) && !(info.flags & TextureFlags::eRenderTarget))
    {
        p_clear_value = nullptr;
    }

    [[maybe_unused]]
    const auto result = device->CreateCommittedResource(&heap_properties,
                                                        D3D12_HEAP_FLAG_NONE,
                                                        &resource_info,
                                                        D3D12_RESOURCE_STATE_COMMON,
                                                        p_clear_value,
                                                        IID_PPV_ARGS(&m_resource));
}

Swift::D3D12::Resource::Resource(ID3D12Device14* device, const BufferCreateInfo& info)
{
    const D3D12_RESOURCE_DESC resource_info = {
        .Dimension = D3D12_RESOURCE_DIMENSION_BUFFER,
        .Alignment = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT,
        .Width = info.num_elements * info.element_size,
        .Height = 1,
        .DepthOrArraySize = 1,
        .MipLevels = 1,
        .Format = DXGI_FORMAT_UNKNOWN,
        .SampleDesc = {1, 0},
        .Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR,
        .Flags = D3D12_RESOURCE_FLAG_NONE,
    };

    D3D12_HEAP_PROPERTIES heap_properties = {
        .Type = D3D12_HEAP_TYPE_GPU_UPLOAD,
        .CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN,
        .MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN,
    };

    if (info.type == BufferType::eReadback)
    {
        heap_properties.Type = D3D12_HEAP_TYPE_READBACK;
    }

    [[maybe_unused]]
    const auto result = device->CreateCommittedResource(&heap_properties,
                                                        D3D12_HEAP_FLAG_NONE,
                                                        &resource_info,
                                                        D3D12_RESOURCE_STATE_COMMON,
                                                        nullptr,
                                                        IID_PPV_ARGS(&m_resource));
}

Swift::D3D12::Resource::~Resource()
{
    if (m_mapped)
    {
        m_resource->Unmap(0, nullptr);
    }
    m_resource->Release();
}

uint64_t Swift::D3D12::Resource::GetVirtualAddress() { return m_resource->GetGPUVirtualAddress(); }

void Swift::D3D12::Resource::Map(void* readback, const uint32_t size)
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

void Swift::D3D12::Resource::Unmap()
{
    m_resource->Unmap(0, nullptr);
    m_mapped = false;
}
