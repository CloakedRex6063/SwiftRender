#include "d3d12/d3d12_texture.hpp"

#include "d3d12_helpers.hpp"

Swift::D3D12::Texture::Texture(ID3D12Resource* resource, const TextureCreateInfo& info) : ITexture(info), m_resource(resource)
{
    m_format = info.format;
    m_size = {info.width, info.height};
    m_array_size = info.array_size;
    m_mip_levels = info.mip_levels;
}

Swift::D3D12::Texture::Texture(Context* context, const TextureCreateInfo& info) : ITexture(info), m_context(context)
{
    m_format = info.format;
    m_size = {info.width, info.height};
    m_array_size = info.array_size;
    m_mip_levels = info.mip_levels;

    CreateCommittedResource(info);
    const auto name = std::wstring{info.name.begin(), info.name.end()};
    m_resource->SetName(name.c_str());
}

Swift::D3D12::Texture::~Texture()
{
    if (m_allocation)
    {
        m_allocation->Release();
    }
    m_resource->Release();
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
        .Alignment = 0,
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

void Swift::D3D12::Texture::CreateCommittedResource(const TextureCreateInfo& info)
{
    const auto resource_info = GetResourceDesc(info);

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

    D3D12MA::ALLOCATION_DESC alloc_desc = {
        .HeapType = D3D12_HEAP_TYPE_DEFAULT,
    };
    auto* allocator = m_context->GetAllocator();
    allocator->CreateResource(&alloc_desc,
                              &resource_info,
                              D3D12_RESOURCE_STATE_COPY_DEST,
                              p_clear_value,
                              &m_allocation,
                              IID_PPV_ARGS(&m_resource));
}