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

        context->NewFrame();

        const auto& command = context->GetCurrentCommand();

        window_size = window.GetSize();

        auto* render_target = context->GetCurrentRenderTarget();

        command->Begin();
        command->SetViewport(Swift::Viewport{.dimensions = Swift::Float2(window_size.x, window_size.y)});
        command->SetScissor(Swift::Scissor{.dimensions = {window_size.x, window_size.y}});
        command->TransitionImage(render_target->GetTexture(), Swift::ResourceState::eRenderTarget);
        command->BindShader(triangle_shader);
        Swift::ColorAttachmentInfo color_attachment
        {
            .render_target = render_target,
            .load_op = Swift::LoadOp::eClear,
            .store_op = Swift::StoreOp::eStore,
            .clear_color = {},
        };
        command->BeginRender(color_attachment, std::nullopt);
        command->DispatchMesh(1, 1, 1);
        command->EndRender();

        command->TransitionImage(render_target->GetTexture(), Swift::ResourceState::ePresent);

        command->End();

        context->Present(true);
    }

    context->GetGraphicsQueue()->WaitIdle();
    context->DestroyShader(triangle_shader);
    Swift::DestroyContext(context);
}
