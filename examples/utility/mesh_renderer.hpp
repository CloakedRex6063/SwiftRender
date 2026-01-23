#pragma once
#include "swift_builders.hpp"

struct TextureView
{
    Swift::ITexture* texture;
    Swift::ITextureSRV* texture_srv;
};

struct MeshRenderer
{
    uint32_t m_vertex_buffer;
    uint32_t m_mesh_buffer;
    uint32_t m_mesh_vertex_buffer;
    uint32_t m_mesh_triangle_buffer;
    uint32_t m_meshlet_count;
    int m_material_index;
    uint32_t m_transform_index;

    void Draw(Swift::ICommand* command) const
    {
        const struct PushConstants
        {
            uint32_t vertex_buffer;
            uint32_t meshlet_buffer;
            uint32_t mesh_vertex_buffer;
            uint32_t mesh_triangle_buffer;
            int material_index;
            uint32_t transform_index;
        } push_constants{
            .vertex_buffer = m_vertex_buffer,
            .meshlet_buffer = m_mesh_buffer,
            .mesh_vertex_buffer = m_mesh_vertex_buffer,
            .mesh_triangle_buffer = m_mesh_triangle_buffer,
            .material_index = m_material_index,
            .transform_index = m_transform_index,
        };
        command->PushConstants(&push_constants, sizeof(PushConstants));
        command->DispatchMesh(m_meshlet_count, 1, 1);
    }
};

struct MeshBuffers
{
    Swift::IBuffer* m_vertex_buffer;
    Swift::IBufferSRV* m_vertex_buffer_srv;
    Swift::IBuffer* m_mesh_buffer;
    Swift::IBufferSRV* m_mesh_buffer_srv;
    Swift::IBuffer* m_mesh_vertex_buffer;
    Swift::IBufferSRV* m_mesh_vertex_buffer_srv;
    Swift::IBuffer* m_mesh_triangle_buffer;
    Swift::IBufferSRV* m_mesh_triangle_buffer_srv;
};

inline std::vector<MeshBuffers> CreateMeshBuffers(Swift::IContext* context, const std::span<Mesh>& meshes)
{
    std::vector<MeshBuffers> mesh_buffers;
    for (auto& mesh : meshes)
    {
        auto* const vertex_buffer =
            Swift::BufferBuilder(context, sizeof(Vertex) * mesh.vertices.size()).SetData(mesh.vertices.data()).Build();
        auto* const vertex_buffer_srv = context->CreateShaderResource(
            vertex_buffer,
            Swift::BufferSRVCreateInfo{.num_elements = static_cast<uint32_t>(mesh.vertices.size()),
                                       .element_size = sizeof(Vertex)});
        auto* const meshlet_buffer =
            Swift::BufferBuilder(context, sizeof(meshopt_Meshlet) * mesh.meshlets.size()).SetData(mesh.meshlets.data()).Build();
        auto* const meshlet_buffer_srv = context->CreateShaderResource(
            meshlet_buffer,
            Swift::BufferSRVCreateInfo{.num_elements = static_cast<uint32_t>(mesh.meshlets.size()),
                                       .element_size = sizeof(meshopt_Meshlet)});
        auto* const mesh_vertex_buffer = Swift::BufferBuilder(context, sizeof(uint32_t) * mesh.meshlet_vertices.size())
                                             .SetData(mesh.meshlet_vertices.data())
                                             .Build();
        auto* const mesh_vertex_buffer_srv = context->CreateShaderResource(
            mesh_vertex_buffer,
            Swift::BufferSRVCreateInfo{.num_elements = static_cast<uint32_t>(mesh.meshlet_vertices.size()),
                                       .element_size = sizeof(uint32_t)});
        auto* const mesh_triangle_buffer = Swift::BufferBuilder(context, sizeof(uint32_t) * mesh.meshlet_triangles.size())
                                               .SetData(mesh.meshlet_triangles.data())
                                               .Build();
        auto* const mesh_triangle_buffer_srv = context->CreateShaderResource(
            mesh_triangle_buffer,
            Swift::BufferSRVCreateInfo{.num_elements = static_cast<uint32_t>(mesh.meshlet_triangles.size()),
                                       .element_size = sizeof(uint32_t)});
        const auto mesh_buffer = MeshBuffers{
            .m_vertex_buffer = vertex_buffer,
            .m_vertex_buffer_srv = vertex_buffer_srv,
            .m_mesh_buffer = meshlet_buffer,
            .m_mesh_buffer_srv = meshlet_buffer_srv,
            .m_mesh_vertex_buffer = mesh_vertex_buffer,
            .m_mesh_vertex_buffer_srv = mesh_vertex_buffer_srv,
            .m_mesh_triangle_buffer = mesh_triangle_buffer,
            .m_mesh_triangle_buffer_srv = mesh_triangle_buffer_srv,
        };
        mesh_buffers.emplace_back(mesh_buffer);
    }
    return mesh_buffers;
}

