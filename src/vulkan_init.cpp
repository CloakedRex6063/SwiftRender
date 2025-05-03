#include "vulkan_init.hpp"

#include "X11/Xlib.h"
#include "algorithm"
#include "array"
#include "cstring"
#include "ranges"
#include "vector"
#include "warnings.hpp"
SWIFT_WARN_BEGIN()
SWIFT_WARN_DISABLE()
#include "vk_mem_alloc.h"
#include "vulkan/vulkan_xlib.h"
SWIFT_WARN_END()

namespace Swift::Vulkan
{
    std::expected<VkInstance, Error> CreateInstance()
    {
        VkInstance instance = nullptr;

        VkApplicationInfo info{
            .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
            .pApplicationName = "Swift",
            .pEngineName = "Swift",
            .engineVersion = 0,
            .apiVersion = VK_API_VERSION_1_3,
        };

        std::array extensions{ VK_KHR_SURFACE_EXTENSION_NAME,
                               VK_KHR_XLIB_SURFACE_EXTENSION_NAME,  // TODO: put under linux define
#ifdef SWIFT_VULKAN_SDK
                               VK_EXT_DEBUG_UTILS_EXTENSION_NAME
#endif
        };

#ifdef SWIFT_VULKAN_SDK
        std::array layers{
            "VK_LAYER_KHRONOS_validation",
        };
#endif

        const VkInstanceCreateInfo create_info{
            .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
            .pApplicationInfo = &info,
#ifdef SWIFT_VULKAN_SDK
            .enabledLayerCount = layers.size(),
            .ppEnabledLayerNames = layers.data(),
#endif
            .enabledExtensionCount = extensions.size(),
            .ppEnabledExtensionNames = extensions.data(),
        };
        const auto result = vkCreateInstance(&create_info, nullptr, &instance);
        return HandleResult(result, Error::eInstanceCreateFailed, instance);
    };

    std::expected<uint, Error> GetGraphicsQueueFamilyIndex(VkPhysicalDevice physical_device)
    {
        uint32_t queue_count = 0;
        vkGetPhysicalDeviceQueueFamilyProperties2(physical_device, &queue_count, nullptr);
        if (queue_count == 0) return std::unexpected(Error::eQueueNotFound);

        std::vector<VkQueueFamilyProperties2> queue_families(queue_count);
        for (auto& queueFamily : queue_families)
        {
            queueFamily.sType = VK_STRUCTURE_TYPE_QUEUE_FAMILY_PROPERTIES_2;
        }
        vkGetPhysicalDeviceQueueFamilyProperties2(physical_device, &queue_count, queue_families.data());

        const auto it = std::ranges::find_if(
            queue_families,
            [&](const VkQueueFamilyProperties2& queueFamily)
            {
                return queueFamily.queueFamilyProperties.queueFlags & VK_QUEUE_GRAPHICS_BIT;
            });

        if (it == queue_families.end()) return std::unexpected(Error::eQueueNotFound);

        return std::distance(it, queue_families.begin());
    }

    std::expected<PhysicalDevice, Error> ChoosePhysicalDevice(VkInstance instance, DevicePreference device_preference)
    {
        VkPhysicalDevice physical_device = nullptr;
        uint count = 0;
        vkEnumeratePhysicalDevices(instance, &count, nullptr);
        if (count == 0) return std::unexpected(Error::eGPUSelectionFailed);

        std::vector<VkPhysicalDevice> devices(count);
        vkEnumeratePhysicalDevices(instance, &count, devices.data());

        DeviceFeatures device_features;

        for (const auto& device : devices)
        {
            VkPhysicalDeviceProperties2 props{
                .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2,
            };
            vkGetPhysicalDeviceProperties2(device, &props);

            if (!GetGraphicsQueueFamilyIndex(device)) continue;

            VkPhysicalDeviceFeatures2 features{
                .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2,
            };
            vkGetPhysicalDeviceFeatures2(device, &features);

            if (!features.features.geometryShader) continue;

            if (!features.features.fillModeNonSolid) continue;

            if (!features.features.textureCompressionBC) continue;

            device_features.MultiDrawIndirect = features.features.multiDrawIndirect;
            device_features.Widelines = features.features.wideLines;

            if (!features.features.fillModeNonSolid) continue;

            if (!features.features.depthClamp) continue;

            if (!features.features.depthBiasClamp) continue;

            if (!features.features.depthBounds) continue;

            if (props.properties.deviceType != static_cast<VkPhysicalDeviceType>(device_preference)) continue;

            physical_device = device;
            break;
        }

        if (!physical_device)
        {
            return std::unexpected(Error::eGPUSelectionFailed);
        }

        return PhysicalDevice{ physical_device, device_features };
    }

