#include "d3d12/d3d12_texture.hpp"

#include "d3d12_helpers.hpp"

Swift::D3D12::Texture::Texture(Context* context, const TextureCreateInfo& info)
{
    m_resource = info.resource;
    m_format = info.format;
    m_size = {info.width, info.height};
    m_array_size = info.array_size;
    m_mip_levels = info.mip_levels;

    if (!m_resource)
    {
        m_resource = context->CreateResource(info);
    }
}