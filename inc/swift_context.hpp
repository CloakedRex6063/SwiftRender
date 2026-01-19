#pragma once
#include "memory"
#include "swift_command.hpp"
#include "swift_macros.hpp"
#include "swift_queue.hpp"
#include "swift_buffer.hpp"
#include "swift_texture.hpp"
#include "swift_resource.hpp"
#include "swift_shader.hpp"
#include "swift_structs.hpp"

namespace Swift
{
    class IContext : public std::enable_shared_from_this<IContext>
    {
    public:
        SWIFT_DESTRUCT(IContext);
        SWIFT_NO_COPY(IContext);
        SWIFT_NO_MOVE(IContext);

        explicit IContext(const ContextCreateInfo&) {}

        [[nodiscard]] virtual void* GetDevice() const = 0;
        [[nodiscard]] virtual void* GetAdapter() const = 0;
        [[nodiscard]] virtual void* GetSwapchain() const = 0;

        virtual std::shared_ptr<ICommand> CreateCommand(QueueType type) = 0;
        virtual std::shared_ptr<IQueue> CreateQueue(const QueueCreateInfo& info) = 0;
        virtual std::shared_ptr<IBuffer> CreateBuffer(const BufferCreateInfo& info) = 0;
        virtual std::shared_ptr<ITexture> CreateTexture(const TextureCreateInfo& info) = 0;
        virtual std::shared_ptr<IResource> CreateResource(const BufferCreateInfo& info) = 0;
        virtual std::shared_ptr<IResource> CreateResource(const TextureCreateInfo& info) = 0;
        virtual std::shared_ptr<IShader> CreateShader(const GraphicsShaderCreateInfo& info) = 0;
        virtual std::shared_ptr<IShader> CreateShader(const ComputeShaderCreateInfo& info) = 0;

        virtual void Present(bool vsync) = 0;
        std::array<std::shared_ptr<ITexture>, 3>& GetSwapchainTextures() { return m_swapchain_textures; }
        std::shared_ptr<ITexture>& GetCurrentSwapchainTexture() { return m_swapchain_textures[m_frame_index]; }
        std::shared_ptr<ICommand>& GetCurrentCommand() { return m_frame_data[m_frame_index].command; }
        std::shared_ptr<IQueue>& GetGraphicsQueue() { return m_graphics_queue; }
        std::shared_ptr<IQueue>& GetCopyQueue() { return m_copy_queue; }

    protected:
        AdapterDescription m_adapter_description{};
        std::shared_ptr<IQueue> m_graphics_queue = nullptr;
        std::shared_ptr<IQueue> m_copy_queue = nullptr;

        auto& GetFrameData() { return m_frame_data[m_frame_index]; }
        auto& GetFrameData() const { return m_frame_data; }

        struct FrameData
        {
            std::shared_ptr<ICommand> command;
            uint64_t fence_value;
        };
        std::array<FrameData, 3> m_frame_data{};
        uint32_t m_frame_index{};
        std::array<std::shared_ptr<ITexture>, 3> m_swapchain_textures;
    };
}  // namespace Swift
