#include "d3d12/d3d12_descriptor.hpp"
#include "d3d12/d3d12_helpers.hpp"
#include "d3d12/d3d12_context.hpp"

Swift::D3D12::RenderTarget::RenderTarget(Context* context, ITexture* texture, const uint32_t mip)
    : IRenderTarget(texture), D3D12Descriptor(context)
{
    const auto& rtv_heap = context->GetRTVHeap();
    m_data = rtv_heap->Allocate();
    const D3D12_RENDER_TARGET_VIEW_DESC rtv_desc = {.Format = ToDXGIFormat(texture->GetFormat()),
                                                    .ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D,
                                                    .Texture2D = {
                                                        .MipSlice = mip,
                                                    }};
    auto* device = static_cast<ID3D12Device*>(context->GetDevice());
    auto* const resource = static_cast<ID3D12Resource*>(texture->GetResource()->GetResource());
    device->CreateRenderTargetView(resource, &rtv_desc, m_data.cpu_handle);
}

Swift::D3D12::RenderTarget::~RenderTarget() { m_context->GetRTVHeap()->Free(m_data); }

Swift::D3D12::DepthStencil::DepthStencil(Context* context, ITexture* texture, const uint32_t mip)
    : IDepthStencil(texture), D3D12Descriptor(context)
{
    auto* device = static_cast<ID3D12Device*>(context->GetDevice());
    auto* const resource = static_cast<ID3D12Resource*>(texture->GetResource()->GetResource());

    const auto& dsv_heap = context->GetDSVHeap();
    m_data = dsv_heap->Allocate();
    const D3D12_DEPTH_STENCIL_VIEW_DESC dsv_desc = {.Format = ToDXGIFormat(texture->GetFormat()),
                                                    .ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D,
                                                    .Texture2D = {
                                                        .MipSlice = mip,
                                                    }};
    device->CreateDepthStencilView(resource, &dsv_desc, m_data.cpu_handle);
}
Swift::D3D12::DepthStencil::~DepthStencil() { m_context->GetDSVHeap()->Free(m_data); }

Swift::D3D12::TextureSRV::~TextureSRV() { m_context->GetCBVSRVUAVHeap()->Free(m_data); }

Swift::D3D12::BufferSRV::~BufferSRV() { m_context->GetCBVSRVUAVHeap()->Free(m_data); }

Swift::D3D12::TextureUAV::~TextureUAV() { m_context->GetCBVSRVUAVHeap()->Free(m_data); }

Swift::D3D12::BufferUAV::~BufferUAV() { m_context->GetCBVSRVUAVHeap()->Free(m_data); }

Swift::D3D12::BufferCBV::~BufferCBV() { m_context->GetCBVSRVUAVHeap()->Free(m_data); }

Swift::D3D12::TextureSRV::TextureSRV(Context* context,
                                     ITexture* texture,
                                     const uint32_t most_detailed_mip,
                                     const uint32_t mip_levels)
    : ITextureSRV(texture), D3D12Descriptor(context)
{
    auto* device = static_cast<ID3D12Device*>(context->GetDevice());
    auto* const resource = static_cast<ID3D12Resource*>(texture->GetResource()->GetResource());
    const auto& srv_heap = context->GetCBVSRVUAVHeap();
    m_data = srv_heap->Allocate();
    D3D12_SHADER_RESOURCE_VIEW_DESC srv_desc = {
        .Format = ToDXGIFormat(texture->GetFormat()),
        .ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D,
        .Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING,
        .Texture2D =
            {
                .MostDetailedMip = most_detailed_mip,
                .MipLevels = mip_levels,
            },
    };
    device->CreateShaderResourceView(resource, &srv_desc, m_data.cpu_handle);
}
Swift::D3D12::BufferSRV::BufferSRV(Context* context, IBuffer* buffer, const BufferSRVCreateInfo& info)
    : IBufferSRV(buffer), D3D12Descriptor(context)
{
    const auto& cbv_heap = context->GetCBVSRVUAVHeap();
    m_data = cbv_heap->Allocate();
    auto* const resource = static_cast<ID3D12Resource*>(buffer->GetResource()->GetResource());
    const D3D12_SHADER_RESOURCE_VIEW_DESC desc{.Format = DXGI_FORMAT_UNKNOWN,
                                               .ViewDimension = D3D12_SRV_DIMENSION_BUFFER,
                                               .Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING,
                                               .Buffer = {
                                                   .FirstElement = info.first_element,
                                                   .NumElements = info.num_elements,
                                                   .StructureByteStride = info.element_size,
                                               }};
    auto* device = static_cast<ID3D12Device*>(context->GetDevice());
    device->CreateShaderResourceView(resource, &desc, m_data.cpu_handle);
}

