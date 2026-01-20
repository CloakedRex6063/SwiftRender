#pragma once
#include "swift_builders.hpp"

struct MeshRenderer
{
    uint32_t m_vertex_buffer;
    uint32_t m_mesh_buffer;
    uint32_t m_mesh_vertex_buffer;
    uint32_t m_mesh_triangle_buffer;
    uint32_t m_meshlet_count;
    int m_material_index;
    uint32_t m_transform_index;

    void Draw(const std::shared_ptr<Swift::ICommand>& command) const
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
    std::shared_ptr<Swift::IBuffer> m_vertex_buffer;
    std::shared_ptr<Swift::IBuffer> m_mesh_buffer;
    std::shared_ptr<Swift::IBuffer> m_mesh_vertex_buffer;
    std::shared_ptr<Swift::IBuffer> m_mesh_triangle_buffer;
};

inline std::vector<MeshBuffers> CreateMeshBuffers(const std::shared_ptr<Swift::IContext>& context,
                                                  const std::span<Mesh>& meshes)
{
    std::vector<MeshBuffers> mesh_buffers;
    for (auto& mesh : meshes)
    {
        const auto vertex_buffer = Swift::BufferBuilder(context, Swift::BufferType::eStructuredBuffer)
                                       .SetElementSize(sizeof(Vertex))
                                       .SetNumElements(mesh.vertices.size())
                                       .SetData(mesh.vertices.data())
                                       .Build();
        const auto meshlet_buffer = Swift::BufferBuilder(context, Swift::BufferType::eStructuredBuffer)
                                        .SetElementSize(sizeof(meshopt_Meshlet))
                                        .SetNumElements(mesh.meshlets.size())
                                        .SetData(mesh.meshlets.data())
                                        .Build();
        const auto mesh_vertex_buffer = Swift::BufferBuilder(context, Swift::BufferType::eStructuredBuffer)
                                            .SetElementSize(sizeof(uint32_t))
                                            .SetNumElements(mesh.meshlet_vertices.size())
                                            .SetData(mesh.meshlet_vertices.data())
                                            .Build();
        const auto mesh_triangle_buffer = Swift::BufferBuilder(context, Swift::BufferType::eStructuredBuffer)
                                              .SetElementSize(sizeof(uint32_t))
                                              .SetNumElements(mesh.meshlet_triangles.size())
                                              .SetData(mesh.meshlet_triangles.data())
                                              .Build();
        const auto mesh_buffer = MeshBuffers{
            .m_vertex_buffer = vertex_buffer,
            .m_mesh_buffer = meshlet_buffer,
            .m_mesh_vertex_buffer = mesh_vertex_buffer,
            .m_mesh_triangle_buffer = mesh_triangle_buffer,
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
        const auto& [m_vertex_buffer, m_mesh_buffer, m_mesh_vertex_buffer, m_mesh_triangle_buffer] =
            mesh_buffers[node.mesh_index];
        MeshRenderer mesh_renderer{
            .m_vertex_buffer = m_vertex_buffer->GetDescriptorIndex(),
            .m_mesh_buffer = m_mesh_buffer->GetDescriptorIndex(),
            .m_mesh_vertex_buffer = m_mesh_vertex_buffer->GetDescriptorIndex(),
            .m_mesh_triangle_buffer = m_mesh_triangle_buffer->GetDescriptorIndex(),
            .m_meshlet_count = static_cast<uint32_t>(mesh.meshlets.size()),
            .m_material_index = mesh.material_index,
            .m_transform_index = node.transform_index,
        };
        mesh_renderers.push_back(mesh_renderer);
    }
    return mesh_renderers;
}

inline std::vector<std::shared_ptr<Swift::ITexture>> CreateTextures(const std::shared_ptr<Swift::IContext>& context,
                                                                    const std::span<Texture> textures,
                                                                    std::span<Material> materials)
{
    std::vector<std::shared_ptr<Swift::ITexture>> output_textures;
    for (auto& texture : textures)
    {
        const auto t = Swift::TextureBuilder(context, texture.width, texture.height)
                           .SetFormat(texture.format)
                           .SetArraySize(texture.array_size)
                           .SetMipmapLevels(texture.mip_levels)
                           .SetFlags(Swift::TextureFlags::eShaderResource)
                           .SetData(texture.pixels.data())
                           .Build();
        output_textures.emplace_back(t);
    }

    for (auto& material : materials)
    {
        if (material.albedo_index != -1)
        {
            material.albedo_index = output_textures[material.albedo_index]->GetDescriptorIndex();
        }
        if (material.metal_rough_index != -1)
        {
            material.metal_rough_index = output_textures[material.metal_rough_index]->GetDescriptorIndex();
        }
        if (material.occlusion_index != -1)
        {
            material.occlusion_index = output_textures[material.occlusion_index]->GetDescriptorIndex();
        }
        if (material.emissive_index != -1)
        {
            material.emissive_index = output_textures[material.emissive_index]->GetDescriptorIndex();
        }
        if (material.normal_index != -1)
        {
            material.normal_index = output_textures[material.normal_index]->GetDescriptorIndex();
        }
    }
    return output_textures;
}
