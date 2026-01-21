#pragma once
#include "swift_command.hpp"
#include "swift_macros.hpp"
#include "swift_queue.hpp"
#include "swift_texture.hpp"
#include "swift_shader.hpp"
#include "swift_structs.hpp"
#include "swift_descriptor.hpp"

namespace Swift
{
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

        virtual ICommand* CreateCommand(QueueType type, std::string_view debug_name = "") = 0;
        virtual IQueue* CreateQueue(const QueueCreateInfo& info) = 0;
        virtual IBuffer* CreateBuffer(const BufferCreateInfo& info) = 0;
        virtual ITexture* CreateTexture(const TextureCreateInfo& info) = 0;
        virtual IShader* CreateShader(const GraphicsShaderCreateInfo& info) = 0;
        virtual IShader* CreateShader(const ComputeShaderCreateInfo& info) = 0;
        virtual IRenderTarget* CreateRenderTarget(ITexture* texture, uint32_t mip = 0) = 0;
        virtual IDepthStencil* CreateDepthStencil(ITexture* texture, uint32_t mip = 0) = 0;
        virtual ITextureSRV* CreateShaderResource(ITexture* texture,
                                                  uint32_t most_detailed_mip = 0,
                                                  uint32_t mip_levels = 1) = 0;
        virtual IBufferSRV* CreateShaderResource(IBuffer* buffer, const BufferSRVCreateInfo& create_info) = 0;
        virtual ITextureUAV* CreateUnorderedAccessView(ITexture* texture, uint32_t mip = 0) = 0;
        virtual IBufferUAV* CreateUnorderedAccessView(IBuffer* buffer, const BufferUAVCreateInfo& create_info) = 0;
        virtual std::shared_ptr<IResource> CreateResource(const BufferCreateInfo& info) = 0;
        virtual std::shared_ptr<IResource> CreateResource(const TextureCreateInfo& info) = 0;

        virtual void DestroyCommand(ICommand* command) = 0;
        virtual void DestroyQueue(IQueue* queue) = 0;
        virtual void DestroyBuffer(IBuffer* buffer) = 0;
        virtual void DestroyTexture(ITexture* texture) = 0;
        virtual void DestroyShader(IShader* shader) = 0;
        virtual void DestroyRenderTarget(IRenderTarget* render_target) = 0;
        virtual void DestroyDepthStencil(IDepthStencil* depth_stencil) = 0;
        virtual void DestroyShaderResource(ITextureSRV* srv) = 0;
        virtual void DestroyShaderResource(IBufferSRV* srv) = 0;
        virtual void DestroyUnorderedAccessView(IBufferUAV* uav) = 0;
        virtual void DestroyUnorderedAccessView(ITextureUAV* uav) = 0;

        virtual void UpdateTextureRegion(ITexture* texture, const TextureUpdateRegion& texture_region) = 0;

        virtual void Present(bool vsync) = 0;
        virtual void ResizeBuffers(uint32_t width, uint32_t height) = 0;

        std::array<ITexture*, 3>& GetSwapchainTextures() { return m_swapchain_textures; }
        std::array<IRenderTarget*, 3>& GetSwapchainRenderTargets() { return m_swapchain_render_targets; }
        ITexture* GetCurrentSwapchainTexture() const { return m_swapchain_textures[m_frame_index]; }
        IRenderTarget* GetCurrentRenderTarget() const { return m_swapchain_render_targets[m_frame_index]; }
        ICommand* GetCurrentCommand() const { return m_frame_data[m_frame_index].command; }
        IQueue* GetGraphicsQueue() const { return m_graphics_queue; }
        IQueue* GetCopyQueue() const { return m_copy_queue; }

    protected:
        AdapterDescription m_adapter_description{};
        IQueue* m_graphics_queue = nullptr;
        IQueue* m_copy_queue = nullptr;

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
        std::array<IRenderTarget*, 3> m_swapchain_render_targets{};
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
        std::vector<IRenderTarget*> m_render_targets;
        std::vector<uint32_t> m_free_render_targets;
        std::vector<IDepthStencil*> m_depth_stencils;
        std::vector<uint32_t> m_free_depth_stencils;
        std::vector<ITextureSRV*> m_texture_srvs;
        std::vector<uint32_t> m_free_texture_srvs;
        std::vector<IBufferSRV*> m_buffer_srvs;
        std::vector<uint32_t> m_free_buffer_srvs;
        std::vector<ITextureUAV*> m_texture_uavs;
        std::vector<uint32_t> m_free_texture_uavs;
        std::vector<IBufferUAV*> m_buffer_uavs;
        std::vector<uint32_t> m_free_buffer_uavs;
    };
}  // namespace Swift