Swift::D3D12::TextureUAV::TextureUAV(Context* context, ITexture* texture, const uint32_t mip)
    : ITextureUAV(texture), D3D12Descriptor(context)
{
    const auto& cbv_heap = context->GetCBVSRVUAVHeap();
    m_data = cbv_heap->Allocate();
    auto* const resource = static_cast<ID3D12Resource*>(texture->GetResource()->GetResource());
    D3D12_UNORDERED_ACCESS_VIEW_DESC uav_desc = {
        .Format = ToDXGIFormat(texture->GetFormat()),
        .ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D,
        .Texture2D =
            {
                .MipSlice = mip,
            },
    };
    auto* device = static_cast<ID3D12Device*>(context->GetDevice());
    device->CreateUnorderedAccessView(resource, nullptr, &uav_desc, m_data.cpu_handle);
}

Swift::D3D12::BufferUAV::BufferUAV(Context* context, IBuffer* buffer, const BufferUAVCreateInfo& info)
    : IBufferUAV(buffer), D3D12Descriptor(context)
{
    const auto& cbv_heap = context->GetCBVSRVUAVHeap();
    m_data = cbv_heap->Allocate();
    auto* const resource = static_cast<ID3D12Resource*>(buffer->GetResource()->GetResource());
    const D3D12_UNORDERED_ACCESS_VIEW_DESC uav_desc = {
        .Format = DXGI_FORMAT_UNKNOWN,
        .ViewDimension = D3D12_UAV_DIMENSION_BUFFER,
        .Buffer =
            {
                .FirstElement = info.first_element,
                .NumElements = info.num_elements,
                .StructureByteStride = info.element_size,
            },
    };
    auto* device = static_cast<ID3D12Device*>(context->GetDevice());
    device->CreateUnorderedAccessView(resource, nullptr, &uav_desc, m_data.cpu_handle);
}

Swift::D3D12::BufferCBV::BufferCBV(Context* context, IBuffer* buffer, const uint32_t size, const uint32_t offset)
    : IBufferCBV(buffer), D3D12Descriptor(context)
{
    const auto& cbv_heap = context->GetCBVSRVUAVHeap();
    m_data = cbv_heap->Allocate();
    auto* const resource = static_cast<ID3D12Resource*>(buffer->GetResource()->GetResource());
    const D3D12_CONSTANT_BUFFER_VIEW_DESC desc{
        .BufferLocation = resource->GetGPUVirtualAddress() + offset,
        .SizeInBytes = size,
    };
    auto* device = static_cast<ID3D12Device*>(context->GetDevice());
    device->CreateConstantBufferView(&desc, m_data.cpu_handle);
}

Swift::D3D12::DescriptorHeap::DescriptorHeap(ID3D12Device14* device,
                                             const D3D12_DESCRIPTOR_HEAP_TYPE heap_type,
                                             const uint32_t count)
    : m_heap_type(heap_type)
{
    auto flag = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    if (heap_type == D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV)
    {
        flag = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    }
    const D3D12_DESCRIPTOR_HEAP_DESC desc = {
        .Type = heap_type,
        .NumDescriptors = count,
        .Flags = flag,
        .NodeMask = 0,
    };
    [[maybe_unused]] const auto result = device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&m_heap));
    m_stride = device->GetDescriptorHandleIncrementSize(heap_type);
    m_cpu_base = m_heap->GetCPUDescriptorHandleForHeapStart();
    if (desc.Flags & D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE)
    {
        m_gpu_base = m_heap->GetGPUDescriptorHandleForHeapStart();
    }
    switch (heap_type)
    {
        case D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV:
            m_heap->SetName(L"CBV_SRV_UAV Heap");
            break;
        case D3D12_DESCRIPTOR_HEAP_TYPE_RTV:
            m_heap->SetName(L"RTV Heap");
            break;
        case D3D12_DESCRIPTOR_HEAP_TYPE_DSV:
            m_heap->SetName(L"DSV Heap");
            break;
        default:
            break;
    }
}

Swift::D3D12::DescriptorHeap::~DescriptorHeap()
{
    m_heap->Release();
}

Swift::D3D12::DescriptorData Swift::D3D12::DescriptorHeap::Allocate()
{
    if (m_free_descriptors.empty())
    {
        const auto index = m_index++;
        auto cpu_handle = m_cpu_base;
        cpu_handle.ptr += index * m_stride;
        auto gpu_handle = m_gpu_base;
        if (m_heap_type == D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV)
        {
            gpu_handle.ptr += index * m_stride;
        }
        m_descriptors.push_back({cpu_handle, gpu_handle, index});
        return m_descriptors.back();
    }
    const auto index = m_free_descriptors.back();
    m_free_descriptors.pop_back();
    return m_descriptors[index];
}

void Swift::D3D12::DescriptorHeap::Free(const DescriptorData& descriptor) { m_free_descriptors.push_back(descriptor.index); }