    std::expected<VkDevice, Error> CreateDevice(VkPhysicalDevice gpu)
    {
        VkDevice device = nullptr;

        const auto index = GetGraphicsQueueFamilyIndex(gpu);

        float priority = 1.0f;
        VkDeviceQueueCreateInfo queue_create_info{
            .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
            .queueFamilyIndex = index.value(),
            .queueCount = 1,
            .pQueuePriorities = &priority,
        };

        std::array extensions{
            VK_KHR_SWAPCHAIN_EXTENSION_NAME,
        };

        VkPhysicalDeviceFeatures features{
            .geometryShader = true,
            .tessellationShader = true,
            .logicOp = true,
            // .multiDrawIndirect = ,
            // .drawIndirectFirstInstance = ,
            .depthClamp = true,
            .depthBiasClamp = true,
            .fillModeNonSolid = true,
            .depthBounds = true,
            .textureCompressionBC = true,
            .vertexPipelineStoresAndAtomics = true,
            .fragmentStoresAndAtomics = true,
            // .wideLines = ,
            // .multiViewport = ,
            // .samplerAnisotropy = ,
            .shaderInt64 = true,
        };

        VkPhysicalDeviceFeatures2 features2{
            .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2,
            .features = features,
        };

        VkPhysicalDeviceVulkan11Features features11{
            .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES,
            .pNext = &features2,
            .multiview = true,
            .shaderDrawParameters = true,
        };

        VkPhysicalDeviceVulkan12Features features12{
            .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES,
            .pNext = &features11,
            .drawIndirectCount = true,
            .uniformAndStorageBuffer8BitAccess = true,
            .descriptorIndexing = true,
            .shaderUniformBufferArrayNonUniformIndexing = true,
            .shaderSampledImageArrayNonUniformIndexing = true,
            .descriptorBindingUniformBufferUpdateAfterBind = true,
            .descriptorBindingSampledImageUpdateAfterBind = true,
            .descriptorBindingStorageBufferUpdateAfterBind = true,
            .descriptorBindingUpdateUnusedWhilePending = true,
            .descriptorBindingPartiallyBound = true,
            .descriptorBindingVariableDescriptorCount = true,
            .runtimeDescriptorArray = true,
            .timelineSemaphore = true,
            .bufferDeviceAddress = true,
            .vulkanMemoryModel = true,
            .vulkanMemoryModelDeviceScope = true,
        };

        VkPhysicalDeviceVulkan13Features features13{ .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES,
                                                     .pNext = &features12,
                                                     .synchronization2 = true,
                                                     .dynamicRendering = true,
                                                     .maintenance4 = true };

        const VkDeviceCreateInfo create_info{
            .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
            .pNext = &features13,
            .queueCreateInfoCount = 1,
            .pQueueCreateInfos = &queue_create_info,
            .enabledExtensionCount = extensions.size(),
            .ppEnabledExtensionNames = extensions.data(),
        };
        const auto result = vkCreateDevice(gpu, &create_info, nullptr, &device);
        return HandleResult(result, Error::eDeviceCreateFailed, device);
    }

    VkPresentModeKHR
    GetPresentMode(VkPhysicalDevice physical_device, VkSurfaceKHR surface, VkPresentModeKHR preferred_present_mode)
    {
        uint count = 0;
        vkGetPhysicalDeviceSurfacePresentModesKHR(physical_device, surface, &count, nullptr);

        std::vector<VkPresentModeKHR> present_modes(count);
        const auto it = std::ranges::find_if(
            present_modes,
            [&](VkPresentModeKHR present_mode)
            {
                return present_mode == preferred_present_mode;
            });
        if (it == present_modes.end()) return VK_PRESENT_MODE_FIFO_KHR;
        return preferred_present_mode;
    }

