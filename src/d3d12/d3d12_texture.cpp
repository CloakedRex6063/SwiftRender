#include "d3d12/d3d12_texture.hpp"

#include "d3d12_helpers.hpp"

Swift::D3D12::Texture::Texture(Context* context,
                               const std::shared_ptr<DescriptorHeap>& rtv_heap,
                               const std::shared_ptr<DescriptorHeap>& dsv_heap,
                               const std::shared_ptr<DescriptorHeap>& srv_heap,
                               const TextureCreateInfo& info)
    : m_rtv_heap(rtv_heap), m_dsv_heap(dsv_heap), m_srv_heap(srv_heap)
{
    m_resource = info.resource;
    m_format = info.format;
    m_size = {info.width, info.height};
    m_array_size = info.array_size;
    m_mip_levels = info.mip_levels;

    if (!m_resource)
    {
        m_resource = context->CreateResource(info);
    }
    auto* dx_resource = static_cast<ID3D12Resource*>(m_resource->GetResource());
    auto* device = static_cast<ID3D12Device14*>(context->GetDevice());
    if (info.flags & TextureFlags::eRenderTarget)
    {
        const D3D12_RENDER_TARGET_VIEW_DESC rtv_desc = {
            .Format = ToDXGIFormat(info.format),
            .ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D,
        };
        m_rtv_descriptor = rtv_heap->Allocate();
        device->CreateRenderTargetView(dx_resource, &rtv_desc, m_rtv_descriptor->cpu_handle);
    }
    if (info.flags & TextureFlags::eDepthStencil)
    {
        const D3D12_DEPTH_STENCIL_VIEW_DESC dsv_desc = {
            .Format = ToDXGIFormat(info.format),
            .ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D,
        };
        m_dsv_descriptor = dsv_heap->Allocate();
        device->CreateDepthStencilView(dx_resource, &dsv_desc, m_dsv_descriptor->cpu_handle);
    }

    if (info.flags & TextureFlags::eShaderResource)
    {
        const D3D12_SHADER_RESOURCE_VIEW_DESC srv_desc = {
            .Format = ToDXGIFormat(info.format),
            .ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D,
            .Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING,
            .Texture2D =
                {
                    .MostDetailedMip = 0,
                    .MipLevels = info.mip_levels,
                    .PlaneSlice = 0,
                    .ResourceMinLODClamp = 0.0f,
                },
        };
        m_srv_descriptor = srv_heap->Allocate();
        device->CreateShaderResourceView(dx_resource, &srv_desc, m_srv_descriptor->cpu_handle);
    }
}

Swift::D3D12::Texture::~Texture()
{
    if (m_rtv_descriptor)
    {
        m_rtv_heap->Free(*m_rtv_descriptor);
    }
    if (m_dsv_descriptor)
    {
        m_dsv_heap->Free(*m_dsv_descriptor);
    }
    if (m_srv_descriptor)
    {
        m_srv_heap->Free(*m_srv_descriptor);
    }
}
