#pragma once
#include "d3d12_context.hpp"
#include "swift_texture.hpp"

namespace Swift::D3D12
{
    class Texture final : public ITexture
    {
    public:
        Texture(Context* context, const TextureCreateInfo& info);
        SWIFT_NO_COPY(Texture);
        SWIFT_NO_MOVE(Texture);
        SWIFT_DESTRUCT(Texture);
    };
}  // namespace Swift::D3D12