    std::expected<VkSurfaceKHR, Error> CreateSurfaceFromNativeHandle(VkInstance instance, const WindowData& window_data)
    {
        VkSurfaceKHR surface = nullptr;
        const VkXlibSurfaceCreateInfoKHR createInfo{ .sType = VK_STRUCTURE_TYPE_XLIB_SURFACE_CREATE_INFO_KHR,
                                                     .dpy = window_data.DisplayHandle,
                                                     .window = window_data.WindowHandle };
        const auto result = vkCreateXlibSurfaceKHR(instance, &createInfo, nullptr, &surface);
        return HandleResult(result, Error::eSurfaceCreateFailed, surface);
    }

    std::expected<VkSurfaceFormatKHR, Error>
    GetSurfaceFormat(VkPhysicalDevice physical_device, VkSurfaceKHR surface, VkSurfaceFormatKHR preferred_format)
    {
        uint format_count = 0;
        vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device, surface, &format_count, nullptr);
        std::vector<VkSurfaceFormatKHR> surface_formats(format_count);
        vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device, surface, &format_count, surface_formats.data());
        const auto it = std::ranges::find_if(
            surface_formats,
            [&](VkSurfaceFormatKHR format)
            {
                return format.format == preferred_format.format && format.colorSpace == preferred_format.colorSpace;
            });
        if (it == surface_formats.end()) return surface_formats.back();
        return preferred_format;
    }

    struct SwapchainData
    {
        VkSwapchainKHR Swapchain = nullptr;
        VkFormat Format = VK_FORMAT_UNDEFINED;
    };

    std::expected<SwapchainData, Error> CreateSwapchain(
        VkPhysicalDevice physical_device,
        VkSurfaceKHR surface,
        VkDevice device,
        const ContextCreateInfo& create_info)
    {
        VkSwapchainKHR swapchain = nullptr;
        const auto index = GetGraphicsQueueFamilyIndex(physical_device);

        VkSurfaceCapabilitiesKHR surface_capabilities{};
        vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physical_device, surface, &surface_capabilities);
        constexpr VkSurfaceFormatKHR preferred_format{
            .format = VK_FORMAT_B8G8R8A8_UNORM,
            .colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR,
        };
        const auto [format, color_space] = GetSurfaceFormat(physical_device, surface, preferred_format).value();
        const VkFormat swapchainFormat = format;
        const VkSwapchainCreateInfoKHR swapchain_create_info{
            .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
            .surface = surface,
            .minImageCount = 3,
            .imageFormat = format,
            .imageColorSpace = color_space,
            .imageExtent = VkExtent2D(create_info.Width, create_info.Height),
            .imageArrayLayers = 1,
            .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
            .imageSharingMode = VK_SHARING_MODE_EXCLUSIVE,
            .queueFamilyIndexCount = 1,
            .pQueueFamilyIndices = &index.value(),
            .preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR,
            .compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
            .presentMode = GetPresentMode(physical_device, surface, VK_PRESENT_MODE_MAILBOX_KHR),
            .clipped = false,
            .oldSwapchain = nullptr,
        };
        const auto result = vkCreateSwapchainKHR(device, &swapchain_create_info, nullptr, &swapchain);
        const auto swapchainData = SwapchainData{ swapchain, swapchainFormat };
        return HandleResult(result, Error::eSwapchainCreateFailed, swapchainData);
    }

    std::expected<VmaAllocator, Error>
    CreateAllocator(VkInstance instance, VkDevice device, VkPhysicalDevice physical_device)
    {
        const VmaAllocatorCreateInfo create_info{
            .flags = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT,
            .physicalDevice = physical_device,
            .device = device,
            .instance = instance,
            .vulkanApiVersion = VK_API_VERSION_1_3,
        };
        VmaAllocator allocator = nullptr;
        const auto result = vmaCreateAllocator(&create_info, &allocator);
        return HandleResult(result, Error::eAllocatorCreateFailed, allocator);
    }

    std::expected<VkShaderModule, Error> CreateShaderModule(VkDevice device, const std::span<const char> code)
    {
        const VkShaderModuleCreateInfo create_info{
            .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
            .codeSize = code.size(),
            .pCode = reinterpret_cast<const uint32_t*>(code.data()),
        };
        VkShaderModule shader_module = nullptr;
        const auto result = vkCreateShaderModule(device, &create_info, nullptr, &shader_module);
        return HandleResult(result, Error::eShaderCreateFailed, shader_module);
    }

    std::expected<VkPipelineShaderStageCreateInfo, Error> CreateShaderStage(
        VkDevice device,
        const VkShaderStageFlagBits shader_stage,
        const std::span<const char> code,
        std::string_view entry_point)
    {
        const auto exp_shader_module = CreateShaderModule(device, code);
        if (!exp_shader_module)
        {
            return std::unexpected(exp_shader_module.error());
        }
        VkPipelineShaderStageCreateInfo create_info{
            .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            .stage = shader_stage,
            .module = exp_shader_module.value(),
            .pName = entry_point.data(),
        };
        return create_info;
    }

    std::expected<VkDescriptorSetLayout, Error> CreateDescriptorSetLayout(
        VkDevice device,
        std::span<VkDescriptorSetLayoutBinding> bindings)
    {
        const VkDescriptorSetLayoutCreateInfo create_info{
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
            .bindingCount = static_cast<uint32_t>(bindings.size()),
            .pBindings = bindings.data(),
        };
        VkDescriptorSetLayout layout = nullptr;
        const auto result = vkCreateDescriptorSetLayout(device, &create_info, nullptr, &layout);
        return HandleResult(result, Error::eDescriptorLayoutCreateFailed, layout);
    }
}  // namespace Swift::Vulkan

