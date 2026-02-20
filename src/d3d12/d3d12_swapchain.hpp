#pragma once
#include "cstdint"
#include "swift_macros.hpp"
#include "swift_structs.hpp"
#define NOMINMAX
#include "directx/d3d12.h"
#include "dxgi1_6.h"

namespace Swift::D3D12
{
    class Swapchain
    {
    public:
        SWIFT_NO_CONSTRUCT(Swapchain);
        SWIFT_NO_COPY(Swapchain);
        SWIFT_NO_MOVE(Swapchain);
        Swapchain(IDXGIFactory7* factory, ID3D12CommandQueue* queue, const ContextCreateInfo& create_info);
        ~Swapchain();

        void Present(bool m_vsync) const;
        IDXGISwapChain4* GetSwapchain() const { return m_swapchain; };
        void Resize(uint32_t width, uint32_t height) const;
        uint32_t GetFrameIndex() const;

    private:
        IDXGISwapChain4* m_swapchain = nullptr;
    };
}  // namespace Swift::D3D12
