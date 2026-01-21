#include "chrono"
#include "camera.hpp"
#include "imgui.h"
#include "imgui.hpp"
#include "swift.hpp"
#include "window.hpp"
#include "shader_compiler.hpp"
#include "glm/gtx/compatibility.hpp"

struct GrassSettings
{
    uint32_t patch_x = 100;
    uint32_t patch_y = 100;
    float width = 0.07f;
    float height = 0.5f;
    float grid_spacing = 0.5f;
    float wind_speed = 0.5f;
    float wind_strength = 1.f;
    float lod_distance = 100.f;
    float radius = .5f;
    bool apply_view_space_thicken = true;
    bool cull = true;
    bool cull_z = false;
};

struct GrassPatch
{
    glm::vec3 position;
    float height;
};

void BuildGrass(Swift::IBuffer* grass_buffer, const GrassSettings& settings, uint32_t& grass_count);

int main()
{
    auto window = Window();
    auto window_size = window.GetSize();
    auto* context = Swift::CreateContext({.backend_type = Swift::BackendType::eD3D12,
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
    auto amplify_code = compiler.CompileShader("hello_grass.slang", ShaderStage::eAmplification);
    auto mesh_code = compiler.CompileShader("hello_grass.slang", ShaderStage::eMesh);
    auto pixel_code = compiler.CompileShader("hello_grass.slang", ShaderStage::ePixel);

    std::vector<Swift::Descriptor> descriptors{};
    Swift::Descriptor descriptor{
        .shader_register = 1,
        .register_space = 0,
        .descriptor_type = Swift::DescriptorType::eConstant,
        .shader_visibility = Swift::ShaderVisibility::eAll,
    };
    descriptors.emplace_back(descriptor);

    const auto formats = std::vector{Swift::Format::eRGBA8_UNORM};
    const auto grass_shader_create_info = Swift::GraphicsShaderCreateInfo{
        .rtv_formats = formats,
        .amplify_code = amplify_code,
        .mesh_code = mesh_code,
        .pixel_code = pixel_code,
        .depth_stencil_state = {.depth_enable = true,
                                .depth_write_enable = true,
                                .depth_test = Swift::DepthTest::eLess,
                                .stencil_enable = false},
        .rasterizer_state =
            {
                .cull_mode = Swift::CullMode::eNone,
            },
        .descriptors = descriptors,
    };
    auto* grass_shader = context->CreateShader(grass_shader_create_info);

    struct ConstantBufferInfo
    {
        glm::mat4 view_proj;

        glm::float3 cam_pos;
        uint32_t frustum_buffer_index;

        uint32_t grass_buffer_index;
        uint32_t grass_patch_count;
        glm::float2 padding;
    };
    constexpr Swift::BufferCreateInfo constant_create_info{
        .size = 65536,
    };
    auto* const constant_buffer = context->CreateBuffer(constant_create_info);

    struct Plane
    {
        glm::vec3 normal{};
        float distance{};
    };

    struct Frustum
    {
        Plane top_face;
        Plane bottom_face;

        Plane left_face;
        Plane right_face;

        Plane near_face;
        Plane far_face;
    };

    constexpr Swift::BufferCreateInfo frustum_create_info{
        .size = sizeof(Frustum),
    };
    auto* frustum_buffer = context->CreateBuffer(frustum_create_info);
    auto* frustum_buffer_srv = context->CreateShaderResource(frustum_buffer,
                                                             Swift::BufferSRVCreateInfo{
                                                                 .num_elements = 1,
                                                                 .element_size = sizeof(Frustum),
                                                             });

    constexpr Swift::BufferCreateInfo grass_info{
        .size = 1'000'000 * sizeof(GrassPatch),
    };
    auto* grass_buffer = context->CreateBuffer(grass_info);
    auto* grass_buffer_srv = context->CreateShaderResource(frustum_buffer,
                                                           Swift::BufferSRVCreateInfo{
                                                               .num_elements = 1'000'000,
                                                               .element_size = sizeof(GrassPatch),
                                                           });
    GrassSettings grass_settings{};
    uint32_t grass_count = grass_settings.patch_x * grass_settings.patch_y;
    BuildGrass(grass_buffer, grass_settings, grass_count);

    float near_plane = 0.01;
    float far_plane = 1000;

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

    float time = 0.0f;
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
        time += delta_time;

        camera.Tick(window, input, delta_time);

        const ConstantBufferInfo scene_buffer_data{
            .view_proj = camera.m_proj_matrix * camera.m_view_matrix,
            .cam_pos = camera.m_position,
            .frustum_buffer_index = frustum_buffer_srv->GetDescriptorIndex(),
            .grass_buffer_index = grass_buffer_srv->GetDescriptorIndex(),
            .grass_patch_count = grass_count,
        };
        constant_buffer->Write(&scene_buffer_data, 0, sizeof(ConstantBufferInfo));

        command->Begin();
        command->SetViewport(Swift::Viewport{.dimensions = float_size});
        command->SetScissor(Swift::Scissor{.dimensions = {window_size.x, window_size.y}});
        command->ClearRenderTarget(render_target, {0.0f, 0.0f, 0.0f, 0.0f});
        command->BindShader(grass_shader);
        command->BindConstantBuffer(constant_buffer, 1);
        command->BindRenderTargets(render_target, depth_stencil);
        const struct PushConstant
        {
            float wind_speed;
            float wind_strength;
            uint32_t apply_view_space_thicken;
            float lod_distance;

            uint32_t grass_count;
            float radius;
            uint32_t cull;
            uint32_t cull_z;

            float width;
            float time;
            glm::float2 padding;
        } push_constant{
            .wind_speed = grass_settings.wind_speed,
            .wind_strength = grass_settings.wind_strength,
            .apply_view_space_thicken = grass_settings.apply_view_space_thicken,
            .lod_distance = grass_settings.lod_distance,
            .grass_count = grass_count,
            .radius = grass_settings.radius,
            .cull = grass_settings.cull,
            .cull_z = grass_settings.cull_z,
            .width = grass_settings.width,
            .time = time,
        };
        command->PushConstants(&push_constant, sizeof(PushConstant));

        const uint32_t num_amp_groups = (grass_count + 127) / 128;
        command->DispatchMesh(num_amp_groups, 1, 1);

        imgui.BeginFrame();

        ImGui::Begin("Grass Patches");

        ImGui::DragScalar("Patch X", ImGuiDataType_U32, &grass_settings.patch_x, 1);
        bool changed = ImGui::IsItemDeactivatedAfterEdit();
        ImGui::DragScalar("Patch Y", ImGuiDataType_U32, &grass_settings.patch_y, 1);
        changed |= ImGui::IsItemDeactivatedAfterEdit();
        ImGui::DragFloat("Grid Spacing", &grass_settings.grid_spacing, 0, 1000);
        changed |= ImGui::IsItemDeactivatedAfterEdit();

        ImGui::Checkbox("Viewspace Thicken", &grass_settings.apply_view_space_thicken);

        ImGui::DragFloat("Wind Speed", &grass_settings.wind_speed);
        ImGui::DragFloat("Wind Strength", &grass_settings.wind_strength);
        ImGui::DragFloat("Grass Lod Distance", &grass_settings.lod_distance);
        ImGui::DragFloat("Grass Patch Radius", &grass_settings.radius);

        ImGui::DragFloat("Grass Width", &grass_settings.width);

        ImGui::Checkbox("Apply Culling", &grass_settings.cull);
        ImGui::Checkbox("Apply Culling Z", &grass_settings.cull_z);
        if (grass_settings.cull && grass_settings.cull_z)
        {
            ImGui::DragFloat("Near Plane", &near_plane);
            ImGui::DragFloat("Far Plane", &far_plane);
        }

        if (changed)
        {
            BuildGrass(grass_buffer, grass_settings, grass_count);
        }

        ImGui::End();

        imgui.Render(command);

        command->End();

        context->Present(false);
    }

    context->DestroyBuffer(constant_buffer);
    context->DestroyShader(grass_shader);
    context->DestroyBuffer(grass_buffer);
    context->DestroyBuffer(frustum_buffer);
    context->DestroyShaderResource(grass_buffer_srv);
    context->DestroyShaderResource(frustum_buffer_srv);

    imgui.Destroy();

    Swift::DestroyContext(context);
}

void BuildGrass(Swift::IBuffer* grass_buffer, const GrassSettings& settings, uint32_t& grass_count)
{
    grass_count = settings.patch_x * settings.patch_y;
    std::vector<GrassPatch> grass_patches;
    for (int i = 0; i < settings.patch_x; i++)
    {
        for (int j = 0; j < settings.patch_y; j++)
        {
            GrassPatch patch{
                .position = glm::vec3(i * settings.grid_spacing, 0, j * settings.grid_spacing),
                .height = settings.height,
            };
            grass_patches.push_back(patch);
        }
    }
    grass_buffer->Write(grass_patches.data(), 0, grass_patches.size() * sizeof(GrassPatch));
}
