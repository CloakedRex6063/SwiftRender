#include "d3d12/d3d12_context.hpp"
#include "d3d12/d3d12_buffer.hpp"
#include "d3d12/d3d12_command.hpp"
#include "d3d12/d3d12_queue.hpp"
#include "d3d12/d3d12_shader.hpp"
#include "d3d12/d3d12_texture.hpp"
#include "d3d12/d3d12_resource.hpp"
#include "d3d12/d3d12_swapchain.hpp"
#include "swift_shader_data.hpp"
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
        CreateMipMapShader();
    }

    Context::~Context()
    {
        m_graphics_queue->WaitIdle();

        delete m_swapchain;

        for (auto* queue : m_queues)
        {
            queue->WaitIdle();
            DestroyQueue(queue);
        }

        for (auto* shader : m_shaders)
        {
            DestroyShader(shader);
        }

        for (auto* texture : m_textures)
        {
            DestroyTexture(texture);
        }

        for (auto* buffer : m_buffers)
        {
            DestroyBuffer(buffer);
        }

        for (auto* buffer : m_buffer_srvs)
        {
            DestroyShaderResource(buffer);
        }

        for (auto* buffer : m_buffer_uavs)
        {
            DestroyUnorderedAccessView(buffer);
        }

        for (auto* texture : m_texture_srvs)
        {
            DestroyShaderResource(texture);
        }

        for (auto* texture : m_texture_uavs)
        {
            DestroyUnorderedAccessView(texture);
        }

        for (auto* texture : m_render_targets)
        {
            DestroyRenderTarget(texture);
        }

        for (auto* texture : m_depth_stencils)
        {
            DestroyDepthStencil(texture);
        }

        for (auto* command : m_commands)
        {
            DestroyCommand(command);
        }

        delete m_rtv_heap;
        delete m_dsv_heap;
        delete m_cbv_srv_uav_heap;

        m_factory->Release();
        m_adapter->Release();

#ifdef SWIFT_DEBUG
        ID3D12DebugDevice* debug_device = nullptr;
        m_device->QueryInterface(
            IID_PPV_ARGS(&debug_device)
        );
        debug_device->ReportLiveDeviceObjects(D3D12_RLDO_DETAIL);
        debug_device->Release();
#endif
        m_device->Release();
    }

    void* Context::GetDevice() const { return m_device; }

    void* Context::GetAdapter() const { return m_adapter; }

    void* Context::GetSwapchain() const { return m_swapchain->GetSwapchain(); }

    ICommand* Context::CreateCommand(const QueueType queue_type, const std::string_view debug_name)
    {
        m_commands.push_back(new Command(this, m_cbv_srv_uav_heap, queue_type, debug_name));
        return m_commands.back();
    }

    IQueue* Context::CreateQueue(const QueueCreateInfo& info)
    {
        m_queues.emplace_back(new Queue(m_device, info));
        return m_queues.back();
    }

    IBuffer* Context::CreateBuffer(const BufferCreateInfo& info)
    {
        m_buffers.emplace_back(new Buffer(this, m_cbv_srv_uav_heap, info));
        return m_buffers.back();
    }

    ITexture* Context::CreateTexture(const TextureCreateInfo& info)
    {
        m_textures.emplace_back(new Texture(this, info));
        auto* texture = m_textures.back();

        // if (info.gen_mipmaps && info.mip_levels > 1)
        // {
        //     const auto command = CreateCommand(QueueType::eCompute);
        //     command->Begin();
        //
        //     command->BindShader(m_mipmap_shader);
        //
        //     for (uint32_t src_mip = 0; src_mip < info.mip_levels - 1; src_mip += 4)
        //     {
        //         const uint32_t remaining_mips = info.mip_levels - src_mip - 1;
        //         const uint32_t num_mips_this_dispatch = std::min(4u, remaining_mips);
        //
        //         const uint32_t src_width = std::max(1u, info.width >> src_mip);
        //         const uint32_t src_height = std::max(1u, info.height >> src_mip);
        //
        //         struct PushConstant
        //         {
        //             uint32_t mip1_index;
        //             uint32_t mip2_index;
        //             uint32_t mip3_index;
        //             uint32_t mip4_index;
        //             std::array<float, 2> texel_size;
        //             uint32_t src_index;
        //             uint32_t src_mip_level;
        //             uint32_t num_mip_levels;
        //             std::array<float, 3> padding;
        //         };
        //
        //         PushConstant pc{
        //             .mip1_index = texture->GetUAVDescriptorIndex(src_mip + 2),
        //             .mip2_index = num_mips_this_dispatch >= 2 ? texture->GetUAVDescriptorIndex(src_mip + 3) : 0,
        //             .mip3_index = num_mips_this_dispatch >= 3 ? texture->GetUAVDescriptorIndex(src_mip + 4) : 0,
        //             .mip4_index = num_mips_this_dispatch >= 4 ? texture->GetUAVDescriptorIndex(src_mip + 5) : 0,
        //             .texel_size = {1.0f / static_cast<float>(src_width), 1.0f / static_cast<float>(src_height)},
        //             .src_index = texture->GetSRVDescriptorIndex(src_mip),
        //             .src_mip_level = src_mip,
        //             .num_mip_levels = num_mips_this_dispatch,
        //         };
        //         command->PushConstants(&pc, sizeof(PushConstant));
        //
        //         // Dispatch size is based on mip1
        //         const uint32_t dstMip1Width = std::max(1u, src_width >> 1);
        //         const uint32_t dstMip1Height = std::max(1u, src_height >> 1);
        //
        //         const uint32_t dispatchX = (dstMip1Width + 7) / 8;
        //         const uint32_t dispatchY = (dstMip1Height + 7) / 8;
        //
        //         command->DispatchCompute(dispatchX, dispatchY, 1);
        //
        //         command->UAVBarrier(texture->GetResource());
        //     }
        //
        //     command->End();
        // }

        if (info.data)
        {
            const uint32_t size = GetTextureSize(info);
            const BufferCreateInfo buffer_info = {
                .size = size,
                .data = info.data,
            };
            auto* const upload_buffer = CreateBuffer(buffer_info);
            auto* const copy_command = CreateCommand(QueueType::eTransfer);
            copy_command->Begin();
            copy_command->CopyBufferToTexture(this, {upload_buffer, texture});
            copy_command->End();

            const auto value = m_copy_queue->Execute(copy_command);
            m_copy_queue->Wait(value);

            DestroyCommand(copy_command);
            DestroyBuffer(upload_buffer);
        }

        return texture;
    }

    IRenderTarget* Context::CreateRenderTarget(ITexture* texture, const uint32_t mip)
    {
        m_render_targets.emplace_back(new RenderTarget(this, texture, mip));
        return m_render_targets.back();
    }

    IDepthStencil* Context::CreateDepthStencil(ITexture* texture, uint32_t mip)
    {
        m_depth_stencils.emplace_back(new DepthStencil(this, texture, mip));
        return m_depth_stencils.back();
    }

    ITextureSRV* Context::CreateShaderResource(ITexture* texture, const uint32_t most_detailed_mip, const uint32_t mip_levels)
    {
        m_texture_srvs.emplace_back(new TextureSRV(this, texture, most_detailed_mip, mip_levels));
        return m_texture_srvs.back();
    }
    IBufferSRV* Context::CreateShaderResource(IBuffer* buffer, const BufferSRVCreateInfo& srv_create_info)
    {
        m_buffer_srvs.emplace_back(new BufferSRV(this, buffer, srv_create_info));
        return m_buffer_srvs.back();
    }
    ITextureUAV* Context::CreateUnorderedAccessView(ITexture* texture, uint32_t mip)
    {
        m_texture_uavs.emplace_back(new TextureUAV(this, texture, mip));
        return m_texture_uavs.back();
    }
    IBufferUAV* Context::CreateUnorderedAccessView(IBuffer* buffer, const BufferUAVCreateInfo& uav_create_info)
    {
        m_buffer_uavs.emplace_back(new BufferUAV(this, buffer, uav_create_info));
        return m_buffer_uavs.back();
    }

    IShader* Context::CreateShader(const GraphicsShaderCreateInfo& info)
    {
        m_shaders.emplace_back(new Shader(m_device, info));
        return m_shaders.back();
    }

    IShader* Context::CreateShader(const ComputeShaderCreateInfo& info)
    {
        m_shaders.emplace_back(new Shader(m_device, info));
        return m_shaders.back();
    }

    std::shared_ptr<IResource> Context::CreateResource(const BufferCreateInfo& info)
    {
        return std::make_shared<Resource>(m_device, info);
    }

    std::shared_ptr<IResource> Context::CreateResource(const TextureCreateInfo& info)
    {
        return std::make_shared<Resource>(m_device, info);
    }

    void Context::DestroyCommand(ICommand* command)
    {
        if (const auto it = std::ranges::find(m_commands, command); it != m_commands.end())
        {
            delete *it;
            *it = nullptr;
        }
    }
    void Context::DestroyQueue(IQueue* queue)
    {
        if (const auto it = std::ranges::find(m_queues, queue); it != m_queues.end())
        {
            delete *it;
            *it = nullptr;
        }
    }
    void Context::DestroyBuffer(IBuffer* buffer)
    {
        if (const auto it = std::ranges::find(m_buffers, buffer); it != m_buffers.end())
        {
            delete *it;
            *it = nullptr;
        }
    }
    void Context::DestroyTexture(ITexture* texture)
    {
        if (const auto it = std::ranges::find(m_textures, texture); it != m_textures.end())
        {
            delete *it;
            *it = nullptr;
        }
    }
    void Context::DestroyShader(IShader* shader)
    {
        if (const auto it = std::ranges::find(m_shaders, shader); it != m_shaders.end())
        {
            delete *it;
            *it = nullptr;
        }
    }
    void Context::DestroyRenderTarget(IRenderTarget* render_target)
    {
        if (const auto it = std::ranges::find(m_render_targets, render_target); it != m_render_targets.end())
        {
            delete *it;
            *it = nullptr;
        }
    }
    void Context::DestroyDepthStencil(IDepthStencil* depth_stencil)
    {
        if (const auto it = std::ranges::find(m_depth_stencils, depth_stencil); it != m_depth_stencils.end())
        {
            delete *it;
            *it = nullptr;
        }
    }
    void Context::DestroyShaderResource(ITextureSRV* srv)
    {
        if (const auto it = std::ranges::find(m_texture_srvs, srv); it != m_texture_srvs.end())
        {
            delete *it;
            *it = nullptr;
        }
    }
    void Context::DestroyShaderResource(IBufferSRV* srv)
    {
        if (const auto it = std::ranges::find(m_buffer_srvs, srv); it != m_buffer_srvs.end())
        {
            delete *it;
            *it = nullptr;
        }
    }
    void Context::DestroyUnorderedAccessView(IBufferUAV* uav)
    {
        if (const auto it = std::ranges::find(m_buffer_uavs, uav); it != m_buffer_uavs.end())
        {
            delete *it;
            *it = nullptr;
        }
    }
    void Context::DestroyUnorderedAccessView(ITextureUAV* uav)
    {
        if (const auto it = std::ranges::find(m_texture_uavs, uav); it != m_texture_uavs.end())
        {
            delete *it;
            *it = nullptr;
        }
    }

    void Context::UpdateTextureRegion(ITexture* texture, const TextureUpdateRegion& texture_region) {}

    void Context::Present(const bool vsync)
    {
        GetFrameData().fence_value = m_graphics_queue->Execute(GetFrameData().command);
        m_swapchain->Present(vsync);
        m_graphics_queue->Wait(GetFrameData().fence_value);
        m_frame_index = m_swapchain->GetFrameIndex();
    }

    void Context::ResizeBuffers(const uint32_t width, const uint32_t height)
    {
        for (auto* const texture : GetSwapchainTextures())
        {
            DestroyTexture(texture);
        }

        for (auto* const render_target : GetSwapchainRenderTargets())
        {
            DestroyRenderTarget(render_target);
        }

        m_swapchain->Resize(width, height);
        constexpr auto format = Format::eRGBA8_UNORM;
        for (int i = 0; i < m_swapchain_textures.size(); i++)
        {
            ID3D12Resource* back_buffer = nullptr;
            auto* swapchain = static_cast<IDXGISwapChain4*>(m_swapchain->GetSwapchain());
            swapchain->GetBuffer(i, IID_PPV_ARGS(&back_buffer));

            auto resource = std::make_shared<Resource>(back_buffer);

            TextureCreateInfo tex_create_info{
                .width = width,
                .height = height,
                .mip_levels = 1,
                .array_size = 1,
                .format = format,
                .resource = resource,
            };
            m_swapchain_textures[i] = CreateTexture(tex_create_info);
            m_swapchain_render_targets[i] = CreateRenderTarget(m_swapchain_textures[i], 0);
        }
        m_frame_index = m_swapchain->GetFrameIndex();
    }

    void Context::CreateBackend()
    {
        CreateDXGIFactory2(0, IID_PPV_ARGS(&m_factory));
        m_factory->EnumAdapterByGpuPreference(0, DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE, IID_PPV_ARGS(&m_adapter));

#ifdef SWIFT_DEBUG
        D3D12GetDebugInterface(IID_PPV_ARGS(&m_debug_controller));
        m_debug_controller->EnableDebugLayer();
        m_debug_controller->Release();
#endif

        m_adapter->GetDesc3(&m_adapter_desc);

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
        D3D12CreateDevice(m_adapter, D3D_FEATURE_LEVEL_12_1, IID_PPV_ARGS(&m_device));

#ifdef SWIFT_DEBUG
        ID3D12InfoQueue1* info_queue = nullptr;
        m_device->QueryInterface(IID_PPV_ARGS(&info_queue));
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
        m_rtv_heap = new DescriptorHeap(m_device, D3D12_DESCRIPTOR_HEAP_TYPE_RTV, 64);
        m_dsv_heap = new DescriptorHeap(m_device, D3D12_DESCRIPTOR_HEAP_TYPE_DSV, 64);
        m_cbv_srv_uav_heap = new DescriptorHeap(m_device, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 4096);
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
        m_graphics_queue = CreateQueue({.type = QueueType::eGraphics, .priority = QueuePriority::eHigh, .name = "Swift Graphics Queue"});
        m_copy_queue = CreateQueue({.type = QueueType::eTransfer, .priority = QueuePriority::eNormal, .name = "Swift Transfer Queue"});
    }

    void Context::CreateTextures(const ContextCreateInfo& create_info)
    {
        constexpr auto format = Format::eRGBA8_UNORM;
        for (int i = 0; i < m_swapchain_textures.size(); i++)
        {
            ID3D12Resource* back_buffer = nullptr;
            auto* swapchain = static_cast<IDXGISwapChain4*>(m_swapchain->GetSwapchain());
            swapchain->GetBuffer(i, IID_PPV_ARGS(&back_buffer));

            auto resource = std::make_shared<Resource>(back_buffer);

            TextureCreateInfo tex_create_info{
                .width = create_info.width,
                .height = create_info.height,
                .mip_levels = 1,
                .array_size = 1,
                .format = format,
                .resource = resource,
            };
            m_swapchain_textures[i] = CreateTexture(tex_create_info);
            m_swapchain_render_targets[i] = CreateRenderTarget(m_swapchain_textures[i], 0);
        }
    }

    void Context::CreateSwapchain(const ContextCreateInfo& create_info)
    {
        auto* queue = static_cast<ID3D12CommandQueue*>(m_graphics_queue->GetQueue());
        m_swapchain = new Swapchain(m_factory, queue, create_info);
    }

    void Context::CreateMipMapShader()
    {
        std::vector<SamplerDescriptor> sampler_descriptors;
        sampler_descriptors.emplace_back(SamplerDescriptor{});
        const ComputeShaderCreateInfo compute_shader_create_info{
            .code = gen_mips_code,
            .descriptors = {},
            .static_samplers = sampler_descriptors,
            .name = "Mip Map Shader"
        };
        m_mipmap_shader = CreateShader(compute_shader_create_info);
    }
}  // namespace Swift::D3D12
