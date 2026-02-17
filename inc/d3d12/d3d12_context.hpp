#pragma once
#include "swift_context.hpp"
#include "swift_macros.hpp"
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include "directx/d3d12.h"
#include "d3d12_queue.hpp"
#include "dxgi1_6.h"
#include "d3d12_descriptor.hpp"

namespace Swift::D3D12
{
    class Swapchain;
    class Context final : public IContext
    {
    public:
        explicit Context(const ContextCreateInfo& create_info);
        ~Context() override;
        SWIFT_NO_COPY(Context);
        SWIFT_NO_MOVE(Context);

        [[nodiscard]] void* GetDevice() const override;
        [[nodiscard]] void* GetAdapter() const override;
        [[nodiscard]] void* GetSwapchain() const override;
        [[nodiscard]] IDXGIFactory7* GetFactory() const { return m_factory; }
        [[nodiscard]] DescriptorHeap* GetRTVHeap() const { return m_rtv_heap; }
        [[nodiscard]] DescriptorHeap* GetDSVHeap() const { return m_dsv_heap; }
        [[nodiscard]] DescriptorHeap* GetCBVSRVUAVHeap() const { return m_cbv_srv_uav_heap; }

        ICommand* CreateCommand(QueueType queue_type, std::string_view debug_name = "") override;
        IQueue* CreateQueue(const QueueCreateInfo& info) override;
        IBuffer* CreateBuffer(const BufferCreateInfo& info) override;
        ITexture* CreateTexture(const TextureCreateInfo& info) override;
        IRenderTarget* CreateRenderTarget(ITexture* texture, uint32_t mip = 0) override;
        IDepthStencil* CreateDepthStencil(ITexture* texture, uint32_t mip = 0) override;
        ITextureSRV* CreateShaderResource(ITexture* texture, uint32_t mip_levels = 0, uint32_t most_detailed_mip = 0) override;
        IBufferSRV* CreateShaderResource(IBuffer* buffer, const BufferSRVCreateInfo& srv_create_info) override;
        ITextureUAV* CreateUnorderedAccessView(ITexture* texture, uint32_t mip = 0) override;
        IBufferUAV* CreateUnorderedAccessView(IBuffer* buffer, const BufferUAVCreateInfo& uav_create_info) override;
        IShader* CreateShader(const GraphicsShaderCreateInfo& info) override;
        IShader* CreateShader(const ComputeShaderCreateInfo& info) override;
        std::shared_ptr<IResource> CreateResource(const BufferCreateInfo& info) override;
        std::shared_ptr<IResource> CreateResource(const TextureCreateInfo& info) override;
        IHeap* CreateHeap(const HeapCreateInfo& heap_create_info) override;

        void DestroyCommand(ICommand* command) override;
        void DestroyQueue(IQueue* queue) override;
        void DestroyBuffer(IBuffer* buffer) override;
        void DestroyTexture(ITexture* texture) override;
        void DestroyShader(IShader* shader) override;
        void DestroyRenderTarget(IRenderTarget* render_target) override;
        void DestroyDepthStencil(IDepthStencil* depth_stencil) override;
        void DestroyShaderResource(ITextureSRV* srv) override;
        void DestroyShaderResource(IBufferSRV* srv) override;
        void DestroyUnorderedAccessView(IBufferUAV* uav) override;
        void DestroyUnorderedAccessView(ITextureUAV* uav) override;
        void DestroyHeap(IHeap* heap) override;

        void UpdateTextureRegion(ITexture* texture, const TextureUpdateRegion& texture_region) override;

        void Present(bool vsync) override;
        void ResizeBuffers(uint32_t width, uint32_t height) override;
        uint32_t CalculateAlignedTextureSize(const TextureCreateInfo& info) override;
        uint32_t CalculateAlignedBufferSize(const BufferCreateInfo& info) override;

    private:
        void CreateBackend();
        void CreateDevice();
        void CreateDescriptorHeaps(const ContextCreateInfo& create_info);
        void CreateFrameData();
        void CreateQueues();
        void CreateTextures(const ContextCreateInfo& create_info);
        void CreateSwapchain(const ContextCreateInfo& create_info);
        void CreateMipMapShader();

        IDXGIAdapter4* m_adapter = nullptr;
        IDXGIFactory7* m_factory = nullptr;
        ID3D12Device14* m_device = nullptr;
        ID3D12Debug6* m_debug_controller = nullptr;
        Swapchain* m_swapchain = nullptr;
        DescriptorHeap* m_rtv_heap{};
        DescriptorHeap* m_dsv_heap{};
        DescriptorHeap* m_cbv_srv_uav_heap{};
        IShader* m_mipmap_shader{};
        DXGI_ADAPTER_DESC3 m_adapter_desc{};
    };
}  // namespace Swift::D3D12
