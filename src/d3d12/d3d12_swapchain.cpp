#include "d3d12/d3d12_swapchain.hpp"
#include "d3d12/d3d12_helpers.hpp"
#include "d3d12/d3d12_context.hpp"
#include "d3d12/d3d12_resource.hpp"

Swift::D3D12::Swapchain::Swapchain(IDXGIFactory7* factory, ID3D12CommandQueue* queue, const ContextCreateInfo &create_info)
{
    const auto hwnd = static_cast<HWND>(create_info.native_window_handle);
    constexpr auto format = Format::eRGBA8_UNORM;
    const DXGI_SWAP_CHAIN_DESC1 swapchain_desc = {
        .Width = create_info.size[0],
        .Height = create_info.size[1],
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
    IDXGISwapChain1 *swapchain;

    [[maybe_unused]]
            auto result = factory->CreateSwapChainForHwnd(
                queue, hwnd, &swapchain_desc, nullptr, nullptr,
                &swapchain);
    result = swapchain->QueryInterface(IID_PPV_ARGS(&m_swapchain));
}

void Swift::D3D12::Swapchain::Present(bool m_vsync) const
{
    [[maybe_unused]]
            const auto result = m_swapchain->Present(m_vsync, 0);
}

void Swift::D3D12::Swapchain::Resize(const std::shared_ptr<IContext> &context, const std::array<float, 2> &size)
{
    for (auto texture: context->GetSwapchainTextures())
    {
        texture.reset();
    }

    [[maybe_unused]]
            auto result = m_swapchain->ResizeBuffers(3, (uint32_t) size[0], (uint32_t) size[1],
                                                     DXGI_FORMAT_R8G8B8A8_UNORM, DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING);

    constexpr auto format = Format::eRGBA8_UNORM;
    for (int i = 0; i < context->GetSwapchainTextures().size(); i++)
    {
        ID3D12Resource *back_buffer;
        result = m_swapchain->GetBuffer(i, IID_PPV_ARGS(&back_buffer));

        const auto resource = std::make_shared<Resource>(back_buffer);
        TextureCreateInfo create_info
        {
            .width = (uint32_t) size[0],
            .height = (uint32_t) size[1],
            .mip_levels = 1,
            .array_size = 1,
            .format = format,
            .flags = TextureFlags::eRenderTarget,
            .msaa = std::nullopt,
            .resource = resource,
        };

        context->GetSwapchainTextures()[i] = context->CreateTexture(create_info);
    }
}

uint32_t Swift::D3D12::Swapchain::GetFrameIndex() const
{
    return m_swapchain->GetCurrentBackBufferIndex();
}
