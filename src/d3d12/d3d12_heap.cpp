#include "d3d12/d3d12_heap.hpp"

#include "d3d12_helpers.hpp"

Swift::D3D12::Heap::Heap(Context* context, const HeapCreateInfo& info) : m_context(context)
{
    const auto heap_desc = D3D12_HEAP_DESC{
        .SizeInBytes = info.size,
        .Properties =
            D3D12_HEAP_PROPERTIES{
                .Type = ToHeapType(info.type),
                .CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN,
                .MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN,
            },
        .Alignment = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT,
        .Flags = D3D12_HEAP_FLAG_NONE,
    };
    auto* device = static_cast<ID3D12Device14*>(m_context->GetDevice());
    device->CreateHeap1(&heap_desc, nullptr, IID_PPV_ARGS(&m_heap));
    const std::wstring debug_name{info.debug_name.begin(), info.debug_name.end()};
    m_heap->SetName(debug_name.c_str());
}

Swift::D3D12::Heap::~Heap() { m_heap->Release(); }

std::shared_ptr<Swift::IResource> Swift::D3D12::Heap::CreateResource(const BufferCreateInfo& info, uint64_t offset)
{
    auto* device = static_cast<ID3D12Device14*>(m_context->GetDevice());
    return std::make_shared<Resource>(device, info, m_heap, offset);
}

std::shared_ptr<Swift::IResource> Swift::D3D12::Heap::CreateResource(const TextureCreateInfo& info, uint64_t offset)
{
    auto* device = static_cast<ID3D12Device14*>(m_context->GetDevice());
    return std::make_shared<Resource>(device, info, m_heap, offset);
}