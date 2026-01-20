#include "camera.hpp"
#include "importer.hpp"
#include "swift.hpp"
#include "window.hpp"
#include "shader_compiler.hpp"
#include "glm/gtc/type_ptr.hpp"
#include "glm/gtx/compatibility.hpp"
#include "chrono"
#include "imgui.h"
#include "imgui.hpp"
#include "mesh_renderer.hpp"
#include "swift_builders.hpp"
#include "lights.hpp"

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

    const auto constant_buffer = Swift::BufferBuilder(context, Swift::BufferType::eConstantBuffer, 65536).Build();

    const auto mesh_buffers = CreateMeshBuffers(context, helmet.meshes);
    std::vector<MeshRenderer> mesh_renderers = CreateMeshRenderers(helmet.nodes, helmet.meshes, mesh_buffers);
    const auto textures = CreateTextures(context, helmet.textures, helmet.materials);

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

    const auto point_light_buffer = Swift::BufferBuilder(context, Swift::BufferType::eStructuredBuffer)
                                        .SetElementSize(sizeof(PointLight))
                                        .SetNumElements(100)
                                        .Build();
    const auto dir_light_buffer = Swift::BufferBuilder(context, Swift::BufferType::eStructuredBuffer)
                                      .SetElementSize(sizeof(DirectionalLight))
                                      .SetNumElements(100)
                                      .Build();
    std::vector<PointLight> point_lights{};
    std::vector<DirectionalLight> dir_lights{};

    Camera camera{};
    Input input{window};
    ImguiBackend imgui{context, window};

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

        struct ConstantBufferInfo
        {
            glm::mat4 view_proj;
            glm::float3 cam_pos;
            uint32_t material_buffer_index;

            uint32_t transform_buffer_index;
            uint32_t point_light_buffer_index;
            uint32_t dir_light_buffer_index;
            uint32_t point_light_count;

            uint32_t dir_light_count;
            glm::float3 padding;
        };
        const ConstantBufferInfo scene_buffer_data{
            .view_proj = camera.m_proj_matrix * camera.m_view_matrix,
            .cam_pos = camera.m_position,
            .material_buffer_index = material_buffer->GetDescriptorIndex(),
            .transform_buffer_index = transforms_buffer->GetDescriptorIndex(),
            .point_light_buffer_index = point_light_buffer->GetDescriptorIndex(),
            .dir_light_buffer_index = dir_light_buffer->GetDescriptorIndex(),
            .point_light_count = static_cast<uint32_t>(point_lights.size()),
            .dir_light_count = static_cast<uint32_t>(dir_lights.size()),
        };
        constant_buffer->Write(&scene_buffer_data, 0, sizeof(ConstantBufferInfo));

        if (!point_lights.empty())
        {
            point_light_buffer->Write(point_lights.data(), 0, sizeof(PointLight) * static_cast<uint32_t>(point_lights.size()));
        }
        if (!dir_lights.empty())
        {
            dir_light_buffer->Write(dir_lights.data(), 0, sizeof(DirectionalLight) * static_cast<uint32_t>(dir_lights.size()));
        }

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

        imgui.BeginFrame();
        ImGui::Begin("Lights");
        if (ImGui::Button("Add Point Light"))
        {
            point_lights.emplace_back();
        }

        for (int i = 0; i < point_lights.size(); ++i)
        {
            auto& [position, intensity, color, range] = point_lights[i];
            ImGui::PushID(i);
            ImGui::DragFloat3("Position", glm::value_ptr(position));
            ImGui::DragFloat("Intensity", &intensity);
            ImGui::DragFloat3("Color", glm::value_ptr(color));
            ImGui::DragFloat("Range", &range);
            ImGui::PopID();
        }

        if (ImGui::Button("Add Directional Light"))
        {
            dir_lights.emplace_back();
        }

        for (int i = 0; i < dir_lights.size(); ++i)
        {
            auto& dir_light = dir_lights[i];
            ImGui::PushID(i);
            ImGui::DragFloat3("Direction", glm::value_ptr(dir_light.direction));
            ImGui::DragFloat("Intensity", &dir_light.intensity);
            ImGui::DragFloat3("Color", glm::value_ptr(dir_light.color));
            ImGui::PopID();
        }

        ImGui::End();
        imgui.Render(command);

        command->End();

        context->Present(false);
    }
}
