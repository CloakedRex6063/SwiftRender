#pragma once
#include "d3d12_context.hpp"
#include "swift_texture.hpp"

namespace Swift::D3D12
{
    class Texture final : public ITexture
    {
    public:
        Texture(ID3D12Resource* resource, const TextureCreateInfo& info);
        Texture(const Context* context, const TextureCreateInfo& info);
        SWIFT_NO_COPY(Texture);
        SWIFT_NO_MOVE(Texture);
        SWIFT_DESTRUCT(Texture);
        [[nodiscard]] void* GetResource() override { return m_resource; }
        [[nodiscard]] uint64_t GetVirtualAddress() override { return m_resource->GetGPUVirtualAddress(); }

    private:
        static D3D12_RESOURCE_DESC GetResourceDesc(const TextureCreateInfo& info);
        static ID3D12Resource* CreateCommittedResource(ID3D12Device14* device, const TextureCreateInfo& info);
        ID3D12Resource* m_resource = nullptr;
    };
}  // namespace Swift::D3D12
