#include "d3d12/d3d12_sampler.hpp"

#include "d3d12_helpers.hpp"
#include "d3d12/d3d12_context.hpp"

Swift::D3D12::Sampler::Sampler(Context* context, const SamplerCreateInfo& create_info) : m_context(context)
{
    auto* device = static_cast<ID3D12Device*>(context->GetDevice());
    D3D12_SAMPLER_DESC sampler_desc = {
        .Filter = ToFilter(create_info.min_filter, create_info.mag_filter, create_info.reduction_type),
        .AddressU = ToWrap(create_info.wrap_u),
        .AddressV = ToWrap(create_info.wrap_y),
        .AddressW = ToWrap(create_info.wrap_w),
        .MipLODBias = 0,
        .MaxAnisotropy = D3D12_DEFAULT_MAX_ANISOTROPY,
        .ComparisonFunc = ToComparisonFunc(create_info.comparison_func),
        .MinLOD = create_info.min_lod,
        .MaxLOD = create_info.max_lod,
    };

    sampler_desc.BorderColor[0] = create_info.border_color.x;
    sampler_desc.BorderColor[1] = create_info.border_color.y;
    sampler_desc.BorderColor[2] = create_info.border_color.z;
    sampler_desc.BorderColor[3] = create_info.border_color.w;

    auto* sampler_heap = context->GetSamplerHeap();
    m_data = sampler_heap->Allocate();
    device->CreateSampler(&sampler_desc, m_data.cpu_handle);
}

Swift::D3D12::Sampler::~Sampler()
{
    auto* sampler_heap = m_context->GetSamplerHeap();
    sampler_heap->Free(m_data);
}