std::expected<Swift::Vulkan::Context, Swift::Error> Swift::Vulkan::CreateContext(const ContextCreateInfo& create_info)
{
    Context context{};
    const auto exp_instance = CreateInstance();
    if (!exp_instance)
    {
        return std::unexpected(exp_instance.error());
    }
    context.Instance = exp_instance.value();

    const auto exp_gpu = ChoosePhysicalDevice(context.Instance, create_info.Preference);
    if (!exp_gpu)
    {
        return std::unexpected(exp_gpu.error());
    }
    context.GPU = exp_gpu.value();

    const auto exp_device = CreateDevice(context.GPU.GPU);
    if (!exp_device)
    {
        return std::unexpected(exp_device.error());
    }
    context.Device = exp_device.value();

    const auto exp_allocator = CreateAllocator(context.Instance, context.Device, context.GPU.GPU);
    if (!exp_allocator)
    {
        return std::unexpected(exp_allocator.error());
    }
    context.Allocator = exp_allocator.value();

    const auto exp_surface_handle = CreateSurfaceFromNativeHandle(context.Instance, create_info.Window);
    if (!exp_surface_handle)
    {
        return std::unexpected(Error::eSurfaceCreateFailed);
    }
    context.Surface = exp_surface_handle.value();

    const auto exp_swapchain = CreateSwapchain(context.GPU.GPU, context.Surface, context.Device, create_info);
    if (!exp_swapchain)
    {
        return std::unexpected(exp_swapchain.error());
    }
    context.Swapchain = exp_swapchain.value().Swapchain;
    context.SwapchainFormat = exp_swapchain.value().Format;

    return context;
}

std::expected<Swift::Vulkan::Queue, Swift::Error> Swift::Vulkan::CreateQueue(
    const Context& context,
    const QueueType queue_type)
{
    uint family_index = 0;
    switch (queue_type)
    {
        case QueueType::eGraphics: family_index = GetGraphicsQueueFamilyIndex(context.GPU.GPU).value(); break;
        case QueueType::eCompute:;
        case QueueType::eTransfer: break;
    }

    const VkDeviceQueueInfo2 queue_info{
        .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_INFO_2,
        .queueFamilyIndex = family_index,
        .queueIndex = 0,
    };
    Queue queue{
        .QueueFamily = family_index,
    };
    vkGetDeviceQueue2(context.Device, &queue_info, &queue.BaseQueue);
    return queue;
}

std::expected<Swift::Vulkan::Command, Swift::Error> Swift::Vulkan::CreateCommand(const Context& context, const Queue queue)
{
    const VkCommandPoolCreateInfo create_info{
        .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .queueFamilyIndex = queue.QueueFamily,
    };
    Command command{};
    if (vkCreateCommandPool(context.Device, &create_info, nullptr, &command.CommandPool) != VK_SUCCESS)
    {
        return std::unexpected(Error::eCommandPoolCreateFailed);
    }
    const VkCommandBufferAllocateInfo allocate_info{
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .commandPool = command.CommandPool,
        .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = 1,
    };
    const auto result = vkAllocateCommandBuffers(context.Device, &allocate_info, &command.CommandBuffer);
    return HandleResult(result, Error::eCommandBufferCreateFailed, command);
}

