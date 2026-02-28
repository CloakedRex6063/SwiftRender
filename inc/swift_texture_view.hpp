#pragma once
#include "swift_structs.hpp"
#include "swift_texture.hpp"

namespace Swift
{
    class ITextureView
    {
    public:
        SWIFT_DESTRUCT(ITextureView);
        SWIFT_NO_COPY(ITextureView);
        SWIFT_NO_MOVE(ITextureView);
        [[nodiscard]] virtual uint32_t GetDescriptorIndex() = 0;
        [[nodiscard]] ITexture* GetTexture() const { return m_texture; }

    protected:
        ITextureView(ITexture* texture, const TextureViewType view_type) : m_texture(texture), m_type(view_type) {}
        ITexture* m_texture;
        TextureViewType m_type;
    };
}