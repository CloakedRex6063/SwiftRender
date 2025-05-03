#include "chrono"
#include "file_io.hpp"
#include "print"
#include "swift.hpp"
#include "utils.hpp"
#include "window.hpp"

int main()
{
    Swift::WindowUtil window;

    const Swift::ContextCreateInfo create_info = { .EngineName = "Swift",
                                                   .AppName = "Hello Triangle",
                                                   .Preference = Swift::DevicePreference::eHighPerformance,
                                                   .Width = 1280,
                                                   .Height = 720,
                                                   .Window = {
                                                       .DisplayHandle = window.GetDisplay(),
                                                       .WindowHandle = window.GetWindow(),
                                                   },
    };
    const auto exp_context = Swift::CreateContext(create_info);
    Swift::ThrowOnFail(exp_context, "Failed to create context");
    const auto exp_graphics_queue = Swift::GetQueue(Swift::QueueType::eGraphics);
    const auto graphics_queue = Swift::ThrowOnFail(exp_graphics_queue, "Failed to create graphics queue.");
    const auto exp_graphics_command = Swift::CreateCommand(graphics_queue);
    const auto graphics_command = Swift::ThrowOnFail(exp_graphics_command, "Failed to create graphics command.");
    const auto exp_swap_textures = Swift::CreateSwapchainTextures();
    const auto swap_textures = Swift::ThrowOnFail(exp_swap_textures, "Failed to get swap textures.");

    const auto exp_shader_layout = Swift::CreateShaderLayoutHandle({});
    const auto shader_layout = Swift::ThrowOnFail(exp_shader_layout, "Failed to create shader layout.");

    const auto vertex_code = Swift::ReadBinaryFile("hello_vertex_buffer.vert.spv");
    const auto fragment_code = Swift::ReadBinaryFile("hello_vertex_buffer.frag.spv");

    struct Vertex
    {
        struct Position
        {
            float X;
            float Y;
        } Pos;

        struct Color
        {
            float X;
            float Y;
            float Z;
        } Col;
    };

    Swift::GraphicsShaderCreateInfo shader_info = {
        .VertexCode = vertex_code,
        .VertexEntryPoint = "main",
        .FragmentCode = fragment_code,
        .FragmentEntryPoint = "main",
        .ColorFormats = std::array{ Swift::Format::eBGRA8Unorm },
        .DepthFormat = Swift::Format::eUnknown,
        .Topology = Swift::PrimitiveTopology::eTriangleList,
        .BlendStateDesc = Swift::Default::BlendState(Swift::Default::BlendAttachment(), 1),
        .VertexBindings = { Swift::Default::VertexBinding<Vertex>() },
        .VertexAttributes = {
            Swift::VertexAttribute{
                .Location = 0,
                .AttributeFormat = Swift::Format::eRG32Float,
                .Offset = 0,
            },
            Swift::VertexAttribute{
                .Location = 1,
                .AttributeFormat = Swift::Format::eRGB32Float,
                .Offset = 8,
            },
        },
    };
    const auto exp_shader = Swift::CreateShader(shader_info, shader_layout);
    const auto shader = Swift::ThrowOnFail(exp_shader, "Failed to create shader.");
    
    std::array vertices{
        Vertex{ Vertex::Position{ -0.5f, -0.5f }, Vertex::Color{ 1.0f, 0.0f, 0.0f } }, 
        Vertex{ Vertex::Position{  0.5f, -0.5f }, Vertex::Color{ 0.0f, 1.0f, 0.0f } },
        Vertex{ Vertex::Position{  0.5f,  0.5f }, Vertex::Color{ 0.0f, 0.0f, 1.0f } }, 
        Vertex{ Vertex::Position{ -0.5f,  0.5f }, Vertex::Color{ 1.0f, 1.0f, 0.0f } }, 
    };

    Swift::BufferCreateInfo buffer_info = {
        .Size = sizeof(Vertex) * 4,
        .Usage = Swift::BufferUsage::eVertexBuffer,
        .InitialData = vertices.data(),
    };
    const auto exp_vertex_buffer = Swift::CreateBuffer(buffer_info);
    const auto vertex_buffer = Swift::ThrowOnFail(exp_vertex_buffer, "Failed to create vertex buffer.");
    
    std::array indices{
        0u, 1u, 2u, 
        2u, 3u, 0u  
    };

    Swift::BufferCreateInfo index_buffer_info = {
        .Size = sizeof(uint32_t) * 6,
        .Usage = Swift::BufferUsage::eIndexBuffer,
        .InitialData = indices.data(),
    };
    const auto exp_index_buffer = Swift::CreateBuffer(index_buffer_info);
    const auto index_buffer = Swift::ThrowOnFail(exp_index_buffer, "Failed to create index buffer.");

    std::print("Initialised Swift \n");

    auto prev_time = std::chrono::high_resolution_clock::now();
    while (window.IsRunning())
    {
        const auto current_time = std::chrono::high_resolution_clock::now();
        const auto deltaTime = std::chrono::duration<float>(current_time - prev_time).count();
        prev_time = current_time;
        window.SetWindowTitle("FPS:" + std::to_string(1.f / deltaTime));
        window.PollEvents();

        const auto exp_index = Swift::AcquireNextImage();
        const auto index = Swift::ThrowOnFail(exp_index, "Failed to acquire image.");

        const auto begin_result = Swift::Command::Begin(graphics_command);
        Swift::ThrowOnFail(begin_result, "Failed to begin command.");

        Swift::RenderInfo render_info{
            .RenderTargets = std::array{ swap_textures[index] },
            .RenderArea = { {}, 1280, 720 },
            .ClearColor = { 0.392f, 0.584f, 0.929f, 1.0f },
            .ClearDepth = { 1.f },
            .ColorLoadOp = Swift::AttachmentLoadOp::eClear,
            .ColorStoreOp = Swift::AttachmentStoreOp::eStore,
        };

        Swift::Viewport viewport = { .Offset = { 0, 720 }, .Extent = { 1280, -720 } };
        Swift::Command::SetViewport(graphics_command, viewport);
        Swift::Rect scissor = { .Extent = { 1280, 720 } };
        Swift::Command::SetScissor(graphics_command, scissor);

        Swift::Command::BindVertexBuffer(graphics_command, vertex_buffer);
        Swift::Command::BindIndexBuffer(graphics_command, index_buffer);
        Swift::Command::BindShader(graphics_command, shader);

        Swift::Command::SetFrontFace(graphics_command, Swift::FrontFace::eClockwise);
        Swift::Command::SetCullMode(graphics_command, Swift::CullMode::eFront);
        Swift::Command::BeginRender(graphics_command, render_info);
        Swift::Command::DrawIndexed(graphics_command, 6, 1, 0, 0, 0);
        Swift::Command::EndRender(graphics_command);

        const auto end_result = Swift::Command::End(graphics_command);
        Swift::ThrowOnFail(end_result, "Failed to end command.");

        const auto result = Swift::Present(graphics_queue, graphics_command);
        Swift::ThrowOnFail(result, "Failed to present image.");
    }

    const auto wait_result = Swift::WaitIdle();
    Swift::ThrowOnFail(wait_result, "Failed to wait to idle.");

    Swift::DestroyBuffer(vertex_buffer);
    Swift::DestroyBuffer(index_buffer);
    Swift::DestroyShaderLayout(shader_layout);
    Swift::DestroyShader(shader);
    Swift::DestroyCommand(graphics_command);
    Swift::DestroySwapchainTextures(swap_textures);
    Swift::DestroyContext();
}