std::expected<std::vector<Swift::Vulkan::Texture>, Swift::Error> Swift::Vulkan::GetSwapchainTextures(const Context& context)
{
    uint imageCount = 0;
    vkGetSwapchainImagesKHR(context.Device, context.Swapchain, &imageCount, nullptr);

    std::vector<VkImage> images(imageCount);
    vkGetSwapchainImagesKHR(context.Device, context.Swapchain, &imageCount, images.data());

    std::vector<Texture> textures(imageCount);

    for (const auto& [index, texture] : std::views::enumerate(textures))
    {
        texture.Image = images[index];
        VkImageViewCreateInfo view_create_info{
            .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            .image = texture.Image,
            .viewType = VK_IMAGE_VIEW_TYPE_2D,
            .format = context.SwapchainFormat,
            .components = {
                .r = VK_COMPONENT_SWIZZLE_IDENTITY,
                .g = VK_COMPONENT_SWIZZLE_IDENTITY,
                .b = VK_COMPONENT_SWIZZLE_IDENTITY,
                .a = VK_COMPONENT_SWIZZLE_IDENTITY, },
            .subresourceRange = {
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .baseMipLevel = 0,
                .levelCount = 1,
                .baseArrayLayer = 0,
                .layerCount = 1, },
        };
        if (vkCreateImageView(context.Device, &view_create_info, nullptr, &texture.View) != VK_SUCCESS)
        {
            return std::unexpected(Error::eImageViewCreateFailed);
        }
    }

    return textures;
}

std::expected<VkFence, Swift::Error> Swift::Vulkan::CreateFence(const Context& context)
{
    VkFence fence = nullptr;
    constexpr VkFenceCreateInfo create_info{
        .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
        .flags = VK_FENCE_CREATE_SIGNALED_BIT,
    };
    const auto result = vkCreateFence(context.Device, &create_info, nullptr, &fence);
    return HandleResult(result, Error::eFenceCreateFailed, fence);
}

std::expected<VkSemaphore, Swift::Error> Swift::Vulkan::CreateSemaphore(const Context& context, const bool timeline)
{
    VkSemaphore semaphore = nullptr;
    VkSemaphoreCreateInfo create_info{
        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
    };
    constexpr VkSemaphoreTypeCreateInfo type_create_info{
        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_TYPE_CREATE_INFO,
        .semaphoreType = VK_SEMAPHORE_TYPE_TIMELINE,
        .initialValue = 0,
    };
    if (timeline)
    {
        create_info.pNext = &type_create_info;
    }
    const auto result = vkCreateSemaphore(context.Device, &create_info, nullptr, &semaphore);
    return HandleResult(result, Error::eSemaphoreCreateFailed, semaphore);
}

std::expected<Swift::Vulkan::ShaderLayout, Swift::Error> Swift::Vulkan::CreateShaderLayout(
    const Context& context,
    const ShaderLayoutCreateInfo& create_info)
{
    std::vector<VkDescriptorSetLayoutBinding> layout_bindings(create_info.LayoutBindings.size());
    std::ranges::transform(
        create_info.LayoutBindings,
        layout_bindings.begin(),
        [&](const LayoutBinding& binding)
        {
            return VkDescriptorSetLayoutBinding{
                .binding = binding.Binding,
                .descriptorType = static_cast<VkDescriptorType>(binding.Type),
                .descriptorCount = binding.Count,
                .stageFlags = VK_SHADER_STAGE_ALL,
                .pImmutableSamplers = nullptr,
            };
        });
    const auto exp_set_layout = CreateDescriptorSetLayout(context.Device, layout_bindings);
    if (!exp_set_layout)
    {
        return std::unexpected(exp_set_layout.error());
    }
    VkPushConstantRange pc_range{
        .stageFlags = VK_SHADER_STAGE_ALL,
        .offset = 0,
        .size = 128,
    };
    const VkPipelineLayoutCreateInfo layout_info{ .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
                                                  .setLayoutCount = 1,
                                                  .pSetLayouts = &exp_set_layout.value(),
                                                  .pushConstantRangeCount = 1,
                                                  .pPushConstantRanges = &pc_range };
    VkPipelineLayout layout = nullptr;
    const auto result = vkCreatePipelineLayout(context.Device, &layout_info, nullptr, &layout);
    ShaderLayout shader_layout{ layout, exp_set_layout.value() };
    return HandleResult(result, Error::ePipelineLayoutCreateFailed, shader_layout);
}

