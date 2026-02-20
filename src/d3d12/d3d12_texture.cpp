#include "d3d12/d3d12_texture.hpp"

#include "d3d12_helpers.hpp"

Swift::D3D12::Texture::Texture(ID3D12Resource* resource, const TextureCreateInfo& info)
    : m_resource(resource)
{
    m_format = info.format;
    m_size = {info.width, info.height};
    m_array_size = info.array_size;
    m_mip_levels = info.mip_levels;
}

Swift::D3D12::Texture::Texture(const Context* context, const TextureCreateInfo& info)
{
    m_format = info.format;
    m_size = {info.width, info.height};
    m_array_size = info.array_size;
    m_mip_levels = info.mip_levels;

    m_resource = CreateCommittedResource(static_cast<ID3D12Device14*>(context->GetDevice()), info);
    const auto name = std::wstring{info.name.begin(), info.name.end()};
    m_resource->SetName(name.c_str());
}

D3D12_RESOURCE_DESC Swift::D3D12::Texture::GetResourceDesc(const TextureCreateInfo& info)
{
    auto flags = D3D12_RESOURCE_FLAG_NONE;
    if (info.flags & TextureFlags::eRenderTarget)
    {
        flags |= D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
    }
    else if (info.flags & TextureFlags::eDepthStencil)
    {
        flags |= D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
    }
    if (info.flags & TextureFlags::eUnorderedAccess)
    {
        flags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
    }
    const auto sample_desc = info.msaa ? DXGI_SAMPLE_DESC{info.msaa->samples, info.msaa->quality} : DXGI_SAMPLE_DESC{1, 0};
    return {
        .Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D,
        .Alignment = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT,
        .Width = info.width,
        .Height = info.height,
        .DepthOrArraySize = info.array_size,
        .MipLevels = info.mip_levels,
        .Format = ToResourceDXGIFormat(info.format),
        .SampleDesc = sample_desc,
        .Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN,
        .Flags = flags,
    };
}

ID3D12Resource* Swift::D3D12::Texture::CreateCommittedResource(ID3D12Device14* device, const TextureCreateInfo& info)
{
    const auto resource_info = GetResourceDesc(info);

    constexpr D3D12_HEAP_PROPERTIES heap_properties = {
        .Type = D3D12_HEAP_TYPE_DEFAULT,
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

    ID3D12Resource* resource = nullptr;
    device->CreateCommittedResource(&heap_properties,
                                    D3D12_HEAP_FLAG_NONE,
                                    &resource_info,
                                    D3D12_RESOURCE_STATE_COMMON,
                                    p_clear_value,
                                    IID_PPV_ARGS(&resource));
    return resource;
}