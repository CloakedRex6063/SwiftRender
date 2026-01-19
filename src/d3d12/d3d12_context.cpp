#pragma once

#include "d3d12/d3d12_context.hpp"
#include "d3d12/d3d12_buffer.hpp"
#include "d3d12/d3d12_command.hpp"
#include "d3d12/d3d12_queue.hpp"
#include "d3d12/d3d12_shader.hpp"
#include "d3d12/d3d12_texture.hpp"
#include "d3d12/d3d12_resource.hpp"
#include "d3d12/d3d12_swapchain.hpp"
#include "swift_helpers.hpp"
#include "format"

namespace Swift::D3D12
{
    Context::Context(const ContextCreateInfo& create_info) : IContext(create_info)
    {
        CreateBackend();
        CreateDevice();
        CreateDescriptorHeaps();
        CreateFrameData();
        CreateQueues();
        CreateSwapchain(create_info);
        CreateTextures(create_info);
    }

    Context::~Context()
    {
        m_factory->Release();
        m_device->Release();
        m_adapter->Release();
    }

    void* Context::GetDevice() const { return m_device; }

    void* Context::GetAdapter() const { return m_adapter; }

    void* Context::GetSwapchain() const { return m_swapchain->GetSwapchain(); }

    std::shared_ptr<ICommand> Context::CreateCommand(QueueType queue_type)
    {
        return std::make_shared<Command>(this, m_cbv_srv_uav_heap, queue_type);
    }

    std::shared_ptr<IQueue> Context::CreateQueue(const QueueCreateInfo& info)
    {
        return std::make_shared<Queue>(m_device, info);
    }

    std::shared_ptr<IBuffer> Context::CreateBuffer(const BufferCreateInfo& info)
    {
        return std::make_shared<Buffer>(std::dynamic_pointer_cast<Context>(shared_from_this()), m_cbv_srv_uav_heap, info);
    }

    std::shared_ptr<ITexture> Context::CreateTexture(const TextureCreateInfo& info)
    {
        const auto texture = std::make_shared<Texture>(this, m_rtv_heap, m_dsv_heap, m_cbv_srv_uav_heap, info);

        if (info.data)
        {
            const uint32_t size = GetTextureSize(info);
            const BufferCreateInfo buffer_info = {
                .num_elements = 1,
                .element_size = size,
                .first_element = 0,
                .data = info.data,
                .type = BufferType::eNone,
            };
            const auto upload_buffer = CreateBuffer(buffer_info);
            const auto copy_command = CreateCommand(QueueType::eTransfer);
            copy_command->Begin();
            copy_command->CopyBufferToTexture(shared_from_this(), {upload_buffer, texture});
            copy_command->End();

            const auto value = m_copy_queue->Execute(std::array{copy_command});
            m_copy_queue->Wait(value);
        }

        return texture;
    }

    std::shared_ptr<IResource> Context::CreateResource(const BufferCreateInfo& info)
    {
        return std::make_shared<Resource>(m_device, info);
    }

    std::shared_ptr<IResource> Context::CreateResource(const TextureCreateInfo& info)
    {
        return std::make_shared<Resource>(m_device, info);
    }

    std::shared_ptr<IShader> Context::CreateShader(const GraphicsShaderCreateInfo& info)
    {
        return std::make_shared<Shader>(m_device, info);
    }

    std::shared_ptr<IShader> Context::CreateShader(const ComputeShaderCreateInfo& info)
    {
        return std::make_shared<Shader>(m_device, info);
    }

    void Context::Present(const bool vsync)
    {
        GetFrameData().fence_value = m_graphics_queue->Execute(std::array{GetFrameData().command});
        m_swapchain->Present(vsync);
        m_graphics_queue->Wait(GetFrameData().fence_value);
        m_frame_index = m_swapchain->GetFrameIndex();
    }

    void Context::CreateBackend()
    {
        [[maybe_unused]]
        auto result = CreateDXGIFactory2(0, IID_PPV_ARGS(&m_factory));
        result = m_factory->EnumAdapterByGpuPreference(0, DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE, IID_PPV_ARGS(&m_adapter));

#ifdef SWIFT_DEBUG
        ID3D12Debug6* debug_controller;
        [[maybe_unused]]
        const auto debug_result = D3D12GetDebugInterface(IID_PPV_ARGS(&debug_controller));
        debug_controller->EnableDebugLayer();
#endif

        result = m_adapter->GetDesc3(&m_adapter_desc);

        const int size = WideCharToMultiByte(CP_UTF8, 0, m_adapter_desc.Description, -1, nullptr, 0, nullptr, nullptr);
        m_adapter_description.name.resize(size - 1);
        WideCharToMultiByte(CP_UTF8,
                            0,
                            m_adapter_desc.Description,
                            -1,
                            m_adapter_description.name.data(),
                            size,
                            nullptr,
                            nullptr);
        m_adapter_description.dedicated_video_memory = (float)m_adapter_desc.DedicatedVideoMemory / 1024 / 1024;
        m_adapter_description.system_memory =
            (float)m_adapter_desc.DedicatedSystemMemory / 1024 / 1024 + (float)m_adapter_desc.SharedSystemMemory / 1024 / 1024;
    }