std::expected<Swift::Vulkan::Shader, Swift::Error> Swift::Vulkan::CreateGraphicsShader(
    const Context& context,
    const GraphicsShaderCreateInfo& shader_info,
    ShaderLayout shader_layout)
{
    VkPipeline pipeline = nullptr;

    std::vector<VkFormat> color_formats(shader_info.ColorFormats.size());
    std::ranges::transform(
        shader_info.ColorFormats,
        color_formats.begin(),
        [&](const Format& format)
        {
            return static_cast<VkFormat>(format);
        });

    VkPipelineRenderingCreateInfo render_info{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO,
        .colorAttachmentCount = static_cast<uint32_t>(shader_info.ColorFormats.size()),
        .pColorAttachmentFormats = color_formats.data(),
        .depthAttachmentFormat = static_cast<VkFormat>(shader_info.DepthFormat),
    };

    if (shader_info.VertexCode.empty() || shader_info.FragmentCode.empty())
        return std::unexpected(Error::eShaderCreateFailed);

    const auto exp_vertex_stage =
        CreateShaderStage(context.Device, VK_SHADER_STAGE_VERTEX_BIT, shader_info.VertexCode, shader_info.VertexEntryPoint);

    if (!exp_vertex_stage)
    {
        return std::unexpected(exp_vertex_stage.error());
    }

    const auto exp_fragment_stage = CreateShaderStage(
        context.Device, VK_SHADER_STAGE_FRAGMENT_BIT, shader_info.FragmentCode, shader_info.FragmentEntryPoint);

    if (!exp_fragment_stage)
    {
        return std::unexpected(exp_fragment_stage.error());
    }

    std::vector<VkVertexInputBindingDescription> input_bindings(shader_info.VertexBindings.size());
    std::ranges::transform(
        shader_info.VertexBindings,
        input_bindings.begin(),
        [](const VertexBinding& binding)
        {
            return VkVertexInputBindingDescription{
                .binding = binding.Binding,
                .stride = binding.Stride,
                .inputRate = VK_VERTEX_INPUT_RATE_VERTEX,
            };
        });

    std::vector<VkVertexInputAttributeDescription> input_attributes(shader_info.VertexAttributes.size());
    std::ranges::transform(
        shader_info.VertexAttributes,
        input_attributes.begin(),
        [&](const VertexAttribute& attribute)
        {
            return VkVertexInputAttributeDescription{
                .location = attribute.Location,
                .binding = attribute.Binding,
                .format = static_cast<VkFormat>(attribute.AttributeFormat),
                .offset = attribute.Offset,
            };
        });

    VkPipelineVertexInputStateCreateInfo vertex_input_info{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
        .vertexBindingDescriptionCount = static_cast<uint32_t>(input_bindings.size()),
        .pVertexBindingDescriptions = input_bindings.data(),
        .vertexAttributeDescriptionCount = static_cast<uint32_t>(input_attributes.size()),
        .pVertexAttributeDescriptions = input_attributes.data(),
    };

    VkPipelineInputAssemblyStateCreateInfo input_assembly_info{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
        .topology = static_cast<VkPrimitiveTopology>(shader_info.Topology),
        .primitiveRestartEnable = false,
    };

    constexpr VkPipelineRasterizationStateCreateInfo rasterization_info{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
        .depthClampEnable = true,
        .rasterizerDiscardEnable = false,
        .polygonMode = VK_POLYGON_MODE_FILL,
        .cullMode = VK_CULL_MODE_BACK_BIT,
        .frontFace = VK_FRONT_FACE_CLOCKWISE,
        .depthBiasEnable = false,
        .depthBiasConstantFactor = 0.f,
        .depthBiasClamp = 1.f,
        .depthBiasSlopeFactor = 0,
        .lineWidth = 1.f,
    };

    VkViewport viewport{
        .x = 1280.f,
        .y = 720.f,
    };
    VkRect2D scissor{
        .extent = { 1280, 720 },
    };

    VkPipelineViewportStateCreateInfo viewport_info{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
        .viewportCount = 1,
        .pViewports = &viewport,
        .scissorCount = 1,
        .pScissors = &scissor,
    };

    constexpr std::array dynamic_states{
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR,
        VK_DYNAMIC_STATE_CULL_MODE,
        VK_DYNAMIC_STATE_FRONT_FACE,
        VK_DYNAMIC_STATE_DEPTH_BIAS,
        VK_DYNAMIC_STATE_DEPTH_BIAS_ENABLE,
        VK_DYNAMIC_STATE_DEPTH_TEST_ENABLE,
        VK_DYNAMIC_STATE_STENCIL_TEST_ENABLE,
        VK_DYNAMIC_STATE_DEPTH_BOUNDS,
        VK_DYNAMIC_STATE_DEPTH_BOUNDS_TEST_ENABLE,
        VK_DYNAMIC_STATE_DEPTH_COMPARE_OP,
        VK_DYNAMIC_STATE_DEPTH_WRITE_ENABLE,
        VK_DYNAMIC_STATE_RASTERIZER_DISCARD_ENABLE,
    };

    VkPipelineDynamicStateCreateInfo dynamic_state_info{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
        .dynamicStateCount = static_cast<uint32_t>(dynamic_states.size()),
        .pDynamicStates = dynamic_states.data(),
    };

    std::vector<VkPipelineColorBlendAttachmentState> color_blend_attachments(
        shader_info.BlendStateDesc.BlendAttachments.size());
    std::ranges::transform(
        shader_info.BlendStateDesc.BlendAttachments,
        color_blend_attachments.begin(),
        [](const BlendAttachment& blend_attachment)
        {
            return VkPipelineColorBlendAttachmentState{
                .blendEnable = blend_attachment.BlendEnable,
                .srcColorBlendFactor = static_cast<VkBlendFactor>(blend_attachment.SrcColorBlendFactor),
                .dstColorBlendFactor = static_cast<VkBlendFactor>(blend_attachment.DstColorBlendFactor),
                .colorBlendOp = static_cast<VkBlendOp>(blend_attachment.ColorBlendOp),
                .srcAlphaBlendFactor = static_cast<VkBlendFactor>(blend_attachment.SrcAlphaBlendFactor),
                .dstAlphaBlendFactor = static_cast<VkBlendFactor>(blend_attachment.DstAlphaBlendFactor),
                .alphaBlendOp = static_cast<VkBlendOp>(blend_attachment.AlphaBlendOp),
                .colorWriteMask = static_cast<VkColorComponentFlags>(blend_attachment.ColorBlendMask),
            };
        });

    const VkPipelineColorBlendStateCreateInfo color_blend_state{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
        .logicOpEnable = shader_info.BlendStateDesc.LogicOpEnable,
        .logicOp = static_cast<VkLogicOp>(shader_info.BlendStateDesc.Logic),
        .attachmentCount = static_cast<uint32_t>(color_blend_attachments.size()),
        .pAttachments = color_blend_attachments.data(),
        .blendConstants = {
            shader_info.BlendStateDesc.BlendConstants[0],
            shader_info.BlendStateDesc.BlendConstants[1],
            shader_info.BlendStateDesc.BlendConstants[2],
            shader_info.BlendStateDesc.BlendConstants[3],
        },
    };

    VkPipelineMultisampleStateCreateInfo multisample_info{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
        .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
        .sampleShadingEnable = false,
        .minSampleShading = 1.f,
        .pSampleMask = nullptr,
        .alphaToCoverageEnable = false,
        .alphaToOneEnable = false,
    };

    std::array stages{ exp_vertex_stage.value(), exp_fragment_stage.value() };

    //  TODO: Support Pipeline caching
    const VkGraphicsPipelineCreateInfo create_info{
        .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
        .pNext = &render_info,
        .stageCount = 2,
        .pStages = stages.data(),
        .pVertexInputState = &vertex_input_info,
        .pInputAssemblyState = &input_assembly_info,
        .pViewportState = &viewport_info,
        .pRasterizationState = &rasterization_info,
        .pMultisampleState = &multisample_info,
        .pDepthStencilState = nullptr,
        .pColorBlendState = &color_blend_state,
        .pDynamicState = &dynamic_state_info,
        .layout = shader_layout.PipelineLayout,
    };

    const auto result = vkCreateGraphicsPipelines(context.Device, nullptr, 1, &create_info, nullptr, &pipeline);

    vkDestroyShaderModule(context.Device, exp_vertex_stage->module, nullptr);
    vkDestroyShaderModule(context.Device, exp_fragment_stage->module, nullptr);

    Shader shader{
        .Pipeline = pipeline,
        .BindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
    };

    return HandleResult(result, Error::eGraphicsPipelineCreateFailed, shader);
}

