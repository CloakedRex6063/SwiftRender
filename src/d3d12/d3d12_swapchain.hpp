#pragma once
#include "swift_context.hpp"
#include "swift_macros.hpp"
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
        Swapchain(IDXGIFactory7 *factory, ID3D12CommandQueue *queue, const ContextCreateInfo &create_info);

        void Present(bool m_vsync) const;
        void* GetSwapchain() const { return m_swapchain; };
        void Resize(const std::shared_ptr<IContext>& context, const std::array<float, 2>& size);
        uint32_t GetFrameIndex() const;

    private:
        IDXGISwapChain4* m_swapchain = nullptr;
    };
}
