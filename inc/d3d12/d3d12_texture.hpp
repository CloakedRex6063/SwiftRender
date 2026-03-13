#pragma once
#include "d3d12_context.hpp"
#include "swift_texture.hpp"

namespace Swift::D3D12
{
    class Texture final : public ITexture
    {
    public:
        Texture(ID3D12Resource* resource, const TextureCreateInfo& info);
        Texture(Context* context, const TextureCreateInfo& info);
        ~Texture() override;
        SWIFT_NO_COPY(Texture);
        SWIFT_NO_MOVE(Texture);
        [[nodiscard]] void* GetResource() override { return m_resource; }
        [[nodiscard]] uint64_t GetVirtualAddress() override { return m_resource->GetGPUVirtualAddress(); }

    private:
        static D3D12_RESOURCE_DESC GetResourceDesc(const TextureCreateInfo& info);
        void CreateCommittedResource(const TextureCreateInfo& info);
        ID3D12Resource* m_resource = nullptr;
        D3D12MA::Allocation* m_allocation = nullptr;
        Context* m_context;
    };
}  // namespace Swift::D3D12