std::expected<Swift::Vulkan::Buffer, Swift::Error> Swift::Vulkan::CreateBuffer(
    const Context& context,
    BufferCreateInfo create_info)
{
    Buffer buffer{};
    const VkBufferCreateInfo buffer_info{
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .size = create_info.Size,
        .usage = static_cast<VkBufferUsageFlags>(create_info.Usage),
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
    };
    constexpr VmaAllocationCreateInfo vma_info{
        .flags = VMA_ALLOCATION_CREATE_MAPPED_BIT,
        .usage = VMA_MEMORY_USAGE_UNKNOWN,
        .requiredFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        .preferredFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
    };
    buffer.Size = create_info.Size;
    VkPhysicalDeviceProperties properties{};
    vkGetPhysicalDeviceProperties(context.GPU.GPU, &properties);
    const auto result = vmaCreateBufferWithAlignment(
        context.Allocator,
        &buffer_info,
        &vma_info,
        properties.limits.minUniformBufferOffsetAlignment,
        &buffer.BaseBuffer,
        &buffer.Allocation,
        nullptr);

    if (result != VK_SUCCESS)
    {
        return std::unexpected(Error::eBufferCreateFailed);
    }

    void* data;
    if (vmaMapMemory(context.Allocator, buffer.Allocation, &data) != VK_SUCCESS)
    {
        return std::unexpected(Error::eBufferMapFailed);
    }
    
    memcpy(data, create_info.InitialData, create_info.Size);

    vmaUnmapMemory(context.Allocator, buffer.Allocation);

    return buffer;
}

