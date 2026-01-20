#include "swift.hpp"
#include "window.hpp"
#include "shader_compiler.hpp"

int main()
{
    const auto window = Window();
    const auto window_size = window.GetSize();
    const auto context = Swift::CreateContext({.backend_type = Swift::BackendType::eD3D12,
                                               .width = window_size.x,
                                               .height = window_size.y,
                                               .native_window_handle = window.GetNativeWindow(),
                                               .native_display_handle = nullptr});

    const Swift::TextureCreateInfo render_tex_info{
        .width = window_size[0],
        .height = window_size[1],
        .mip_levels = 1,
        .array_size = 1,
        .format = Swift::Format::eRGBA8_UNORM,
        .flags = EnumFlags(Swift::TextureFlags::eRenderTarget) | EnumFlags(Swift::TextureFlags::eShaderResource),
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
    auto mesh_shader = compiler.CompileShader("hello_triangle.slang", ShaderStage::eMesh);
    auto pixel_shader = compiler.CompileShader("hello_triangle.slang", ShaderStage::ePixel);

    const auto triangle_create_info = Swift::GraphicsShaderCreateInfo{
        .rtv_formats = {Swift::Format::eRGBA8_UNORM},
        .mesh_code = mesh_shader,
        .pixel_code = pixel_shader,
        .rasterizer_state =
            {
                .cull_mode = Swift::CullMode::eNone,
            },
    };
    auto triangle_shader = context->CreateShader(triangle_create_info);

    while (window.IsRunning())
    {
        window.PollEvents();

        const auto& command = context->GetCurrentCommand();

        const auto window_size = window.GetSize();
        const auto float_size = std::array{static_cast<float>(window_size[0]), static_cast<float>(window_size[1])};

        auto& render_target = context->GetCurrentSwapchainTexture();

        command->Begin();
        command->SetViewport(Swift::Viewport{.dimensions = float_size});
        command->SetScissor(Swift::Scissor{.dimensions = {window_size.x, window_size.y}});
        command->ClearRenderTarget(render_target, {0.0f, 0.0f, 0.0f, 0.0f});
        command->BindShader(triangle_shader);
        command->BindRenderTargets(std::array{render_target}, {});
        command->DispatchMesh(1, 1, 1);
        command->End();

        context->Present(false);
    }
}