inline std::vector<MeshRenderer> CreateMeshRenderers(const std::span<const Node> nodes,
                                                     const std::span<const Mesh> meshes,
                                                     const std::span<const MeshBuffers> mesh_buffers)
{
    std::vector<MeshRenderer> mesh_renderers;
    for (const auto& node : nodes)
    {
        const auto& mesh = meshes[node.mesh_index];
        const auto& mesh_buffer = mesh_buffers[node.mesh_index];
        MeshRenderer mesh_renderer{
            .m_vertex_buffer = mesh_buffer.m_vertex_buffer_srv->GetDescriptorIndex(),
            .m_mesh_buffer = mesh_buffer.m_mesh_buffer_srv->GetDescriptorIndex(),
            .m_mesh_vertex_buffer = mesh_buffer.m_mesh_vertex_buffer_srv->GetDescriptorIndex(),
            .m_mesh_triangle_buffer = mesh_buffer.m_mesh_triangle_buffer_srv->GetDescriptorIndex(),
            .m_meshlet_count = static_cast<uint32_t>(mesh.meshlets.size()),
            .m_material_index = mesh.material_index,
            .m_transform_index = node.transform_index,
        };
        mesh_renderers.push_back(mesh_renderer);
    }
    return mesh_renderers;
}

inline std::vector<TextureView> CreateTextures(Swift::IContext* context,
                                               Swift::IHeap* heap,
                                               const std::span<Texture> textures,
                                               std::span<Material> materials)
{
    std::vector<TextureView> output_textures;
    uint32_t offset = 0;
    for (auto& texture : textures)
    {
        auto texture_builder = Swift::TextureBuilder(context, texture.width, texture.height)
                                   .SetFormat(texture.format)
                                   .SetArraySize(texture.array_size)
                                   .SetMipmapLevels(0)
                                   .SetData(texture.pixels.data())
                                   .SetName(texture.name);
        auto info = texture_builder.GetBuildInfo();
        texture_builder.SetResource(heap->CreateResource(info, offset));
        offset += context->CalculateAlignedTextureSize(info);
        auto* t = texture_builder.Build();

        auto* srv = context->CreateShaderResource(t);
        output_textures.emplace_back(TextureView{t, srv});
    }

    for (auto& material : materials)
    {
        if (material.albedo_index != -1)
        {
            material.albedo_index = output_textures[material.albedo_index].texture_srv->GetDescriptorIndex();
        }
        if (material.metal_rough_index != -1)
        {
            material.metal_rough_index = output_textures[material.metal_rough_index].texture_srv->GetDescriptorIndex();
        }
        if (material.occlusion_index != -1)
        {
            material.occlusion_index = output_textures[material.occlusion_index].texture_srv->GetDescriptorIndex();
        }
        if (material.emissive_index != -1)
        {
            material.emissive_index = output_textures[material.emissive_index].texture_srv->GetDescriptorIndex();
        }
        if (material.normal_index != -1)
        {
            material.normal_index = output_textures[material.normal_index].texture_srv->GetDescriptorIndex();
        }
    }
    return output_textures;
}

inline void DestroyMeshBuffers(Swift::IContext* context, std::span<const MeshBuffers> mesh_buffers)
{
    for (const auto& [m_vertex_buffer,
                      m_vertex_buffer_srv,
                      m_mesh_buffer,
                      m_mesh_buffer_srv,
                      m_mesh_vertex_buffer,
                      m_mesh_vertex_buffer_srv,
                      m_mesh_triangle_buffer,
                      m_mesh_triangle_buffer_srv] : mesh_buffers)
    {
        context->DestroyBuffer(m_mesh_buffer);
        context->DestroyShaderResource(m_mesh_buffer_srv);
        context->DestroyBuffer(m_vertex_buffer);
        context->DestroyShaderResource(m_vertex_buffer_srv);
        context->DestroyBuffer(m_mesh_vertex_buffer);
        context->DestroyShaderResource(m_mesh_vertex_buffer_srv);
        context->DestroyBuffer(m_mesh_triangle_buffer);
        context->DestroyShaderResource(m_mesh_triangle_buffer_srv);
    }
}

inline void DestroyTextures(Swift::IContext* context, const std::span<const TextureView> textures)
{
    for (const auto& [texture, texture_srv] : textures)
    {
        context->DestroyTexture(texture);
        context->DestroyShaderResource(texture_srv);
    }
}