void Swift::Vulkan::DestroyCommand(const Context& context, const Command& command)
{
    vkDestroyCommandPool(context.Device, command.CommandPool, nullptr);
}

void Swift::Vulkan::DestroyTexture(const Context& context, const Texture& texture, bool only_view)
{
    vkDestroyImageView(context.Device, texture.View, nullptr);
    if (only_view) return;

    if (texture.Allocation)
    {
        vmaDestroyImage(context.Allocator, texture.Image, texture.Allocation);
    }
    else
    {
        vkDestroyImage(context.Device, texture.Image, nullptr);
    }
}

void Swift::Vulkan::DestroyBuffer(const Context& context, const Buffer& buffer)
{
    vmaDestroyBuffer(context.Allocator, buffer.BaseBuffer, buffer.Allocation);
}

void Swift::Vulkan::DestroySemaphore(const Context& context, VkSemaphore semaphore)
{
    vkDestroySemaphore(context.Device, semaphore, nullptr);
}

void Swift::Vulkan::DestroyFence(const Context& context, VkFence fence) { vkDestroyFence(context.Device, fence, nullptr); }

void Swift::Vulkan::DestroyContext(const Context& context)
{
    vmaDestroyAllocator(context.Allocator);
    vkDestroySwapchainKHR(context.Device, context.Swapchain, nullptr);
    vkDestroySurfaceKHR(context.Instance, context.Surface, nullptr);
    vkDestroyDevice(context.Device, nullptr);
    vkDestroyInstance(context.Instance, nullptr);
}

void Swift::Vulkan::DestroyShaderLayout(const Context& context, ShaderLayout layout)
{
    vkDestroyPipelineLayout(context.Device, layout.PipelineLayout, nullptr);
    vkDestroyDescriptorSetLayout(context.Device, layout.DescriptorSetLayout, nullptr);
}

void Swift::Vulkan::DestroyShader(const Context& context, const Shader& shader)
{
    vkDestroyPipeline(context.Device, shader.Pipeline, nullptr);
}