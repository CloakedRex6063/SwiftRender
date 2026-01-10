#pragma once
#include "swift_resource.hpp"
#include "directx/d3d12.h"

namespace Swift::D3D12
{
    class Resource final : public IResource
    {
    public:
        explicit Resource(ID3D12Resource* resource);
        Resource(ID3D12Device14* device, const TextureCreateInfo& info);
        Resource(ID3D12Device14* device, const BufferCreateInfo& info);
        ~Resource() override;
        void* GetResource() override { return m_resource; }
        uint64_t GetVirtualAddress() override;
        void Map(void* readback = nullptr, uint32_t size = 0) override;
        void Unmap() override;

    private:
        ID3D12Resource* m_resource;
    };
}
