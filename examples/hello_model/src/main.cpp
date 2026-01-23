#include "camera.hpp"
#include "importer.hpp"
#include "swift.hpp"
#include "window.hpp"
#include "shader_compiler.hpp"
#include "glm/gtc/type_ptr.hpp"
#include "glm/gtx/compatibility.hpp"
#include "chrono"

int main()
{
    auto window = Window();
    auto window_size = window.GetSize();
    auto* const context = Swift::CreateContext({.backend_type = Swift::BackendType::eD3D12,
                                                .width = window_size.x,
                                                .height = window_size.y,
                                                .native_window_handle = window.GetNativeWindow(),
                                                .native_display_handle = nullptr});

    const Swift::TextureCreateInfo depth_tex_info{
        .width = window_size[0],
        .height = window_size[1],
        .mip_levels = 1,
        .array_size = 1,
        .format = Swift::Format::eD32F,
        .flags = Swift::TextureFlags::eDepthStencil,
    };
    auto* depth_texture = context->CreateTexture(depth_tex_info);
    auto* depth_stencil = context->CreateDepthStencil(depth_texture);

    ShaderCompiler compiler{};

    auto mesh_shader = compiler.CompileShader("hello_model.slang", ShaderStage::eMesh);
    auto pixel_shader = compiler.CompileShader("hello_model.slang", ShaderStage::ePixel);

    Importer importer{};
    auto chess = importer.LoadModel("assets/chess.glb");

    std::vector<Swift::SamplerDescriptor> sampler_descriptors{};
    for (const auto& sampler : chess.samplers)
    {
        Swift::SamplerDescriptor sampler_descriptor{
            .min_filter = sampler.min_filter,
            .mag_filter = sampler.mag_filter,
            .wrap_u = sampler.wrap_u,
            .wrap_y = sampler.wrap_y,
        };
        sampler_descriptors.emplace_back(sampler_descriptor);
    }

    std::vector<Swift::Descriptor> descriptors{};
    Swift::Descriptor descriptor{
        .shader_register = 1,
        .register_space = 0,
        .descriptor_type = Swift::DescriptorType::eConstant,
        .shader_visibility = Swift::ShaderVisibility::eAll,
    };
    descriptors.emplace_back(descriptor);

    std::vector rtv_formats{Swift::Format::eRGBA8_UNORM};
    const Swift::GraphicsShaderCreateInfo shader_create_info{
        .rtv_formats = rtv_formats,
        .dsv_format = Swift::Format::eD32F,
        .mesh_code = mesh_shader,
        .pixel_code = pixel_shader,
        .depth_stencil_state =
            {
                .depth_enable = true,
                .depth_write_enable = true,
                .depth_test = Swift::DepthTest::eLess,
                .stencil_enable = false,
            },
        .polygon_mode = Swift::PolygonMode::eTriangle,
        .descriptors = descriptors,
        .static_samplers = sampler_descriptors,
    };
    auto* shader = context->CreateShader(shader_create_info);

    struct ConstantBufferInfo
    {
        glm::mat4 view_proj;
        glm::float3 cam_pos;
        uint32_t material_buffer_index;
        uint32_t transform_buffer_index;
        glm::float3 padding;
    };

    const Swift::BufferCreateInfo constant_create_info{
        .size = 65536,
    };
    auto* const constant_buffer = context->CreateBuffer(constant_create_info);

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

    std::vector<MeshRenderer> mesh_renderers;

    struct MeshBuffer
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
    std::vector<MeshBuffer> mesh_buffers;
    for (auto& mesh : chess.meshes)
    {
        auto* const vertex_buffer = context->CreateBuffer(Swift::BufferCreateInfo{
            .size = static_cast<uint32_t>(mesh.vertices.size() * sizeof(Vertex)),
            .data = mesh.vertices.data(),
        });
        auto* const vertex_buffer_srv =
            context->CreateShaderResource(vertex_buffer,
                                          Swift::BufferSRVCreateInfo{
                                              .num_elements = static_cast<uint32_t>(mesh.vertices.size()),
                                              .element_size = sizeof(Vertex),
                                          });

        auto* const meshlet_buffer = context->CreateBuffer(Swift::BufferCreateInfo{
            .size = static_cast<uint32_t>(mesh.meshlets.size() * sizeof(meshopt_Meshlet)),
            .data = mesh.meshlets.data(),
        });
        auto* const meshlet_buffer_srv =
            context->CreateShaderResource(meshlet_buffer,
                                          Swift::BufferSRVCreateInfo{
                                              .num_elements = static_cast<uint32_t>(mesh.meshlets.size()),
                                              .element_size = sizeof(meshopt_Meshlet),
                                          });

        auto* const mesh_vertex_buffer = context->CreateBuffer(Swift::BufferCreateInfo{
            .size = static_cast<uint32_t>(mesh.meshlet_vertices.size() * sizeof(uint32_t)),
            .data = mesh.meshlet_vertices.data(),
        });
        auto* const mesh_vertex_buffer_srv =
            context->CreateShaderResource(mesh_vertex_buffer,
                                          Swift::BufferSRVCreateInfo{
                                              .num_elements = static_cast<uint32_t>(mesh.meshlet_vertices.size()),
                                              .element_size = sizeof(uint32_t),
                                          });

        const Swift::BufferCreateInfo mesh_triangle_create_info{
            .size = static_cast<uint32_t>(mesh.meshlet_triangles.size() * sizeof(uint32_t)),
            .data = mesh.meshlet_triangles.data(),
        };
        auto* const mesh_triangle_buffer = context->CreateBuffer(mesh_triangle_create_info);
        auto* const mesh_triangle_buffer_srv = context->CreateShaderResource(
            mesh_triangle_buffer,
            Swift::BufferSRVCreateInfo{.num_elements = static_cast<uint32_t>(mesh.meshlet_triangles.size()),
                                       .element_size = sizeof(uint32_t)});
        MeshBuffer mesh_buffer{
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

    for (auto& node : chess.nodes)
    {
        const auto& mesh = chess.meshes[node.mesh_index];
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

    struct TextureView
    {
        Swift::ITexture* texture;
        Swift::ITextureSRV* texture_srv;
    };
    std::vector<TextureView> textures;
    for (auto& texture : chess.textures)
    {
        const Swift::TextureCreateInfo texture_create_info{
            .width = texture.width,
            .height = texture.height,
            .mip_levels = texture.mip_levels,
            .array_size = texture.array_size,
            .format = texture.format,
            .data = texture.pixels.data(),
        };
        auto* t = context->CreateTexture(texture_create_info);
        auto* t_srv = context->CreateShaderResource(t);
        textures.emplace_back(t, t_srv);
    }

    for (auto& material : chess.materials)
    {
        if (material.albedo_index != -1)
        {
            material.albedo_index = textures[material.albedo_index].texture_srv->GetDescriptorIndex();
        }
        if (material.metal_rough_index != -1)
        {
            material.metal_rough_index = textures[material.metal_rough_index].texture_srv->GetDescriptorIndex();
        }
        if (material.occlusion_index != -1)
        {
            material.occlusion_index = textures[material.occlusion_index].texture_srv->GetDescriptorIndex();
        }
        if (material.emissive_index != -1)
        {
            material.emissive_index = textures[material.emissive_index].texture_srv->GetDescriptorIndex();
        }
        if (material.normal_index != -1)
        {
            material.normal_index = textures[material.normal_index].texture_srv->GetDescriptorIndex();
        }
    }

    const Swift::BufferCreateInfo material_create_info{
        .size = static_cast<uint32_t>(chess.materials.size() * sizeof(Material)),
        .data = chess.materials.data(),
    };
    auto* material_buffer = context->CreateBuffer(material_create_info);
    auto* material_buffer_srv =
        context->CreateShaderResource(material_buffer,
                                      Swift::BufferSRVCreateInfo{.num_elements = static_cast<uint32_t>(chess.materials.size()),
                                                                 .element_size = sizeof(Material)});

    const Swift::BufferCreateInfo transforms_create_info{
        .size = static_cast<uint32_t>(chess.transforms.size() * sizeof(glm::mat4)),
        .data = chess.transforms.data(),
    };
    auto* transforms_buffer = context->CreateBuffer(transforms_create_info);
    auto* transforms_buffer_srv =
        context->CreateShaderResource(transforms_buffer,
                                      Swift::BufferSRVCreateInfo{.num_elements = static_cast<uint32_t>(chess.transforms.size()),
                                                                 .element_size = sizeof(glm::mat4)});

    Camera camera{};
    Input input{window};

    window.AddResizeCallback(
        [&context, &depth_stencil, &depth_texture](const glm::uvec2 size)
        {
            context->GetGraphicsQueue()->WaitIdle();
            context->ResizeBuffers(size.x, size.y);
            context->DestroyTexture(depth_texture);
            context->DestroyDepthStencil(depth_stencil);
            const Swift::TextureCreateInfo depth_tex_info{
                .width = size.x,
                .height = size.y,
                .mip_levels = 1,
                .array_size = 1,
                .format = Swift::Format::eD32F,
                .flags = Swift::TextureFlags::eDepthStencil,
            };
            depth_texture = context->CreateTexture(depth_tex_info);
            depth_stencil = context->CreateDepthStencil(depth_texture);
        });

    auto prev_time = std::chrono::high_resolution_clock::now();
    while (window.IsRunning())
    {
        input.Tick();
        window.PollEvents();

        const auto& command = context->GetCurrentCommand();

        window_size = window.GetSize();
        const auto float_size = std::array{static_cast<float>(window_size[0]), static_cast<float>(window_size[1])};

        auto* render_target = context->GetCurrentRenderTarget();

        const auto current_time = std::chrono::high_resolution_clock::now();
        const auto delta_time = std::chrono::duration<float>(current_time - prev_time).count();
        prev_time = current_time;

        camera.Tick(window, input, delta_time);

        const ConstantBufferInfo scene_buffer_data{
            .view_proj = camera.m_proj_matrix * camera.m_view_matrix,
            .cam_pos = camera.m_position,
            .material_buffer_index = material_buffer_srv->GetDescriptorIndex(),
            .transform_buffer_index = transforms_buffer_srv->GetDescriptorIndex(),
        };
        constant_buffer->Write(&scene_buffer_data, 0, sizeof(ConstantBufferInfo));

        command->Begin();
        command->SetViewport(Swift::Viewport{.dimensions = float_size});
        command->SetScissor(Swift::Scissor{.dimensions = {window_size.x, window_size.y}});

        command->TransitionResource(render_target->GetTexture()->GetResource(), Swift::ResourceState::eRenderTarget);
        command->TransitionResource(depth_texture->GetResource(), Swift::ResourceState::eDepthWrite);

        command->ClearRenderTarget(render_target, {0.0f, 0.0f, 0.0f, 0.0f});
        command->ClearDepthStencil(depth_stencil, 1.f, 0.f);
        command->BindShader(shader);
        command->BindConstantBuffer(constant_buffer, 1);
        command->BindRenderTargets(render_target, depth_stencil);
        for (auto& mesh : mesh_renderers)
        {
            mesh.Draw(command);
        }

        command->TransitionResource(context->GetCurrentSwapchainTexture()->GetResource(), Swift::ResourceState::ePresent);

        command->End();

        context->Present(false);
    }

    context->DestroyBuffer(material_buffer);
    context->DestroyShaderResource(material_buffer_srv);
    context->DestroyBuffer(transforms_buffer);
    context->DestroyShaderResource(transforms_buffer_srv);
    context->DestroyBuffer(constant_buffer);

    for (auto& [texture, texture_srv] : textures)
    {
        context->DestroyTexture(texture);
        context->DestroyShaderResource(texture_srv);
    }

    context->DestroyShader(shader);
    Swift::DestroyContext(context);
}
