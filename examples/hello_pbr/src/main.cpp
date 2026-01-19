#include "camera.hpp"
#include "importer.hpp"
#include "swift.hpp"
#include "window.hpp"
#include "shader_compiler.hpp"
#include "glm/fwd.hpp"
#include "glm/gtc/type_ptr.hpp"
#include "glm/gtx/compatibility.hpp"
#include "chrono"
#include "swift_builders.hpp"

int main()
{
    auto window = Window();
    const auto window_size = window.GetSize();
    const auto context = Swift::CreateContext({.backend_type = Swift::BackendType::eD3D12,
                                               .width = window_size.x,
                                               .height = window_size.y,
                                               .native_window_handle = window.GetNativeWindow(),
                                               .native_display_handle = nullptr});

    const auto render_texture =
        Swift::TextureBuilder(context, window_size[0], window_size[1])
            .SetFlags(EnumFlags(Swift::TextureFlags::eRenderTarget) | EnumFlags(Swift::TextureFlags::eShaderResource))
            .SetFormat(Swift::Format::eRGBA8_UNORM)
            .Build();

    const auto depth_texture = Swift::TextureBuilder(context, window_size[0], window_size[1])
                                   .SetFlags(EnumFlags(Swift::TextureFlags::eDepthStencil))
                                   .SetFormat(Swift::Format::eD32F)
                                   .Build();

    ShaderCompiler compiler{};

    auto mesh_shader = compiler.CompileShader("hello_pbr.slang", ShaderStage::eMesh);
    auto pixel_shader = compiler.CompileShader("hello_pbr.slang", ShaderStage::ePixel);

    Importer importer{};
    auto helmet = importer.LoadModel("assets/damaged_helmet.glb");

    std::vector<Swift::SamplerDescriptor> sampler_descriptors{};
    for (const auto& sampler : helmet.samplers)
    {
        const auto sampler_descriptor = Swift::SamplerDescriptorBuilder()
                                            .SetMinFilter(sampler.min_filter)
                                            .SetMagFilter(sampler.mag_filter)
                                            .SetWrapU(sampler.wrap_u)
                                            .SetWrapY(sampler.wrap_y)
                                            .Build();
        sampler_descriptors.emplace_back(sampler_descriptor);
    }

    std::vector<Swift::Descriptor> descriptors{};
    const auto descriptor = Swift::DescriptorBuilder(Swift::DescriptorType::eConstant).SetShaderRegister(1).Build();
    descriptors.emplace_back(descriptor);

    const auto shader = Swift::GraphicsShaderBuilder(context)
                            .SetRTVFormats({Swift::Format::eRGBA8_UNORM})
                            .SetDSVFormat(Swift::Format::eD32F)
                            .SetMeshShader(mesh_shader)
                            .SetPixelShader(pixel_shader)
                            .SetDepthTestEnable(true)
                            .SetDepthWriteEnable(true)
                            .SetDepthTest(Swift::DepthTest::eLess)
                            .SetPolygonMode(Swift::PolygonMode::eTriangle)
                            .SetDescriptors(descriptors)
                            .SetStaticSamplers(sampler_descriptors)
                            .Build();

    struct ConstantBufferInfo
    {
        glm::mat4 view_proj;
        glm::float3 cam_pos;
        uint32_t material_buffer_index;
        uint32_t transform_buffer_index;
        glm::float3 padding;
    };

    const auto constant_buffer = Swift::BufferBuilder(context, Swift::BufferType::eConstantBuffer, 65536).Build();

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

    std::vector<MeshRenderer> mesh_renderers;

    struct MeshBuffers
    {
        std::shared_ptr<Swift::IBuffer> m_vertex_buffer;
        std::shared_ptr<Swift::IBuffer> m_mesh_buffer;
        std::shared_ptr<Swift::IBuffer> m_mesh_vertex_buffer;
        std::shared_ptr<Swift::IBuffer> m_mesh_triangle_buffer;
    };
    std::vector<MeshBuffers> mesh_buffers;
    for (auto& mesh : helmet.meshes)
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

        MeshBuffers mesh_buffer{
            .m_vertex_buffer = vertex_buffer,
            .m_mesh_buffer = meshlet_buffer,
            .m_mesh_vertex_buffer = mesh_vertex_buffer,
            .m_mesh_triangle_buffer = mesh_triangle_buffer,
        };
        mesh_buffers.emplace_back(mesh_buffer);
    }

    for (auto& node : helmet.nodes)
    {
        const auto& mesh = helmet.meshes[node.mesh_index];
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

    std::vector<std::shared_ptr<Swift::ITexture>> textures;
    for (auto& texture : helmet.textures)
    {
        const auto t = Swift::TextureBuilder(context, texture.width, texture.height)
                           .SetFormat(texture.format)
                           .SetArraySize(texture.array_size)
                           .SetMipmapLevels(texture.mip_levels)
                           .SetFlags(Swift::TextureFlags::eShaderResource)
                           .SetData(texture.pixels.data())
                           .Build();
        textures.emplace_back(t);
    }

    for (auto& material : helmet.materials)
    {
        if (material.albedo_index != -1)
        {
            material.albedo_index = textures[material.albedo_index]->GetDescriptorIndex();
        }
        if (material.metal_rough_index != -1)
        {
            material.metal_rough_index = textures[material.metal_rough_index]->GetDescriptorIndex();
        }
        if (material.occlusion_index != -1)
        {
            material.occlusion_index = textures[material.occlusion_index]->GetDescriptorIndex();
        }
        if (material.emissive_index != -1)
        {
            material.emissive_index = textures[material.emissive_index]->GetDescriptorIndex();
        }
        if (material.normal_index != -1)
        {
            material.normal_index = textures[material.normal_index]->GetDescriptorIndex();
        }
    }

    const auto material_buffer = Swift::BufferBuilder(context, Swift::BufferType::eStructuredBuffer)
                                     .SetElementSize(sizeof(Material))
                                     .SetNumElements(helmet.materials.size())
                                     .SetData(helmet.materials.data())
                                     .Build();
    const auto transforms_buffer = Swift::BufferBuilder(context, Swift::BufferType::eStructuredBuffer)
                                       .SetElementSize(sizeof(glm::mat4))
                                       .SetNumElements(helmet.transforms.size())
                                       .SetData(helmet.transforms.data())
                                       .Build();

    Camera camera{};
    Input input{window};

    auto prev_time = std::chrono::high_resolution_clock::now();
    while (window.IsRunning())
    {
        input.Tick();
        window.PollEvents();

        const auto& command = context->GetCurrentCommand();

        const auto window_size = window.GetSize();
        const auto float_size = std::array{static_cast<float>(window_size[0]), static_cast<float>(window_size[1])};

        auto& render_target = context->GetCurrentSwapchainTexture();

        const auto current_time = std::chrono::high_resolution_clock::now();
        const auto delta_time = std::chrono::duration<float>(current_time - prev_time).count();
        prev_time = current_time;

        camera.Tick(window, input, delta_time);

        const ConstantBufferInfo scene_buffer_data{
            .view_proj = camera.m_proj_matrix * camera.m_view_matrix,
            .cam_pos = camera.m_position,
            .material_buffer_index = material_buffer->GetDescriptorIndex(),
            .transform_buffer_index = transforms_buffer->GetDescriptorIndex(),
        };
        constant_buffer->Write(&scene_buffer_data, 0, sizeof(ConstantBufferInfo));

        command->Begin();
        command->SetViewport(Swift::Viewport{.dimensions = float_size});
        command->SetScissor(Swift::Scissor{.dimensions = {window_size.x, window_size.y}});
        command->ClearRenderTarget(render_target, {0.0f, 0.0f, 0.0f, 0.0f});
        command->ClearDepthStencil(depth_texture, 1.f, 0.f);
        command->BindShader(shader);
        command->BindConstantBuffer(constant_buffer, 1);
        command->BindRenderTargets(std::array{render_target}, depth_texture);
        for (auto& mesh : mesh_renderers)
        {
            mesh.Draw(command);
        }
        command->End();

        context->Present(false);
    }
}
