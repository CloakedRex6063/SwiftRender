#define GLM_FORCE_DEPTH_ZERO_TO_ONE
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

struct Frustum
{
    std::array<glm::vec4, 6> planes;
};

Frustum CreateFrustum(const Camera& camera, float near_plane, float far_plane);

int main()
{
    auto window = Window();
    auto window_size = window.GetSize();
    auto* const context = Swift::CreateContext({.backend_type = Swift::BackendType::eD3D12,
                                                .width = window_size.x,
                                                .height = window_size.y,
                                                .native_window_handle = window.GetNativeWindow(),
                                                .native_display_handle = nullptr});

    auto* depth_texture = Swift::TextureBuilder(context, window_size[0], window_size[1])
                              .SetFlags(EnumFlags(Swift::TextureFlags::eDepthStencil))
                              .SetFormat(Swift::Format::eD32F)
                              .Build();
    auto* depth_stencil = context->CreateDepthStencil(depth_texture);

    ShaderCompiler compiler{};
    auto ampl_shader = compiler.CompileShader("hello_cull.slang", ShaderStage::eAmplification);
    auto mesh_shader = compiler.CompileShader("hello_cull.slang", ShaderStage::eMesh);
    auto pixel_shader = compiler.CompileShader("hello_cull.slang", ShaderStage::ePixel);

    Importer importer{};
    auto helmet = importer.LoadModel("assets/cathedral.glb");

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

    auto* const shader = Swift::GraphicsShaderBuilder(context)
                             .SetRTVFormats({Swift::Format::eRGBA8_UNORM})
                             .SetDSVFormat(Swift::Format::eD32F)
                             .SetAmplificationShader(ampl_shader)
                             .SetMeshShader(mesh_shader)
                             .SetPixelShader(pixel_shader)
                             .SetDepthTestEnable(true)
                             .SetDepthWriteEnable(true)
                             .SetCullMode(Swift::CullMode::eBack)
                             .SetDepthTest(Swift::DepthTest::eLess)
                             .SetPolygonMode(Swift::PolygonMode::eTriangle)
                             .SetDescriptors(descriptors)
                             .SetStaticSamplers(sampler_descriptors)
                             .SetName("PBR Shader")
                             .Build();

    auto* const constant_buffer = Swift::BufferBuilder(context, 65536).Build();

    const auto mesh_buffers = CreateMeshBuffers(context, helmet.meshes);
    std::vector<MeshRenderer> mesh_renderers = CreateMeshRenderers(helmet.nodes, helmet.meshes, mesh_buffers);

    auto* texture_heap = context->CreateHeap(
        Swift::HeapCreateInfo{.type = Swift::HeapType::eGPU_Upload, .size = 1'000'000'000, .debug_name = "Texture Heap"});
    const auto textures = CreateTextures(context, texture_heap, helmet.textures, helmet.materials);

    auto* const material_buffer =
        Swift::BufferBuilder(context, sizeof(Material) * helmet.materials.size()).SetData(helmet.materials.data()).Build();
    auto* const material_buffer_srv =
        context->CreateShaderResource(material_buffer,
                                      Swift::BufferSRVCreateInfo{.num_elements = static_cast<uint32_t>(helmet.materials.size()),
                                                                 .element_size = sizeof(Material)});
    auto* const transforms_buffer =
        Swift::BufferBuilder(context, sizeof(glm::mat4) * helmet.transforms.size()).SetData(helmet.transforms.data()).Build();
    auto* const transforms_buffer_srv = context->CreateShaderResource(
        transforms_buffer,
        Swift::BufferSRVCreateInfo{.num_elements = static_cast<uint32_t>(helmet.transforms.size()),
                                   .element_size = sizeof(glm::mat4)});

    auto* const frustum_buffer = Swift::BufferBuilder(context, sizeof(Frustum)).Build();
    auto* frustum_buffer_srv = context->CreateShaderResource(frustum_buffer,
                                                             Swift::BufferSRVCreateInfo{
                                                                 .num_elements = 1,
                                                                 .element_size = sizeof(Frustum),
                                                             });
    
    auto* const cull_data_buffer = Swift::BufferBuilder(context, sizeof(CullData) * helmet.cull_datas.size())
                                      .SetData(helmet.cull_datas.data())
                                      .Build();
    auto* cull_data_buffer_srv = context->CreateShaderResource(cull_data_buffer,
                                                              Swift::BufferSRVCreateInfo{
                                                                  .num_elements = static_cast<uint32_t>(helmet.cull_datas.size()),
                                                                  .element_size = sizeof(CullData),
                                                              });

    auto* const point_light_buffer = Swift::BufferBuilder(context, sizeof(PointLight) * 100).Build();
    auto* const point_light_buffer_srv =
        context->CreateShaderResource(point_light_buffer,
                                      Swift::BufferSRVCreateInfo{.num_elements = 100, .element_size = sizeof(PointLight)});
    auto* const dir_light_buffer = Swift::BufferBuilder(context, sizeof(DirectionalLight) * 100).Build();
    auto* const dir_light_buffer_srv = context->CreateShaderResource(
        dir_light_buffer,
        Swift::BufferSRVCreateInfo{.num_elements = 100, .element_size = sizeof(DirectionalLight)});
    std::vector<PointLight> point_lights{};
    std::vector<DirectionalLight> dir_lights{};

    Camera camera{};
    Input input{window};
    ImguiBackend imgui{context, window};

    window.AddResizeCallback(
        [&context, &depth_stencil, &depth_texture](const glm::uvec2 size)
        {
            context->GetGraphicsQueue()->WaitIdle();
            context->ResizeBuffers(size.x, size.y);
            context->DestroyTexture(depth_texture);
            context->DestroyDepthStencil(depth_stencil);
            depth_texture = Swift::TextureBuilder(context, size.x, size.y)
                                .SetFlags(EnumFlags(Swift::TextureFlags::eDepthStencil))
                                .SetFormat(Swift::Format::eD32F)
                                .Build();
            depth_stencil = context->CreateDepthStencil(depth_texture);
        });

    auto prev_time = std::chrono::high_resolution_clock::now();

    float near_plane = 0.01;
    float far_plane = 1000;
    bool should_cull = false;

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
            uint32_t frustum_buffer_index;
            uint32_t bounding_buffer_index;
            float padding;
        };
        const ConstantBufferInfo scene_buffer_data{
            .view_proj = camera.m_proj_matrix * camera.m_view_matrix,
            .cam_pos = camera.m_position,
            .material_buffer_index = material_buffer_srv->GetDescriptorIndex(),
            .transform_buffer_index = transforms_buffer_srv->GetDescriptorIndex(),
            .point_light_buffer_index = point_light_buffer_srv->GetDescriptorIndex(),
            .dir_light_buffer_index = dir_light_buffer_srv->GetDescriptorIndex(),
            .point_light_count = static_cast<uint32_t>(point_lights.size()),
            .dir_light_count = static_cast<uint32_t>(dir_lights.size()),
            .frustum_buffer_index = frustum_buffer_srv->GetDescriptorIndex(),
            .bounding_buffer_index = cull_data_buffer_srv->GetDescriptorIndex(),
        };
        constant_buffer->Write(&scene_buffer_data, 0, sizeof(ConstantBufferInfo));

        Frustum frustum = CreateFrustum(camera, near_plane, far_plane);
        frustum_buffer->Write(&frustum, 0, sizeof(Frustum));

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

        command->TransitionResource(render_target->GetTexture()->GetResource(), Swift::ResourceState::eRenderTarget);
        command->TransitionResource(depth_texture->GetResource(), Swift::ResourceState::eDepthWrite);

        command->ClearRenderTarget(render_target, {0.0f, 0.0f, 0.0f, 0.0f});
        command->ClearDepthStencil(depth_stencil, 1.f, 0.f);
        command->BindShader(shader);
        command->BindConstantBuffer(constant_buffer, 1);
        command->BindRenderTargets(render_target, depth_stencil);

        for (auto& mesh : mesh_renderers)
        {
            mesh.Draw(command, should_cull);
        }

        imgui.BeginFrame();

        ImGui::Begin("Culling");
        ImGui::DragFloat("Near Plane", &near_plane);
        ImGui::DragFloat("Far Plane", &far_plane);
        ImGui::Checkbox("Should Cull", &should_cull);
        ImGui::Text("%f", 1.f / delta_time);
        ImGui::End();

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

        command->TransitionResource(context->GetCurrentSwapchainTexture()->GetResource(), Swift::ResourceState::ePresent);

        command->End();

        context->Present(false);
    }

    context->DestroyBuffer(point_light_buffer);
    context->DestroyShaderResource(point_light_buffer_srv);
    context->DestroyBuffer(dir_light_buffer);
    context->DestroyShaderResource(dir_light_buffer_srv);
    context->DestroyBuffer(material_buffer);
    context->DestroyShaderResource(material_buffer_srv);
    context->DestroyBuffer(transforms_buffer);
    context->DestroyShaderResource(transforms_buffer_srv);
    context->DestroyBuffer(constant_buffer);
    context->DestroyShader(shader);
    DestroyTextures(context, textures);
    DestroyMeshBuffers(context, mesh_buffers);
    context->DestroyHeap(texture_heap);

    imgui.Destroy();

    Swift::DestroyContext(context);
}

glm::vec4 NormalizePlane(glm::vec4 p)
{
    float len = glm::length(glm::vec3(p));
    return p / len;
}

Frustum CreateFrustum(const Camera& camera, float near_plane, float far_plane)
{
    auto perspective = glm::perspective(camera.m_fov, camera.m_aspect_ratio, near_plane, far_plane);
    auto vp = perspective * camera.m_view_matrix;
    vp = glm::transpose(vp);
    const Frustum frustum{.planes = {
                        NormalizePlane(vp[3] + vp[0]),  // Left
                        NormalizePlane(vp[3] - vp[0]),  // Right
                        NormalizePlane(vp[3] + vp[1]),  // Bottom
                        NormalizePlane(vp[3] - vp[1]),  // Top
                        NormalizePlane(vp[2]),          // Near  (0-1 depth range)
                        NormalizePlane(vp[3] - vp[2]),  // Far
                    }};

    return frustum;
}
