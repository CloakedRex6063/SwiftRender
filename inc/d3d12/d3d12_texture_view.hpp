#pragma once
#include "d3d12_descriptor.hpp"
#include "swift_texture_view.hpp"

namespace Swift::D3D12
{
    class Context;
    class TextureView : public ITextureView
    {
    public:
        SWIFT_NO_COPY(TextureView);
        SWIFT_NO_MOVE(TextureView);
        TextureView(Context* context, ITexture* texture, const TextureViewCreateInfo& texture_view_create_info);
        ~TextureView() override;
        [[nodiscard]] uint32_t GetDescriptorIndex() override { return m_descriptor_data.index; }
        [[nodiscard]] DescriptorData GetDescriptorData() const { return m_descriptor_data; }
        
    private:
        void CreateRenderTarget(const Context* context, ITexture* texture, const TextureViewCreateInfo& texture_view_create_info);
        void CreateDepthStencil(const Context* context, ITexture* texture, const TextureViewCreateInfo& texture_view_create_info);
        void CreateShaderResource(const Context* context,
                                  ITexture* texture,
                                  const TextureViewCreateInfo& texture_view_create_info);
        void CreateUnorderedAccess(const Context* context, ITexture* texture, const TextureViewCreateInfo& texture_view_create_info);
        
        DescriptorData m_descriptor_data;
        Context* m_context;
    };
}