    void Context::CreateDevice()
    {
        [[maybe_unused]]
        auto result = D3D12CreateDevice(m_adapter, D3D_FEATURE_LEVEL_12_1, IID_PPV_ARGS(&m_device));

#ifdef SWIFT_DEBUG
        ID3D12InfoQueue1* info_queue;
        result = m_device->QueryInterface(IID_PPV_ARGS(&info_queue));
        DWORD callback_cookie = 0;
        [[maybe_unused]]
        const auto register_result = info_queue->RegisterMessageCallback(
            [](D3D12_MESSAGE_CATEGORY,
               const D3D12_MESSAGE_SEVERITY severity,
               D3D12_MESSAGE_ID,
               const LPCSTR p_description,
               void*)
            {
                const auto description = std::string(p_description);
                switch (severity)
                {
                    case D3D12_MESSAGE_SEVERITY_CORRUPTION:
                        printf(std::format("[DX12] {} \n", description).c_str());
                        break;
                    case D3D12_MESSAGE_SEVERITY_ERROR:
                        printf(std::format("[DX12] {} \n", description).c_str());
                        break;
                    case D3D12_MESSAGE_SEVERITY_WARNING:
                        printf(std::format("[DX12] {} \n", description).c_str());
                        break;
                    case D3D12_MESSAGE_SEVERITY_INFO:
                    case D3D12_MESSAGE_SEVERITY_MESSAGE:
                        break;
                }
            },
            D3D12_MESSAGE_CALLBACK_FLAG_NONE,
            nullptr,
            &callback_cookie);
#endif
    }

    void Context::CreateDescriptorHeaps()
    {
        m_rtv_heap = std::make_shared<DescriptorHeap>(m_device, D3D12_DESCRIPTOR_HEAP_TYPE_RTV, 64);
        m_dsv_heap = std::make_shared<DescriptorHeap>(m_device, D3D12_DESCRIPTOR_HEAP_TYPE_DSV, 64);
        m_cbv_srv_uav_heap = std::make_shared<DescriptorHeap>(m_device, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 4096);
    }

    typedef HRESULT(__stdcall* PFN_DxcCreateInstance)(REFCLSID rclsid, REFIID riid, LPVOID* ppv);

    void Context::CreateFrameData()
    {
        for (auto& [command, fence_value] : m_frame_data)
        {
            command = CreateCommand(QueueType::eGraphics);
        }
    }

    void Context::CreateQueues()
    {
        m_graphics_queue = CreateQueue({.type = QueueType::eGraphics, .priority = QueuePriority::eHigh});
        m_copy_queue = CreateQueue({.type = QueueType::eTransfer, .priority = QueuePriority::eNormal});
    }

    void Context::CreateTextures(const ContextCreateInfo& create_info)
    {
        constexpr auto format = Format::eRGBA8_UNORM;
        for (int i = 0; i < m_swapchain_textures.size(); i++)
        {
            ID3D12Resource* back_buffer;
            auto swapchain = static_cast<IDXGISwapChain4*>(m_swapchain->GetSwapchain());
            auto result = swapchain->GetBuffer(i, IID_PPV_ARGS(&back_buffer));
            auto resource = std::make_shared<Resource>(back_buffer);
            TextureCreateInfo tex_create_info{
                .width = create_info.width,
                .height = create_info.height,
                .mip_levels = 1,
                .array_size = 1,
                .format = format,
                .flags = EnumFlags(TextureFlags::eRenderTarget) | EnumFlags(TextureFlags::eShaderResource),
                .msaa = std::nullopt,
                .resource = resource,
            };
            m_swapchain_textures[i] = CreateTexture(tex_create_info);
        }
    }

    void Context::CreateSwapchain(const ContextCreateInfo& create_info)
    {
        auto queue = static_cast<ID3D12CommandQueue*>(m_graphics_queue->GetQueue());
        m_swapchain = std::make_shared<Swapchain>(m_factory, queue, create_info);
    }
}  // namespace Swift::D3D12
