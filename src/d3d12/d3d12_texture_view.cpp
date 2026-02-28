#include "d3d12/d3d12_texture_view.hpp"

#include "d3d12_helpers.hpp"
#include "d3d12/d3d12_context.hpp"

Swift::D3D12::TextureView::TextureView(Context* context, ITexture* texture, const TextureViewCreateInfo& texture_view_create_info)
    : ITextureView(texture, texture_view_create_info.type), m_context(context)
{
    switch (texture_view_create_info.type)
    {
        case TextureViewType::eRenderTarget:
            CreateRenderTarget(context, texture, texture_view_create_info);
            break;
        case TextureViewType::eDepthStencil:
            CreateDepthStencil(context, texture, texture_view_create_info);
            break;
        case TextureViewType::eShaderResource:
            CreateShaderResource(context, texture, texture_view_create_info);
            break;
        case TextureViewType::eUnorderedAccess:
            CreateUnorderedAccess(context, texture, texture_view_create_info);
            break;
    }
}

Swift::D3D12::TextureView::~TextureView()
{
    switch (m_type)
    {
        case TextureViewType::eRenderTarget:
            m_context->GetRTVHeap()->Free(m_descriptor_data);
            break;
        case TextureViewType::eDepthStencil:
            m_context->GetDSVHeap()->Free(m_descriptor_data);
            break;
        case TextureViewType::eShaderResource:
            m_context->GetCBVSRVUAVHeap()->Free(m_descriptor_data);
            break;
        case TextureViewType::eUnorderedAccess:
            m_context->GetCBVSRVUAVHeap()->Free(m_descriptor_data);
            break;
    }
}

void Swift::D3D12::TextureView::CreateRenderTarget(const Context* context,
                                                   ITexture* texture,
                                                   const TextureViewCreateInfo& texture_view_create_info)
{
    const auto& rtv_heap = context->GetRTVHeap();
    m_descriptor_data = rtv_heap->Allocate();
    const D3D12_RENDER_TARGET_VIEW_DESC rtv_desc = {.Format = ToViewDXGIFormat(texture->GetFormat()),
                                                    .ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D,
                                                    .Texture2D = {
                                                        .MipSlice = texture_view_create_info.base_mip_level,
                                                    }};
    auto* device = static_cast<ID3D12Device*>(context->GetDevice());
    auto* const resource = static_cast<ID3D12Resource*>(texture->GetResource());
    device->CreateRenderTargetView(resource, &rtv_desc, m_descriptor_data.cpu_handle);
}
void Swift::D3D12::TextureView::CreateDepthStencil(const Context* context,
                                                   ITexture* texture,
                                                   const TextureViewCreateInfo& texture_view_create_info)
{
    auto* device = static_cast<ID3D12Device*>(context->GetDevice());
    auto* const resource = static_cast<ID3D12Resource*>(texture->GetResource());

    auto* dsv_heap = context->GetDSVHeap();
    m_descriptor_data = dsv_heap->Allocate();
    const D3D12_DEPTH_STENCIL_VIEW_DESC dsv_desc = {.Format = ToDXGIFormat(texture->GetFormat()),
                                                    .ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D,
                                                    .Texture2D = {
                                                        .MipSlice = texture_view_create_info.base_mip_level,
                                                    }};
    device->CreateDepthStencilView(resource, &dsv_desc, m_descriptor_data.cpu_handle);
}

void Swift::D3D12::TextureView::CreateShaderResource(const Context* context,
                                                     ITexture* texture,
                                                     const TextureViewCreateInfo& texture_view_create_info)
{
    uint32_t mip_count = texture_view_create_info.mip_count;
    if (texture_view_create_info.mip_count == 0)
    {
        mip_count = texture->GetMipLevels();
    }
    auto* device = static_cast<ID3D12Device*>(context->GetDevice());
    auto* const resource = static_cast<ID3D12Resource*>(texture->GetResource());
    auto* srv_heap = context->GetCBVSRVUAVHeap();
    m_descriptor_data = srv_heap->Allocate();
    const bool is_cubemap = texture->GetArraySize() > 1;

    D3D12_SHADER_RESOURCE_VIEW_DESC srv_desc = {
        .Format = ToViewDXGIFormat(texture->GetFormat()),
        .ViewDimension = is_cubemap ? D3D12_SRV_DIMENSION_TEXTURECUBE : D3D12_SRV_DIMENSION_TEXTURE2D,
        .Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING,
    };

    if (is_cubemap)
    {
        srv_desc.TextureCube = {
            .MostDetailedMip = texture_view_create_info.base_mip_level,
            .MipLevels = mip_count,
        };
    }
    else
    {
        srv_desc.Texture2D = {
            .MostDetailedMip = texture_view_create_info.base_mip_level,
            .MipLevels = mip_count,
        };
    }
    device->CreateShaderResourceView(resource, &srv_desc, m_descriptor_data.cpu_handle);
}

void Swift::D3D12::TextureView::CreateUnorderedAccess(const Context* context,
                                                      ITexture* texture,
                                                      const TextureViewCreateInfo& texture_view_create_info)
{
    auto* cbv_heap = context->GetCBVSRVUAVHeap();
    m_descriptor_data = cbv_heap->Allocate();
    auto* const resource = static_cast<ID3D12Resource*>(texture->GetResource());
    const D3D12_UNORDERED_ACCESS_VIEW_DESC uav_desc = {
        .Format = ToViewDXGIFormat(texture->GetFormat()),
        .ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D,
        .Texture2D =
            {
                .MipSlice = texture_view_create_info.base_mip_level,
            },
    };
    auto* device = static_cast<ID3D12Device*>(context->GetDevice());
    device->CreateUnorderedAccessView(resource, nullptr, &uav_desc, m_descriptor_data.cpu_handle);
}