#pragma once
#include "d3d12_context.hpp"
#include "d3d12_resource.hpp"
#include "swift_heap.hpp"
#include "swift_structs.hpp"
#include "directx/d3d12.h"

namespace Swift::D3D12
{
    class Heap : public IHeap
    {
    public:
        Heap(Context* context, const HeapCreateInfo& info);
        ~Heap() override;

        std::shared_ptr<IResource> CreateResource(const BufferCreateInfo& info, uint64_t offset) override;
        std::shared_ptr<IResource> CreateResource(const TextureCreateInfo& info, uint64_t offset) override;

        SWIFT_NO_CONSTRUCT(Heap);
        SWIFT_NO_COPY(Heap);
        SWIFT_NO_MOVE(Heap);

    private:
        ID3D12Heap1* m_heap = nullptr;
        Context* m_context = nullptr;
    };
}