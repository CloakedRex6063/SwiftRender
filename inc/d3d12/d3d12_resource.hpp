#pragma once
#include "swift_heap.hpp"
#include "swift_resource.hpp"
#define NOMINMAX
#include "directx/d3d12.h"

namespace Swift::D3D12
{
    class Resource final : public IResource
    {
    public:
        explicit Resource(ID3D12Resource* resource);
        Resource(ID3D12Device14* device, const TextureCreateInfo& info, ID3D12Heap1* heap = nullptr, uint32_t offset = 0);
        Resource(ID3D12Device14* device, const BufferCreateInfo& info, ID3D12Heap1* heap = nullptr, uint32_t offset = 0);
        ~Resource() override;
        void* GetResource() override { return m_resource; }
        uint64_t GetVirtualAddress() override;
        void Map(void* readback = nullptr, uint32_t size = 0) override;
        void Unmap() override;

    private:
        static D3D12_RESOURCE_DESC GetResourceDesc(const BufferCreateInfo& info);
        static D3D12_RESOURCE_DESC GetResourceDesc(const TextureCreateInfo& info);
        static ID3D12Resource* CreatePlacedResource(ID3D12Device14* device, ID3D12Heap1* heap, uint32_t offset, const BufferCreateInfo& info);
        static ID3D12Resource* CreatePlacedResource(ID3D12Device14* device, ID3D12Heap1* heap, uint32_t offset, const TextureCreateInfo& info);
        static ID3D12Resource* CreateCommittedResource(ID3D12Device14* device, const BufferCreateInfo& info);
        static ID3D12Resource* CreateCommittedResource(ID3D12Device14* device, const TextureCreateInfo& info);
        ID3D12Resource* m_resource = nullptr;
    };
}  // namespace Swift::D3D12
