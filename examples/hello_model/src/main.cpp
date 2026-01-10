#include "camera.hpp"
#include "importer.hpp"
#include "swift.hpp"
#include "window.hpp"
#include "shader_compiler.hpp"
#include "glm/fwd.hpp"
#include "glm/gtc/type_ptr.hpp"
#include "glm/gtx/compatibility.hpp"
#include "chrono"

int main()
{
    auto window = Window();
    const auto window_size = window.GetSize();
    const auto context = Swift::CreateContext({
        .backend_type = Swift::BackendType::eD3D12,
        .size = std::array{window_size.x, window_size.y},
        .native_window_handle = window.GetNativeWindow(),
        .native_display_handle = nullptr
    });

    const Swift::TextureCreateInfo render_tex_info{
        .width = window_size[0],
        .height = window_size[1],
        .mip_levels = 1,
        .array_size = 1,
        .format = Swift::Format::eRGBA8_UNORM,
        .flags = EnumFlags(Swift::TextureFlags::eRenderTarget) |
                 EnumFlags(Swift::TextureFlags::eShaderResource),
    };
    const auto render_texture = context->CreateTexture(render_tex_info);
    const Swift::TextureCreateInfo depth_tex_info{
        .width = window_size[0],
        .height = window_size[1],
        .mip_levels = 1,
        .array_size = 1,
        .format = Swift::Format::eD32F,
        .flags = Swift::TextureFlags::eDepthStencil,
    };
    const auto depth_texture = context->CreateTexture(depth_tex_info);

    ShaderCompiler compiler{};

    auto mesh_shader = compiler.CompileShader("hello_model.slang", ShaderStage::eMesh);
    auto pixel_shader = compiler.CompileShader("hello_model.slang", ShaderStage::ePixel);

    Importer importer{};
    auto chess = importer.LoadModel("assets/chess.glb");

    std::vector<Swift::SamplerDescriptor> sampler_descriptors{};
    for (const auto &sampler: chess.samplers)
    {
        Swift::SamplerDescriptor sampler_descriptor{
            .min_filter = sampler.min_filter,
            .mag_filter = sampler.mag_filter,
            .wrap_u = sampler.wrap_u,
            .wrap_y = sampler.wrap_y,
            .wrap_w = sampler.wrap_w,
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

    const Swift::GraphicsShaderCreateInfo shader_create_info
    {
        .rtv_formats = {Swift::Format::eRGBA8_UNORM},
        .dsv_format = Swift::Format::eD32F,
        .mesh_code = mesh_shader,
        .pixel_code = pixel_shader,
        .cull_mode = Swift::CullMode::eBack,
        .descriptors = descriptors,
        .static_samplers = sampler_descriptors,
    };
    auto shader = context->CreateShader(shader_create_info);

    struct ConstantBufferInfo
    {
        glm::mat4 view_proj;
        glm::float3 cam_pos;
        uint32_t material_buffer_index;
        uint32_t transform_buffer_index;
        glm::float3 padding;
    };

    const Swift::BufferCreateInfo constant_create_info
    {
        .num_elements = 1,
        .element_size = 65536,
        .first_element = 0,
        .flags = Swift::BufferFlags::eConstantBuffer,
    };
    const auto constant_buffer = context->CreateBuffer(constant_create_info);

    struct MeshRenderer
    {
        uint32_t m_vertex_buffer;
        uint32_t m_mesh_buffer;
        uint32_t m_mesh_vertex_buffer;
        uint32_t m_mesh_triangle_buffer;
        uint32_t m_meshlet_count;
        int m_material_index;
        uint32_t m_transform_index;

        void Draw(const std::shared_ptr<Swift::ICommand> &command) const
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
        std::shared_ptr<Swift::IBuffer> m_vertex_buffer;
        std::shared_ptr<Swift::IBuffer> m_mesh_buffer;
        std::shared_ptr<Swift::IBuffer> m_mesh_vertex_buffer;
        std::shared_ptr<Swift::IBuffer> m_mesh_triangle_buffer;
    };
    std::vector<MeshBuffer> mesh_buffers;
    for (auto &mesh: chess.meshes)
    {
        const Swift::BufferCreateInfo vertex_create_info{
            .num_elements = static_cast<uint32_t>(mesh.vertices.size()),
            .element_size = sizeof(Vertex),
            .first_element = 0,
            .data = mesh.vertices.data(),
            .flags = Swift::BufferFlags::eStructuredBuffer,
        };
        const auto vertex_buffer = context->CreateBuffer(vertex_create_info);

        const Swift::BufferCreateInfo mesh_create_info{
            .num_elements = static_cast<uint32_t>(mesh.meshlets.size()),
            .element_size = sizeof(meshopt_Meshlet),
            .first_element = 0,
            .data = mesh.meshlets.data(),
            .flags = Swift::BufferFlags::eStructuredBuffer,
        };
        const auto meshlet_buffer = context->CreateBuffer(mesh_create_info);

        const Swift::BufferCreateInfo mesh_vertex_create_info{
            .num_elements = static_cast<uint32_t>(mesh.meshlet_vertices.size()),
            .element_size = sizeof(uint32_t),
            .first_element = 0,
            .data = mesh.meshlet_vertices.data(),
            .flags = Swift::BufferFlags::eStructuredBuffer,
        };
        const auto mesh_vertex_buffer = context->CreateBuffer(mesh_vertex_create_info);

        const Swift::BufferCreateInfo mesh_triangle_create_info{
            .num_elements = static_cast<uint32_t>(mesh.meshlet_triangles.size()),
            .element_size = sizeof(uint32_t),
            .first_element = 0,
            .data = mesh.meshlet_triangles.data(),
            .flags = Swift::BufferFlags::eStructuredBuffer,
        };
        const auto mesh_triangle_buffer = context->CreateBuffer(mesh_triangle_create_info);
        MeshBuffer mesh_buffer
        {
            .m_vertex_buffer = vertex_buffer,
            .m_mesh_buffer = meshlet_buffer,
            .m_mesh_vertex_buffer = mesh_vertex_buffer,
            .m_mesh_triangle_buffer = mesh_triangle_buffer,
        };
        mesh_buffers.emplace_back(mesh_buffer);
    }

    for (auto &node: chess.nodes)
    {
        const auto &mesh = chess.meshes[node.mesh_index];
        const auto &[m_vertex_buffer, m_mesh_buffer, m_mesh_vertex_buffer, m_mesh_triangle_buffer] = mesh_buffers[node.
            mesh_index];
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

    std::vector<std::shared_ptr<Swift::ITexture> > textures;
    for (auto &texture: chess.textures)
    {
        const Swift::TextureCreateInfo texture_create_info{
            .width = texture.width,
            .height = texture.height,
            .mip_levels = texture.mip_levels,
            .array_size = texture.array_size,
            .format = texture.format,
            .flags = Swift::TextureFlags::eShaderResource,
            .data = texture.pixels.data(),
            .msaa = std::nullopt,
        };
        const auto t = context->CreateTexture(texture_create_info);
        textures.emplace_back(t);
    }


    for (auto &material: chess.materials)
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

    const Swift::BufferCreateInfo material_create_info{
        .num_elements = static_cast<uint32_t>(chess.materials.size()),
        .element_size = sizeof(Material),
        .first_element = 0,
        .data = chess.materials.data(),
        .flags = Swift::BufferFlags::eStructuredBuffer,
    };
    const auto material_buffer = context->CreateBuffer(material_create_info);

    const Swift::BufferCreateInfo transforms_create_info{
        .num_elements = static_cast<uint32_t>(chess.transforms.size()),
        .element_size = sizeof(glm::mat4),
        .first_element = 0,
        .data = chess.transforms.data(),
        .flags = Swift::BufferFlags::eStructuredBuffer,
    };
    const auto transforms_buffer = context->CreateBuffer(transforms_create_info);

    Camera camera{};
    Input input{window};


    auto prev_time = std::chrono::high_resolution_clock::now();
    while (window.IsRunning())
    {
        input.Tick();
        window.PollEvents();

        const auto &command = context->GetCurrentCommand();

        const auto window_size = window.GetSize();
        const auto float_size = std::array{static_cast<float>(window_size[0]), static_cast<float>(window_size[1])};

        auto &render_target = context->GetCurrentSwapchainTexture();

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
        for (auto &mesh: mesh_renderers)
        {
            mesh.Draw(command);
        }
        command->End();

        context->Present(false);
    }
}
