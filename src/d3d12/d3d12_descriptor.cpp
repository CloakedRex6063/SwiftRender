#include "d3d12/d3d12_descriptor.hpp"
#include "d3d12/d3d12_helpers.hpp"

Swift::D3D12::DescriptorHeap::DescriptorHeap(ID3D12Device14* device,
                                             const D3D12_DESCRIPTOR_HEAP_TYPE heap_type,
                                             const uint32_t count)
    : m_heap_type(heap_type)
{
    auto flag = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    if (heap_type == D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV || heap_type == D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER)
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

Swift::D3D12::DescriptorHeap::~DescriptorHeap() { m_heap->Release(); }

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