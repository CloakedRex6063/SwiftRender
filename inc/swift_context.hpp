#pragma once
#include "swift_command.hpp"
#include "swift_macros.hpp"
#include "swift_queue.hpp"
#include "swift_texture.hpp"
#include "swift_shader.hpp"
#include "swift_structs.hpp"
#include "array"
#include "swift_texture_view.hpp"
#include "swift_buffer_view.hpp"
#include "swift_sampler.hpp"
#include "swift_buffer.hpp"
#include "vector"

namespace Swift
{
    class ICommandSignature
    {
    public:
        SWIFT_DESTRUCT(ICommandSignature);
        SWIFT_NO_COPY(ICommandSignature);
        SWIFT_NO_MOVE(ICommandSignature);

        explicit ICommandSignature(std::span<IndirectArgument> indirect_arguments) {};
        [[nodiscard]] virtual void* GetSignature() = 0;
    };
    class IContext
    {
    public:
        SWIFT_DESTRUCT(IContext);
        SWIFT_NO_COPY(IContext);
        SWIFT_NO_MOVE(IContext);

        explicit IContext(const ContextCreateInfo&) {}

        [[nodiscard]] virtual void* GetDevice() const = 0;
        [[nodiscard]] virtual void* GetAdapter() const = 0;
        [[nodiscard]] virtual void* GetSwapchain() const = 0;

        [[nodiscard]] virtual ICommand* CreateCommand(IQueue* queue, std::string_view debug_name = "") = 0;
        [[nodiscard]] virtual IQueue* CreateQueue(const QueueCreateInfo& info) = 0;
        [[nodiscard]] virtual IBuffer* CreateBuffer(const BufferCreateInfo& info) = 0;
        [[nodiscard]] virtual ITexture* CreateTexture(const TextureCreateInfo& info) = 0;
        [[nodiscard]] virtual IShader* CreateShader(const GraphicsShaderCreateInfo& info) = 0;
        [[nodiscard]] virtual IShader* CreateShader(const ComputeShaderCreateInfo& info) = 0;
        [[nodiscard]] virtual ITextureView* CreateTextureView(ITexture* texture, const TextureViewCreateInfo& info) = 0;
        [[nodiscard]] virtual IBufferView* CreateBufferView(IBuffer* buffer, const BufferViewCreateInfo& info) = 0;
        [[nodiscard]] virtual ISampler* CreateSampler(const SamplerCreateInfo& info) = 0;
        [[nodiscard]] virtual ICommandSignature* CreateCommandSignature(std::span<IndirectArgument> indirect_arguments) = 0;

        virtual void DestroyCommand(ICommand* command) = 0;
        virtual void DestroyQueue(IQueue* queue) = 0;
        virtual void DestroyBuffer(IBuffer* buffer) = 0;
        virtual void DestroyTexture(ITexture* texture) = 0;
        virtual void DestroyShader(IShader* shader) = 0;
        virtual void DestroyTextureView(ITextureView* texture_view) = 0;
        virtual void DestroyBufferView(IBufferView* buffer_view) = 0;
        virtual void DestroySampler(ISampler* sampler) = 0;
        virtual void DestroyCommandSignature(ICommandSignature* signature) = 0;

        virtual void NewFrame() = 0;
        virtual void Present(bool vsync) = 0;
        virtual void ResizeBuffers(uint32_t width, uint32_t height) = 0;
        virtual uint32_t CalculateAlignedTextureSize(const TextureCreateInfo& info) = 0;
        virtual uint32_t CalculateAlignedBufferSize(const BufferCreateInfo& info) = 0;

        std::array<ITexture*, 3>& GetSwapchainTextures() { return m_swapchain_textures; }
        std::array<ITextureView*, 3>& GetSwapchainRenderTargets() { return m_swapchain_render_targets; }
        virtual ITexture* GetCurrentSwapchainTexture() const = 0;
        virtual ITextureView* GetCurrentRenderTarget() const = 0;
        ICommand* GetCurrentCommand() const { return m_frame_data[m_frame_index].command; }
        uint32_t GetFrameIndex() const { return m_frame_index; }
        IQueue* GetGraphicsQueue() const { return m_graphics_queue; }

    protected:
        AdapterDescription m_adapter_description{};
        IQueue* m_graphics_queue = nullptr;

        auto& GetFrameData() { return m_frame_data[m_frame_index]; }
        auto& GetFrameData() const { return m_frame_data; }

        struct FrameData
        {
            ICommand* command;
            uint64_t fence_value;
        };
        std::array<FrameData, 3> m_frame_data{};
        uint32_t m_frame_index{};
        std::array<ITexture*, 3> m_swapchain_textures{};
        std::array<ITextureView*, 3> m_swapchain_render_targets{};
        std::vector<ICommand*> m_commands;
        std::vector<uint32_t> m_free_commands;
        std::vector<IQueue*> m_queues;
        std::vector<uint32_t> m_free_queues;
        std::vector<IBuffer*> m_buffers;
        std::vector<uint32_t> m_free_buffers;
        std::vector<ITexture*> m_textures;
        std::vector<uint32_t> m_free_textures;
        std::vector<IShader*> m_shaders;
        std::vector<uint32_t> m_free_shaders;
        std::vector<ITextureView*> m_texture_views;
        std::vector<uint32_t> m_free_texture_views;
        std::vector<IBufferView*> m_buffer_views;
        std::vector<uint32_t> m_free_buffer_views;
        std::vector<ICommandSignature*> m_command_sigs;
        std::vector<uint32_t> m_free_command_sigs;
        std::vector<ISampler*> m_samplers;
        std::vector<uint32_t> m_free_samplers;
    };
}  // namespace Swift
