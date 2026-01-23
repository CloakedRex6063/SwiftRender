#include "d3d12/d3d12_swapchain.hpp"
#include "d3d12/d3d12_helpers.hpp"
#include "d3d12/d3d12_context.hpp"

Swift::D3D12::Swapchain::Swapchain(IDXGIFactory7* factory, ID3D12CommandQueue* queue, const ContextCreateInfo& create_info)
{
    auto *const hwnd = static_cast<HWND>(create_info.native_window_handle);
    constexpr auto format = Format::eRGBA8_UNORM;
    const DXGI_SWAP_CHAIN_DESC1 swapchain_desc = {
        .Width = create_info.width,
        .Height = create_info.height,
        .Format = ToDXGIFormat(format),
        .Stereo = false,
        .SampleDesc = DXGI_SAMPLE_DESC{1, 0},
        .BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT,
        .BufferCount = 3,
        .Scaling = DXGI_SCALING_STRETCH,
        .SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD,
        .AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED,
        .Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING,
    };
    IDXGISwapChain1* swapchain = nullptr;

    factory->CreateSwapChainForHwnd(queue, hwnd, &swapchain_desc, nullptr, nullptr, &swapchain);
    swapchain->QueryInterface(IID_PPV_ARGS(&m_swapchain));
}

Swift::D3D12::Swapchain::~Swapchain()
{
    m_swapchain->SetFullscreenState(false, nullptr);
    m_swapchain->Release();
}

void Swift::D3D12::Swapchain::Present(const bool m_vsync) const
{
    [[maybe_unused]]
    const auto result = m_swapchain->Present(m_vsync, 0);
}

void Swift::D3D12::Swapchain::Resize(const uint32_t width, const uint32_t height) const
{
    m_swapchain->ResizeBuffers(3,
                               width,
                               height,
                               DXGI_FORMAT_R8G8B8A8_UNORM,
                               DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING);
}

uint32_t Swift::D3D12::Swapchain::GetFrameIndex() const { return m_swapchain->GetCurrentBackBufferIndex(); }
