#include "d3d12/d3d12_context.hpp"
#include "d3d12/d3d12_helpers.hpp"
#include "d3d12/d3d12_buffer.hpp"
#include "d3d12/d3d12_command.hpp"
#include "d3d12/d3d12_queue.hpp"
#include "d3d12/d3d12_shader.hpp"
#include "d3d12/d3d12_texture.hpp"
#include "d3d12/d3d12_swapchain.hpp"
#include "swift_shader_data.hpp"
#include "swift_helpers.hpp"
#include "format"
#include "d3d12/d3d12_texture_view.hpp"
#include "d3d12/d3d12_buffer_view.hpp"
#include "d3d12/d3d12_sampler.hpp"

extern "C" { __declspec(dllexport) extern const UINT D3D12SDKVersion = D3D12_SDK_VERSION; }
extern "C" { __declspec(dllexport) extern const char* D3D12SDKPath = SWIFT_D3D12_SDK_PATH; }

namespace Swift::D3D12
{
    Context::Context(const ContextCreateInfo& create_info) : IContext(create_info)
    {
        CreateBackend();
        CreateDevice();
        CreateAllocator();
        CreateDescriptorHeaps(create_info);
        CreateRootSignature();
        CreateQueues();
        CreateFrameData();
        CreateSwapchain(create_info);
        CreateTextures(create_info);
        CreateMipMapShader();
    }

