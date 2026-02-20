#include "swift.hpp"
#include "window.hpp"
#include "shader_compiler.hpp"

int main()
{
    auto window = Window();
    auto window_size = window.GetSize();
    auto* const context = Swift::CreateContext({.backend_type = Swift::BackendType::eD3D12,
                                                .width = window_size.x,
                                                .height = window_size.y,
                                                .native_window_handle = window.GetNativeWindow(),
                                                .native_display_handle = nullptr});

    ShaderCompiler compiler{};
    auto mesh_shader = compiler.CompileShader("hello_triangle.slang", ShaderStage::eMesh);
    auto pixel_shader = compiler.CompileShader("hello_triangle.slang", ShaderStage::ePixel);

    std::array formats{Swift::Format::eRGBA8_UNORM};
    const auto triangle_create_info = Swift::GraphicsShaderCreateInfo{
        .rtv_formats = formats,
        .mesh_code = mesh_shader,
        .pixel_code = pixel_shader,
    };
    auto* triangle_shader = context->CreateShader(triangle_create_info);

    window.AddResizeCallback(
        [context](const glm::uvec2 size)
    {
        context->GetGraphicsQueue()->WaitIdle();
        context->ResizeBuffers(size.x, size.y);
    });

    while (window.IsRunning())
    {
        window.PollEvents();

        const auto& command = context->GetCurrentCommand();

        window_size = window.GetSize();
        const auto float_size = std::array{static_cast<float>(window_size[0]), static_cast<float>(window_size[1])};

        auto* render_target = context->GetCurrentRenderTarget();

        command->Begin();
        command->SetViewport(Swift::Viewport{.dimensions = float_size});
        command->SetScissor(Swift::Scissor{.dimensions = {window_size.x, window_size.y}});
        command->TransitionImage(render_target->GetTexture(), Swift::ResourceState::eRenderTarget);
        command->ClearRenderTarget(render_target, {0.0f, 0.0f, 0.0f, 0.0f});
        command->BindShader(triangle_shader);
        command->BindRenderTargets(render_target, {});
        command->DispatchMesh(1, 1, 1);

        command->TransitionImage(render_target->GetTexture(), Swift::ResourceState::ePresent);

        command->End();

        context->Present(true);
    }

    context->DestroyShader(triangle_shader);
    Swift::DestroyContext(context);
}
