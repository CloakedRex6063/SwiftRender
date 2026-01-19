#pragma once
#include "swift_context.hpp"
#include "swift_macros.hpp"
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include "directx/d3d12.h"
#include "directx/d3dx12.h"
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

        [[nodiscard]] void* GetDevice() const override;
        [[nodiscard]] void* GetAdapter() const override;
        [[nodiscard]] void* GetSwapchain() const override;
        [[nodiscard]] IDXGIFactory7* GetFactory() const { return m_factory; }
        [[nodiscard]] std::shared_ptr<DescriptorHeap> GetRTVHeap() { return m_rtv_heap; }
        [[nodiscard]] std::shared_ptr<DescriptorHeap> GetDSVHeap() { return m_dsv_heap; }
        [[nodiscard]] std::shared_ptr<DescriptorHeap> GetCBVSRVUAVHeap() { return m_cbv_srv_uav_heap; }

        std::shared_ptr<ICommand> CreateCommand(QueueType queue_type) override;
        std::shared_ptr<IQueue> CreateQueue(const QueueCreateInfo& info) override;
        std::shared_ptr<IBuffer> CreateBuffer(const BufferCreateInfo& info) override;
        std::shared_ptr<ITexture> CreateTexture(const TextureCreateInfo& info) override;
        std::shared_ptr<IResource> CreateResource(const BufferCreateInfo& info) override;
        std::shared_ptr<IResource> CreateResource(const TextureCreateInfo& info) override;
        std::shared_ptr<IShader> CreateShader(const GraphicsShaderCreateInfo& info) override;
        std::shared_ptr<IShader> CreateShader(const ComputeShaderCreateInfo& info) override;

        void Present(bool vsync) override;

    private:
        void CreateBackend();
        void CreateDevice();
        void CreateDescriptorHeaps();
        void CreateFrameData();
        void CreateQueues();
        void CreateTextures(const ContextCreateInfo& create_info);
        void CreateSwapchain(const ContextCreateInfo& create_info);

        IDXGIAdapter4* m_adapter = nullptr;
        IDXGIFactory7* m_factory = nullptr;
        ID3D12Device14* m_device = nullptr;
        std::shared_ptr<Swapchain> m_swapchain = nullptr;
        std::shared_ptr<DescriptorHeap> m_rtv_heap{};
        std::shared_ptr<DescriptorHeap> m_dsv_heap{};
        std::shared_ptr<DescriptorHeap> m_cbv_srv_uav_heap{};
        DXGI_ADAPTER_DESC3 m_adapter_desc{};
    };
}  // namespace Swift::D3D12