    Context::~Context()
    {
        m_graphics_queue->WaitIdle();

        m_root_signature->Release();
        m_swapchain.reset();

        for (auto* queue : m_queues)
        {
            queue->WaitIdle();
            DestroyQueue(queue);
        }

        for (auto* shader : m_shaders)
        {
            DestroyShader(shader);
        }

        for (auto* buffer : m_buffer_views)
        {
            DestroyBufferView(buffer);
        }

        for (auto* texture : m_texture_views)
        {
            DestroyTextureView(texture);
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

        m_rtv_heap.reset();
        m_dsv_heap.reset();
        m_cbv_srv_uav_heap.reset();

        m_allocator->Release();
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

    ICommand* Context::CreateCommand(IQueue* queue, const std::string_view debug_name)
    {
        return CreateObject(
            [&]
            {
                return new Command(this,
                                   m_cbv_srv_uav_heap.get(),
                                   m_sampler_heap.get(),
                                   m_root_signature,
                                   queue->GetQueueType(),
                                   debug_name);
            },
            m_commands,
            m_free_commands);
    }

    IQueue* Context::CreateQueue(const QueueCreateInfo& info)
    {
        return CreateObject([&] { return new Queue(m_device, info); }, m_queues, m_free_queues);
    }

    IBuffer* Context::CreateBuffer(const BufferCreateInfo& info)
    {
        return CreateObject([&] { return new Buffer(this, info); }, m_buffers, m_free_buffers);
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
            const auto* src = static_cast<const char*>(create_info.data);
            auto* const upload_buffer = CreateBuffer(buffer_info);
            auto [footprints, num_rows, row_sizes, total_size] = GetTextureCopyData(m_device, create_info);
            for (uint32_t i = 0; i < create_info.mip_levels * create_info.array_size; ++i)
            {
                for (uint32_t row = 0; row < num_rows[i]; ++row)
                {
                    upload_buffer->Write(src + row * row_sizes[i],
                                         footprints[i].Offset + row * footprints[i].Footprint.RowPitch,
                                         row_sizes[i]);
                }

                src += num_rows[i] * row_sizes[i];
            }
            auto* const copy_command = CreateCommand(m_graphics_queue);
            copy_command->Begin();
            copy_command->CopyBufferToTexture(upload_buffer, texture, create_info.mip_levels, create_info.array_size);
            copy_command->TransitionImage(texture, ResourceState::eCommon);
            copy_command->End();

            const auto value = m_graphics_queue->Execute(copy_command);
            m_graphics_queue->Wait(value);

            DestroyCommand(copy_command);
            DestroyBuffer(upload_buffer);

            if (create_info.gen_mipmaps)
            {
                create_info.mip_levels = info.mip_levels == 0 ? CalculateMaxMips(info.width, info.height) : info.mip_levels;
                std::vector<ITextureView*> texture_srvs;
                std::vector<ITextureView*> texture_uavs;

                auto* const command = CreateCommand(m_graphics_queue);
                command->Begin();

                command->BindShader(m_mipmap_shader);

                for (uint32_t src_mip = 0; src_mip < create_info.mip_levels - 1; src_mip++)
                {
                    texture_srvs.emplace_back(CreateTextureView(texture,
                                                                {
                                                                    .type = TextureViewType::eShaderResource,
                                                                    .base_mip_level = src_mip,
                                                                }));
                    texture_uavs.emplace_back(CreateTextureView(texture,
                                                                {
                                                                    .type = TextureViewType::eShaderResource,
                                                                    .base_mip_level = src_mip + 1,
                                                                }));

                    struct PushConstant
                    {
                        uint32_t sampler_index;
                        uint32_t mip1_index;
                        uint32_t src_index;
                        std::array<float, 2> texel_size;
                    };

                    const uint32_t dst_width = std::max(create_info.width >> (src_mip + 1), 1u);
                    const uint32_t dst_height = std::max(create_info.height >> (src_mip + 1), 1u);

                    PushConstant pc{
                        .sampler_index = m_mipmap_sampler->GetDescriptorIndex(),
                        .mip1_index = texture_uavs[src_mip]->GetDescriptorIndex(),
                        .src_index = texture_srvs[src_mip]->GetDescriptorIndex(),
                        .texel_size = {1.0f / static_cast<float>(dst_width), 1.0f / static_cast<float>(dst_height)}};
                    command->PushConstants(&pc, sizeof(PushConstant));

                    command->DispatchCompute(std::max(dst_width / 8u, 1u), std::max(dst_height / 8u, 1u), 1);

                    command->UAVBarrier(texture);
                }

                command->End();
                const auto fence_value = GetGraphicsQueue()->Execute(command);
                GetGraphicsQueue()->Wait(fence_value);

                for (auto* texture_uav : texture_uavs)
                {
                    DestroyTextureView(texture_uav);
                }

                for (auto* texture_srv : texture_srvs)
                {
                    DestroyTextureView(texture_srv);
                }
            }
        }

        return texture;
    }

    ISampler* Context::CreateSampler(const SamplerCreateInfo& info)
    {
        return CreateObject([&] { return new Sampler(this, info); }, m_samplers, m_free_samplers);
    }

    IShader* Context::CreateShader(const GraphicsShaderCreateInfo& info)
    {
        return CreateObject([&] { return new Shader(m_device, m_root_signature, info); }, m_shaders, m_free_shaders);
    }

    IShader* Context::CreateShader(const ComputeShaderCreateInfo& info)
    {
        return CreateObject([&] { return new Shader(m_device, m_root_signature, info); }, m_shaders, m_free_shaders);
    }

    ITextureView* Context::CreateTextureView(ITexture* texture, const TextureViewCreateInfo& info)
    {
        return CreateObject([&] { return new TextureView(this, texture, info); }, m_texture_views, m_free_texture_views);
    }
    IBufferView* Context::CreateBufferView(IBuffer* buffer, const BufferViewCreateInfo& info)
    {
        return CreateObject([&] { return new BufferView(this, buffer, info); }, m_buffer_views, m_free_buffer_views);
    }

    void Context::DestroyCommand(ICommand* command) { DestroyObject(command, m_commands, m_free_commands); }
    void Context::DestroyQueue(IQueue* queue) { DestroyObject(queue, m_queues, m_free_queues); }
    void Context::DestroyBuffer(IBuffer* buffer) { DestroyObject(buffer, m_buffers, m_free_buffers); }
    void Context::DestroyTexture(ITexture* texture) { DestroyObject(texture, m_textures, m_free_textures); }
    void Context::DestroyShader(IShader* shader) { DestroyObject(shader, m_shaders, m_free_shaders); }
    void Context::DestroyTextureView(ITextureView* texture_view)
    {
        DestroyObject(texture_view, m_texture_views, m_free_texture_views);
    }
    void Context::DestroyBufferView(IBufferView* buffer_view)
    {
        DestroyObject(buffer_view, m_buffer_views, m_free_buffer_views);
    }
    void Context::DestroySampler(ISampler* sampler) { DestroyObject(sampler, m_samplers, m_free_samplers); }

    void Context::NewFrame() { m_graphics_queue->Wait(GetFrameData().fence_value); }

    void Context::Present(const bool vsync)
    {
        GetFrameData().fence_value = m_graphics_queue->Execute(GetFrameData().command);
        m_swapchain->Present(vsync);
        m_frame_index = (m_frame_index + 1) % 3;
    }

    void Context::ResizeBuffers(const uint32_t width, const uint32_t height)
    {
        for (auto* const texture : GetSwapchainTextures())
        {
            DestroyTexture(texture);
        }

        for (auto* const render_target : GetSwapchainRenderTargets())
        {
            DestroyTextureView(render_target);
        }

        m_swapchain->Resize(width, height);
        constexpr auto format = Format::eRGBA8_UNORM;
        for (int i = 0; i < m_swapchain_textures.size(); i++)
        {
            ID3D12Resource* back_buffer = nullptr;
            auto* swapchain = m_swapchain->GetSwapchain();
            swapchain->GetBuffer(i, IID_PPV_ARGS(&back_buffer));

            TextureCreateInfo tex_create_info{
                .width = width,
                .height = height,
                .mip_levels = 1,
                .array_size = 1,
                .format = format,
            };
            m_swapchain_textures[i] =
                CreateObject([&] { return new Texture(back_buffer, tex_create_info); }, m_textures, m_free_textures);
            m_swapchain_render_targets[i] = CreateTextureView(m_swapchain_textures[i], {});
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

    ITextureView* Context::GetCurrentRenderTarget() const { return m_swapchain_render_targets[m_swapchain->GetFrameIndex()]; }

    ITexture* Context::GetCurrentSwapchainTexture() const { return m_swapchain_textures[m_swapchain->GetFrameIndex()]; }

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

    void Context::CreateAllocator()
    {
        D3D12MA::ALLOCATOR_DESC desc
        {
            .Flags = D3D12MA::ALLOCATOR_FLAG_SINGLETHREADED,
            .pDevice = m_device,
            .pAdapter = m_adapter,
        };
        D3D12MA::CreateAllocator(&desc, &m_allocator);
    }

    void Context::CreateDescriptorHeaps(const ContextCreateInfo& create_info)
    {
        m_rtv_heap = std::make_unique<DescriptorHeap>(m_device, D3D12_DESCRIPTOR_HEAP_TYPE_RTV, create_info.rtv_handle_count);
        m_dsv_heap = std::make_unique<DescriptorHeap>(m_device, D3D12_DESCRIPTOR_HEAP_TYPE_DSV, create_info.dsv_handle_count);
        m_cbv_srv_uav_heap = std::make_unique<DescriptorHeap>(m_device,
                                                              D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
                                                              create_info.cbv_srv_uav_handle_count);
        m_sampler_heap =
            std::make_unique<DescriptorHeap>(m_device, D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER, create_info.sampler_handle_count);
    }

    typedef HRESULT(__stdcall* PFN_DxcCreateInstance)(REFCLSID rclsid, REFIID riid, LPVOID* ppv);

    void Context::CreateFrameData()
    {
        for (auto& [command, fence_value] : m_frame_data)
        {
            command = CreateCommand(m_graphics_queue);
        }
    }

    void Context::CreateQueues()
    {
        m_graphics_queue =
            CreateQueue({.type = QueueType::eGraphics, .priority = QueuePriority::eHigh, .name = "Swift Graphics Queue"});
    }

    void Context::CreateTextures(const ContextCreateInfo& create_info)
    {
        constexpr auto format = Format::eRGBA8_UNORM;
        for (int i = 0; i < m_swapchain_textures.size(); i++)
        {
            ID3D12Resource* back_buffer = nullptr;
            auto* swapchain = m_swapchain->GetSwapchain();
            swapchain->GetBuffer(i, IID_PPV_ARGS(&back_buffer));

            TextureCreateInfo tex_create_info{
                .width = create_info.width,
                .height = create_info.height,
                .mip_levels = 1,
                .array_size = 1,
                .format = format,
            };

            m_swapchain_textures[i] =
                CreateObject([&] { return new Texture(back_buffer, tex_create_info); }, m_textures, m_free_textures);
            m_swapchain_render_targets[i] = CreateTextureView(m_swapchain_textures[i], {});
        }
    }

    void Context::CreateSwapchain(const ContextCreateInfo& create_info)
    {
        auto* queue = static_cast<ID3D12CommandQueue*>(m_graphics_queue->GetQueue());
        m_swapchain = std::make_unique<Swapchain>(m_factory, queue, create_info);
    }

    void Context::CreateMipMapShader()
    {
        std::vector<SamplerCreateInfo> sampler_descriptors;
        sampler_descriptors.emplace_back(SamplerCreateInfo{});
        const ComputeShaderCreateInfo compute_shader_create_info{
            .code = gen_mips_code,
            .name = "Mip Map Shader",
        };
        m_mipmap_shader = CreateShader(compute_shader_create_info);
        m_mipmap_sampler = CreateSampler({});
    }

    void Context::CreateRootSignature()
    {
        D3D12_DESCRIPTOR_RANGE1 ranges[2];

        ranges[0] = {
            .RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV,
            .NumDescriptors = UINT_MAX,
            .BaseShaderRegister = 0,
            .RegisterSpace = 0,
            .Flags = D3D12_DESCRIPTOR_RANGE_FLAG_DESCRIPTORS_VOLATILE,
            .OffsetInDescriptorsFromTableStart = 0,
        };

        ranges[1] = {
            .RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER,
            .NumDescriptors = UINT_MAX,
            .BaseShaderRegister = 0,
            .RegisterSpace = 0,
            .Flags = D3D12_DESCRIPTOR_RANGE_FLAG_DESCRIPTORS_VOLATILE,
            .OffsetInDescriptorsFromTableStart = 0,
        };
        std::array root_params{
            D3D12_ROOT_PARAMETER1{
                .ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS,
                .Constants =
                    {
                        .ShaderRegister = 0,
                        .RegisterSpace = 0,
                        .Num32BitValues = 32,
                    },
                .ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL,
            },
            D3D12_ROOT_PARAMETER1{
                .ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV,
                .Descriptor =
                    {
                        .ShaderRegister = 1,
                        .RegisterSpace = 0,
                    },
                .ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL,
            },
            D3D12_ROOT_PARAMETER1{
                .ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV,
                .Descriptor =
                    {
                        .ShaderRegister = 2,
                        .RegisterSpace = 0,
                    },
                .ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL,
            },
            D3D12_ROOT_PARAMETER1{
                .ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV,
                .Descriptor =
                    {
                        .ShaderRegister = 3,
                        .RegisterSpace = 0,
                    },
                .ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL,
            },
            D3D12_ROOT_PARAMETER1{
                .ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE,
                .DescriptorTable =
                    {
                        .NumDescriptorRanges = 1,
                        .pDescriptorRanges = &ranges[0],
                    },
                .ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL,
            },
            D3D12_ROOT_PARAMETER1{
                .ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE,
                .DescriptorTable =
                    {
                        .NumDescriptorRanges = 1,
                        .pDescriptorRanges = &ranges[1],
                    },
                .ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL,
            },
        };

        const D3D12_VERSIONED_ROOT_SIGNATURE_DESC root_signature_desc = {
            .Version = D3D_ROOT_SIGNATURE_VERSION_1_1,
            .Desc_1_1 = {
                .NumParameters = static_cast<uint32_t>(root_params.size()),
                .pParameters = root_params.data(),
                .Flags = D3D12_ROOT_SIGNATURE_FLAG_CBV_SRV_UAV_HEAP_DIRECTLY_INDEXED |
                         D3D12_ROOT_SIGNATURE_FLAG_SAMPLER_HEAP_DIRECTLY_INDEXED,
            }};

        ID3DBlob* error_blob = nullptr;
        ID3DBlob* root_signature_blob = nullptr;
        if (const auto result = D3D12SerializeVersionedRootSignature(&root_signature_desc, &root_signature_blob, &error_blob);
            result != S_OK)
        {
            error_blob->Release();
        }

        [[maybe_unused]]
        const auto result = m_device->CreateRootSignature(0,
                                                          root_signature_blob->GetBufferPointer(),
                                                          root_signature_blob->GetBufferSize(),
                                                          IID_PPV_ARGS(&m_root_signature));
        root_signature_blob->Release();
    }
}  // namespace Swift::D3D12
