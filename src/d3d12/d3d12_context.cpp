#include "d3d12/d3d12_context.hpp"
#include "d3d12/d3d12_helpers.hpp"
#include "d3d12/d3d12_heap.hpp"
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
        CreateDescriptorHeaps(create_info);
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

        for (auto* buffer : m_buffers)
        {
            DestroyBuffer(buffer);
        }

        for (auto* texture : m_textures)
        {
            DestroyTexture(texture);
        }

        for (auto* command : m_commands)
        {
            DestroyCommand(command);
        }

        for (auto* heap : m_heaps)
        {
            DestroyHeap(heap);
        }

        delete m_rtv_heap;
        delete m_dsv_heap;
        delete m_cbv_srv_uav_heap;

        m_factory->Release();
        m_adapter->Release();

#ifdef SWIFT_DEBUG
        ID3D12DebugDevice* debug_device = nullptr;
        m_device->QueryInterface(IID_PPV_ARGS(&debug_device));
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
        return CreateObject([&] { return new Command(this, m_cbv_srv_uav_heap, queue_type, debug_name); },
                            m_commands,
                            m_free_commands);
    }

    IQueue* Context::CreateQueue(const QueueCreateInfo& info)
    {
        return CreateObject([&] { return new Queue(m_device, info); }, m_queues, m_free_queues);
    }

    IBuffer* Context::CreateBuffer(const BufferCreateInfo& info)
    {
        return CreateObject([&] { return new Buffer(this, m_cbv_srv_uav_heap, info); }, m_buffers, m_free_buffers);
    }

    ITexture* Context::CreateTexture(const TextureCreateInfo& info)
    {
        auto create_info = info;
        create_info.flags |= info.gen_mipmaps ? TextureFlags::eUnorderedAccess : TextureFlags::eNone;
        create_info.mip_levels = info.mip_levels == 0 ? CalculateMaxMips(info.width, info.height) : info.mip_levels;
        auto* texture = CreateObject([&] { return new Texture(this, create_info); }, m_textures, m_free_textures);

        if (create_info.data)
        {
            if (create_info.gen_mipmaps)
            {
                create_info.mip_levels = 1;
            }

            const uint32_t size = GetTextureSize(m_device, create_info);
            const BufferCreateInfo buffer_info = {
                .size = size,
            };
            auto* const upload_buffer = CreateBuffer(buffer_info);
            CopyTextureData(m_device, upload_buffer, create_info);
            auto* const copy_command = CreateCommand(QueueType::eTransfer);
            copy_command->Begin();
            copy_command->CopyBufferToTexture(this, upload_buffer, texture, create_info.mip_levels, create_info.array_size);
            copy_command->TransitionResource(texture->GetResource(), ResourceState::eCommon);
            copy_command->End();

            const auto value = m_copy_queue->Execute(copy_command);
            m_copy_queue->Wait(value);

            DestroyCommand(copy_command);
            DestroyBuffer(upload_buffer);

            if (create_info.gen_mipmaps)
            {
                create_info.mip_levels = info.mip_levels == 0 ? CalculateMaxMips(info.width, info.height) : info.mip_levels;
                std::vector<ITextureSRV*> texture_srvs;
                std::vector<ITextureUAV*> texture_uavs;

                auto* const command = CreateCommand(QueueType::eCompute);
                command->Begin();

                command->BindShader(m_mipmap_shader);

                for (uint32_t src_mip = 0; src_mip < create_info.mip_levels - 1; src_mip++)
                {
                    texture_srvs.emplace_back(CreateShaderResource(texture, 1, src_mip));
                    texture_uavs.emplace_back(CreateUnorderedAccessView(texture, src_mip + 1));

                    struct PushConstant
                    {
                        uint32_t mip1_index;
                        uint32_t src_index;
                        std::array<float, 2> texel_size;
                    };

                    const uint32_t dst_width = std::max(create_info.width >> (src_mip + 1), 1u);
                    const uint32_t dst_height = std::max(create_info.height >> (src_mip + 1), 1u);

                    PushConstant pc{
                        .mip1_index = texture_uavs[src_mip]->GetDescriptorIndex(),
                        .src_index = texture_srvs[src_mip]->GetDescriptorIndex(),
                        .texel_size = {1.0f / static_cast<float>(dst_width), 1.0f / static_cast<float>(dst_height)}};
                    command->PushConstants(&pc, sizeof(PushConstant));

                    command->DispatchCompute(std::max(dst_width / 8u, 1u), std::max(dst_height / 8u, 1u), 1);

                    command->UAVBarrier(texture->GetResource());
                }

                command->End();
                const auto fence_value = GetComputeQueue()->Execute(command);
                GetComputeQueue()->Wait(fence_value);

                for (auto* texture_uav : texture_uavs)
                {
                    DestroyUnorderedAccessView(texture_uav);
                }

                for (auto* texture_srv : texture_srvs)
                {
                    DestroyShaderResource(texture_srv);
                }
            }
        }

        return texture;
    }

    IRenderTarget* Context::CreateRenderTarget(ITexture* texture, const uint32_t mip)
    {
        return CreateObject([&] { return new RenderTarget(this, texture, mip); }, m_render_targets, m_free_render_targets);
    }

    IDepthStencil* Context::CreateDepthStencil(ITexture* texture, const uint32_t mip)
    {
        return CreateObject([&] { return new DepthStencil(this, texture, mip); }, m_depth_stencils, m_free_depth_stencils);
    }

    ITextureSRV* Context::CreateShaderResource(ITexture* texture, const uint32_t mip_levels, const uint32_t most_detailed_mip)
    {
        return CreateObject([&] { return new TextureSRV(this, texture, most_detailed_mip, mip_levels); },
                            m_texture_srvs,
                            m_free_texture_srvs);
    }
    IBufferSRV* Context::CreateShaderResource(IBuffer* buffer, const BufferSRVCreateInfo& srv_create_info)
    {
        return CreateObject([&] { return new BufferSRV(this, buffer, srv_create_info); }, m_buffer_srvs, m_free_buffer_srvs);
    }
    ITextureUAV* Context::CreateUnorderedAccessView(ITexture* texture, const uint32_t mip)
    {
        return CreateObject([&] { return new TextureUAV(this, texture, mip); }, m_texture_uavs, m_free_texture_uavs);
    }
    IBufferUAV* Context::CreateUnorderedAccessView(IBuffer* buffer, const BufferUAVCreateInfo& uav_create_info)
    {
        return CreateObject([&] { return new BufferUAV(this, buffer, uav_create_info); }, m_buffer_uavs, m_free_buffer_uavs);
    }

    IShader* Context::CreateShader(const GraphicsShaderCreateInfo& info)
    {
        return CreateObject([&] { return new Shader(m_device, info); }, m_shaders, m_free_shaders);
    }

    IShader* Context::CreateShader(const ComputeShaderCreateInfo& info)
    {
        return CreateObject([&] { return new Shader(m_device, info); }, m_shaders, m_free_shaders);
    }

    std::shared_ptr<IResource> Context::CreateResource(const BufferCreateInfo& info)
    {
        return std::make_shared<Resource>(m_device, info);
    }

    std::shared_ptr<IResource> Context::CreateResource(const TextureCreateInfo& info)
    {
        return std::make_shared<Resource>(m_device, info);
    }

    IHeap* Context::CreateHeap(const HeapCreateInfo& heap_create_info)
    {
        return CreateObject([&] { return new Heap(this, heap_create_info); }, m_heaps, m_free_heaps);
    }

    void Context::DestroyCommand(ICommand* command) { DestroyObject(command, m_commands, m_free_commands); }
    void Context::DestroyQueue(IQueue* queue) { DestroyObject(queue, m_queues, m_free_queues); }
    void Context::DestroyBuffer(IBuffer* buffer) { DestroyObject(buffer, m_buffers, m_free_buffers); }
    void Context::DestroyTexture(ITexture* texture) { DestroyObject(texture, m_textures, m_free_textures); }
    void Context::DestroyShader(IShader* shader) { DestroyObject(shader, m_shaders, m_free_shaders); }
    void Context::DestroyRenderTarget(IRenderTarget* render_target)
    {
        DestroyObject(render_target, m_render_targets, m_free_render_targets);
    }
    void Context::DestroyDepthStencil(IDepthStencil* depth_stencil)
    {
        DestroyObject(depth_stencil, m_depth_stencils, m_free_depth_stencils);
    }
    void Context::DestroyShaderResource(ITextureSRV* srv) { DestroyObject(srv, m_texture_srvs, m_free_texture_srvs); }
    void Context::DestroyShaderResource(IBufferSRV* srv) { DestroyObject(srv, m_buffer_srvs, m_free_buffer_srvs); }
    void Context::DestroyUnorderedAccessView(IBufferUAV* uav) { DestroyObject(uav, m_buffer_uavs, m_free_buffer_uavs); }
    void Context::DestroyUnorderedAccessView(ITextureUAV* uav) { DestroyObject(uav, m_texture_uavs, m_free_texture_uavs); }

    void Context::DestroyHeap(IHeap* heap) { DestroyObject(heap, m_heaps, m_free_heaps); }

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

    uint32_t Context::CalculateAlignedTextureSize(const TextureCreateInfo& info)
    {
        return Align(GetTextureSize(m_device, info), D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT);
    }
    uint32_t Context::CalculateAlignedBufferSize(const BufferCreateInfo& info)
    {
        return Align(GetBufferSize(m_device, info), D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT);
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

    void Context::CreateDescriptorHeaps(const ContextCreateInfo& create_info)
    {
        m_rtv_heap = new DescriptorHeap(m_device, D3D12_DESCRIPTOR_HEAP_TYPE_RTV, create_info.rtv_handle_count);
        m_dsv_heap = new DescriptorHeap(m_device, D3D12_DESCRIPTOR_HEAP_TYPE_DSV, create_info.dsv_handle_count);
        m_cbv_srv_uav_heap =
            new DescriptorHeap(m_device, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, create_info.cbv_srv_uav_handle_count);
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
        m_graphics_queue =
            CreateQueue({.type = QueueType::eGraphics, .priority = QueuePriority::eHigh, .name = "Swift Graphics Queue"});
        m_compute_queue =
            CreateQueue({.type = QueueType::eCompute, .priority = QueuePriority::eNormal, .name = "Swift Compute Queue"});
        m_copy_queue =
            CreateQueue({.type = QueueType::eTransfer, .priority = QueuePriority::eNormal, .name = "Swift Transfer Queue"});
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
            .name = "Mip Map Shader",
        };
        m_mipmap_shader = CreateShader(compute_shader_create_info);
    }
}  // namespace Swift::D3D12
