#include "Swift.hpp"
#include "Vulkan/VulkanInit.hpp"
#include "Vulkan/VulkanRender.hpp"
#include "Vulkan/VulkanStructs.hpp"
#include "Vulkan/VulkanUtil.hpp"

#define VMA_IMPLEMENTATION
#include "vk_mem_alloc.h"

namespace
{
    using namespace Swift;
    Vulkan::Context gContext;

    Vulkan::Queue gGraphicsQueue;
    Vulkan::Queue gComputeQueue;
    Vulkan::Queue gTransferQueue;

    Vulkan::Command gTransferCommand;
    vk::Fence gTransferFence;

    Vulkan::Command gGraphicsCommand; // For non render loop operations
    vk::Fence gGraphicsFence;         // For non render loop operations

    Vulkan::Swapchain gSwapchain;
    Vulkan::BindlessDescriptor gDescriptor;
    // TODO: sampler pool for all types of samplers
    vk::Sampler gLinearSampler;

    std::vector<Vulkan::Thread> gThreadDatas;
    std::vector<Vulkan::Buffer> gTransferStagingBuffers;
    std::vector<Vulkan::Buffer> gBuffers;
    std::vector<Vulkan::Shader> gShaders;
    u32 gCurrentShader = 0;
    std::vector<Vulkan::Image> gWriteableImages;
    std::vector<Vulkan::Image> gSamplerImages;
    std::vector<Vulkan::Image> gTemporaryImages;

    std::vector<Vulkan::FrameData> gFrameData;
    u32 gCurrentFrame = 0;
    Vulkan::FrameData gCurrentFrameData;

    InitInfo gInitInfo;

    u32 PackImageType(
        const u32 value,
        const ImageUsage type)
    {
        return (value << 8) | (static_cast<u32>(type) & 0xFF);
    }

    ImageUsage GetImageType(const u32 value)
    {
        return static_cast<ImageUsage>(value & 0xFF);
    }

    u32 GetImageIndex(const u32 value)
    {
        return (value >> 8) & 0xFFFFFF;
    }

    Vulkan::Image& GetRealImage(const u32 imageHandle)
    {
        const auto index = GetImageIndex(imageHandle);
        switch (GetImageType(imageHandle))
        {
        case ImageUsage::eSampledReadWrite:
        case ImageUsage::eReadWrite:
            return gWriteableImages[index];
        case ImageUsage::eSampled:
            return gSamplerImages[index];
        case ImageUsage::eTemporary:
            return gTemporaryImages[index];
        }
        return gSamplerImages[index];
    }
} // namespace

using namespace Vulkan;

void Swift::Init(const InitInfo& initInfo)
{
    gInitInfo = initInfo;

    gContext = Init::CreateContext(initInfo);

    const auto indices = Util::GetQueueFamilyIndices(gContext.gpu, gContext.surface);
    const auto graphicsQueue = Init::GetQueue(gContext, indices[0], 0, "Graphics Queue");
    gGraphicsQueue.SetIndex(indices[0]).SetQueue(graphicsQueue);
    if (indices.size() > 1)
    {
        const auto computeQueue = Init::GetQueue(gContext, indices[1], 0, "Compute Queue");
        gComputeQueue.SetIndex(indices[1]).SetQueue(computeQueue);
        const auto transferQueue = Init::GetQueue(gContext, indices[2], 0, "Transfer Queue");
        gTransferQueue.SetIndex(indices[2]).SetQueue(transferQueue);
    }
    else
    {
        const auto computeQueue = Init::GetQueue(gContext, indices[0], 0, "Compute Queue");
        gComputeQueue.SetIndex(indices[1]).SetQueue(computeQueue);
        const auto transferQueue = Init::GetQueue(gContext, indices[0], 0, "Transfer Queue");
        gTransferQueue.SetIndex(indices[2]).SetQueue(transferQueue);
    }

    constexpr auto depthFormat = vk::Format::eD32Sfloat;
    const auto depthImage = Init::CreateImage(
        gContext,
        vk::ImageType::e2D,
        Util::To3D(initInfo.extent),
        depthFormat,
        vk::ImageUsageFlagBits::eDepthStencilAttachment,
        1,
        {},
        "Swapchain Depth");

    const auto swapchain =
        Init::CreateSwapchain(gContext, Util::To2D(initInfo.extent), gGraphicsQueue.index);
    gSwapchain.SetSwapchain(swapchain)
        .SetDepthImage(depthImage)
        .SetImages(Init::CreateSwapchainImages(gContext, gSwapchain))
        .SetIndex(0)
        .SetExtent(Util::To2D(initInfo.extent));

    gFrameData.resize(gSwapchain.images.size());
    for (auto& [renderSemaphore, presentSemaphore, renderFence, renderCommand] : gFrameData)
    {
        renderFence =
            Init::CreateFence(gContext, vk::FenceCreateFlagBits::eSignaled, "Render Fence");
        renderSemaphore = Init::CreateSemaphore(gContext, "Render Semaphore");
        presentSemaphore = Init::CreateSemaphore(gContext, "Present Semaphore");
        renderCommand.commandPool =
            Init::CreateCommandPool(gContext, gGraphicsQueue.index, "Command Pool");
        renderCommand.commandBuffer =
            Init::CreateCommandBuffer(gContext, renderCommand.commandPool, "Command Buffer");
    }

    gDescriptor.SetDescriptorSetLayout(Init::CreateDescriptorSetLayout(gContext))
        .SetDescriptorPool(Init::CreateDescriptorPool(gContext, {}))
        .SetDescriptorSet(
            Init::CreateDescriptorSet(gContext, gDescriptor.pool, gDescriptor.setLayout));

    gLinearSampler = Init::CreateSampler(gContext);

    gTransferCommand.commandPool =
        Init::CreateCommandPool(gContext, gTransferQueue.index, "Transfer Command Pool");
    gTransferCommand.commandBuffer = Init::CreateCommandBuffer(
        gContext,
        gTransferCommand.commandPool,
        "Transfer Command Buffer");
    gTransferFence = Init::CreateFence(gContext, {}, "Transfer Fence");

    gGraphicsCommand.commandPool =
        Init::CreateCommandPool(gContext, gGraphicsQueue.index, "Graphics Command Pool");
    gGraphicsCommand.commandBuffer = Init::CreateCommandBuffer(
        gContext,
        gGraphicsCommand.commandPool,
        "Graphics Command Buffer");
    gGraphicsFence = Init::CreateFence(gContext, {}, "Graphics Fence");
}

void Swift::Shutdown()
{
    [[maybe_unused]]
    const auto result = gContext.device.waitIdle();
    VK_ASSERT(result, "Failed to wait for device while cleaning up");

    for (auto& frameData : gFrameData)
    {
        frameData.Destroy(gContext);
    }
    gDescriptor.Destroy(gContext);

    for (const auto& shader : gShaders)
    {
        shader.Destroy(gContext);
    }

    for (auto& image : gWriteableImages)
    {
        image.Destroy(gContext);
    }

    for (auto& image : gSamplerImages)
    {
        image.Destroy(gContext);
    }

    for (auto& image : gTemporaryImages)
    {
        image.Destroy(gContext);
    }

    for (auto& buffer : gBuffers)
    {
        buffer.Destroy(gContext);
    }

    gTransferCommand.Destroy(gContext);
    gGraphicsCommand.Destroy(gContext);
    gContext.device.destroy(gTransferFence);
    gContext.device.destroy(gGraphicsFence);
    gContext.device.destroy(gLinearSampler);

    gSwapchain.Destroy(gContext);
    gContext.Destroy();
}

Swift::InitInfo Swift::GetInitInfo()
{
    return gInitInfo;
}

Swift::Vulkan::Context Swift::GetContext()
{
    return gContext;
}

Vulkan::Queue Swift::GetGraphicsQueue()
{
    return gGraphicsQueue;
}

Vulkan::Queue Swift::GetTransferQueue()
{
    return gTransferQueue;
}

Vulkan::Queue Swift::GetComputeQueue()
{
    return gComputeQueue;
}
Vulkan::Command Swift::GetGraphicsCommand()
{
    return gCurrentFrameData.renderCommand;
}

vk::Sampler Swift::GetDefaultSampler()
{
    return gLinearSampler;
}

void Swift::WaitIdle()
{
    [[maybe_unused]]
    const auto result = gContext.device.waitIdle();
    VK_ASSERT(result, "Failed to wait for device while cleaning up");
}

bool Swift::SupportsGraphicsMultithreading()
{
    const auto queueFamilyProps = gContext.gpu.getQueueFamilyProperties();
    return queueFamilyProps.at(gGraphicsQueue.index).queueCount > 1;
}

void Swift::BeginFrame(const DynamicInfo& dynamicInfo)
{
    gCurrentFrameData = gFrameData[gCurrentFrame];
    const auto& commandBuffer = Render::GetCommandBuffer(gCurrentFrameData);
    const auto& renderFence = Render::GetRenderFence(gCurrentFrameData);

    Util::WaitFence(gContext, renderFence, 1000000000);
    gSwapchain.imageIndex = Render::AcquireNextImage(
        gGraphicsQueue,
        gContext,
        gSwapchain,
        gCurrentFrameData.renderSemaphore,
        Util::To2D(dynamicInfo.extent));
    Util::ResetFence(gContext, renderFence);
    Util::BeginOneTimeCommand(commandBuffer);
}

void Swift::EndFrame(const DynamicInfo& dynamicInfo)
{
    const auto& commandBuffer = Render::GetCommandBuffer(gCurrentFrameData);
    const auto& renderSemaphore = Render::GetRenderSemaphore(gCurrentFrameData);
    const auto& presentSemaphore = Render::GetPresentSemaphore(gCurrentFrameData);
    const auto& renderFence = Render::GetRenderFence(gCurrentFrameData);
    auto& swapchainImage = Render::GetSwapchainImage(gSwapchain);

    const auto presentBarrier = Util::ImageBarrier(
        swapchainImage.currentLayout,
        vk::ImageLayout::ePresentSrcKHR,
        swapchainImage,
        vk::ImageAspectFlagBits::eColor);
    Util::PipelineBarrier(commandBuffer, presentBarrier);

    Util::EndCommand(commandBuffer);
    Util::SubmitQueue(
        gGraphicsQueue,
        commandBuffer,
        renderSemaphore,
        vk::PipelineStageFlagBits2::eColorAttachmentOutput,
        presentSemaphore,
        vk::PipelineStageFlagBits2::eAllGraphics,
        renderFence);
    Render::Present(
        gContext,
        gSwapchain,
        gGraphicsQueue,
        presentSemaphore,
        Util::To2D(dynamicInfo.extent));

    gCurrentFrame = (gCurrentFrame + 1) % gSwapchain.images.size();
}

void Swift::BeginRendering()
{
    const auto& commandBuffer = Render::GetCommandBuffer(gCurrentFrameData);
    Render::BeginRendering(commandBuffer, gSwapchain, true);
    Render::SetPipelineDefault(gContext, commandBuffer, gSwapchain.extent, gInitInfo.bUsePipelines);
}

void Swift::BeginRendering(const bool bLoadPreviousData)
{
    const auto& commandBuffer = Render::GetCommandBuffer(gCurrentFrameData);
    Render::BeginRendering(commandBuffer, gSwapchain, true, bLoadPreviousData);
    Render::SetPipelineDefault(gContext, commandBuffer, gSwapchain.extent, gInitInfo.bUsePipelines);
}

void Swift::EndRendering()
{
    const auto& commandBuffer = Render::GetCommandBuffer(gCurrentFrameData);
    Render::EndRendering(commandBuffer);
}

void Swift::BeginRendering(
    const glm::uvec2& extent,
    const std::vector<ImageHandle>& colorImages,
    const ImageHandle& depthImage)
{
    const auto& commandBuffer = Render::GetCommandBuffer(gCurrentFrameData);

    std::vector<Image> realColorImages;
    realColorImages.reserve(colorImages.size());
    for (const auto& imageHandle : colorImages)
    {
        const auto& realImage = GetRealImage(imageHandle);
        realColorImages.emplace_back(realImage);
    }

    Image realDepthImage{};
    bool enableDepth = false;
    if (depthImage != 0 || depthImage != InvalidHandle)
    {
        realDepthImage = GetRealImage(depthImage);
        enableDepth = true;
    }

    Render::BeginRendering(commandBuffer, Util::To2D(extent), realColorImages, realDepthImage, enableDepth);
    Render::SetPipelineDefault(gContext, commandBuffer, gSwapchain.extent, gInitInfo.bUsePipelines);
}

void Swift::SetCullMode(const CullMode& cullMode)
{
    const auto& commandBuffer = Render::GetCommandBuffer(gCurrentFrameData);
    Render::SetCullMode(commandBuffer, static_cast<vk::CullModeFlagBits>(cullMode));
}

void Swift::SetDepthCompareOp(DepthCompareOp depthCompareOp)
{
    const auto& commandBuffer = Render::GetCommandBuffer(gCurrentFrameData);
    Render::SetDepthCompareOp(commandBuffer, static_cast<vk::CompareOp>(depthCompareOp));
}
void Swift::SetPolygonMode(PolygonMode polygonMode)
{
    const auto& commandBuffer = Render::GetCommandBuffer(gCurrentFrameData);
    Render::SetPolygonMode(gContext, commandBuffer, static_cast<vk::PolygonMode>(polygonMode));
}

ShaderHandle Swift::CreateGraphicsShader(
    const std::string_view vertexPath,
    const std::string_view fragmentPath,
    const std::string_view debugName)
{
    const auto shader = Init::CreateGraphicsShader(
        gContext,
        gDescriptor,
        gInitInfo.bUsePipelines,
        128,
        vertexPath,
        fragmentPath,
        debugName);
    gShaders.emplace_back(shader);
    const auto index = static_cast<u32>(gShaders.size() - 1);
    return index;
}

ShaderHandle Swift::CreateComputeShader(
    const std::string& computePath,
    const std::string_view debugName)
{
    const auto shader = Init::CreateComputeShader(
        gContext,
        gDescriptor,
        gInitInfo.bUsePipelines,
        128,
        computePath,
        debugName);
    gShaders.emplace_back(shader);
    const auto index = static_cast<u32>(gShaders.size() - 1);
    return index;
}

void Swift::BindShader(const ShaderHandle& shaderHandle)
{
    const auto& commandBuffer = Render::GetCommandBuffer(gCurrentFrameData);
    const auto& shader = gShaders.at(shaderHandle);

    Render::BindShader(commandBuffer, gContext, gDescriptor, shader, gInitInfo.bUsePipelines);

    gCurrentShader = shaderHandle;
}

void Swift::Draw(
    const u32 vertexCount,
    const u32 instanceCount,
    const u32 firstVertex,
    const u32 firstInstance)
{
    const auto& commandBuffer = Render::GetCommandBuffer(gCurrentFrameData);
    commandBuffer.draw(vertexCount, instanceCount, firstVertex, firstInstance);
}

void Swift::DrawIndexed(
    const u32 indexCount,
    const u32 instanceCount,
    const u32 firstIndex,
    const int vertexOffset,
    const u32 firstInstance)
{
    const auto& commandBuffer = Render::GetCommandBuffer(gCurrentFrameData);
    commandBuffer.drawIndexed(indexCount, instanceCount, firstIndex, vertexOffset, firstInstance);
}

void Swift::DrawIndexedIndirect(
    const BufferHandle& buffer,
    const u64 offset,
    const u32 drawCount,
    const u32 stride)
{
    const auto& commandBuffer = Render::GetCommandBuffer(gCurrentFrameData);
    const auto& realBuffer = gBuffers[buffer];
    commandBuffer.drawIndexedIndirect(realBuffer, offset, drawCount, stride);
}

void Swift::DrawIndexedIndirectCount(
    const BufferHandle& buffer,
    const u64 offset,
    const BufferHandle& countBuffer,
    const u64 countOffset,
    const u32 maxDrawCount,
    const u32 stride)
{
    const auto& commandBuffer = Render::GetCommandBuffer(gCurrentFrameData);
    const auto& realBuffer = gBuffers[buffer];
    const auto& realCountBuffer = gBuffers[countBuffer];
    commandBuffer.drawIndexedIndirectCount(
        realBuffer,
        offset,
        realCountBuffer,
        countOffset,
        maxDrawCount,
        stride);
}

ImageHandle Swift::CreateImage(
    const ImageUsage usage,
    const ImageFormat format,
    const glm::uvec2 size,
    const std::string_view debugName)
{
    auto imageUsage = vk::ImageUsageFlagBits::eTransferSrc |
                                vk::ImageUsageFlagBits::eTransferDst |
                                vk::ImageUsageFlagBits::eStorage | vk::ImageUsageFlagBits::eSampled;
    
    auto vkFormat = vk::Format::eR16G16B16A16Sfloat;
    switch (format)
    {
    case ImageFormat::eR16G16B16A16_SRGB:
        vkFormat = vk::Format::eR16G16B16A16Sfloat;
        imageUsage |= vk::ImageUsageFlagBits::eColorAttachment;
        break;
    case ImageFormat::eR32G32B32A32_SRGB:
        vkFormat = vk::Format::eR32G32B32A32Sfloat;
        imageUsage |= vk::ImageUsageFlagBits::eColorAttachment;
        break;
    case ImageFormat::eD32:
        vkFormat = vk::Format::eD32Sfloat;
        imageUsage |= vk::ImageUsageFlagBits::eDepthStencilAttachment;
        break;
    }
    
    const auto image = Init::CreateImage(
        gContext,
        vk::ImageType::e2D,
        Util::To3D(size),
        vkFormat,
        imageUsage,
        1,
        {},
        debugName);

    u32 arrayElement = 0;
    switch (usage)
    {
    case ImageUsage::eSampledReadWrite:
        gWriteableImages.emplace_back(image);
        arrayElement = static_cast<u32>(gWriteableImages.size() - 1);
        Util::UpdateDescriptorImage(
            gDescriptor.set,
            image.imageView,
            gLinearSampler,
            arrayElement,
            gContext);

        gSamplerImages.emplace_back(image);
        arrayElement = static_cast<u32>(gSamplerImages.size() - 1);
        Util::UpdateDescriptorSampler(
            gDescriptor.set,
            image.imageView,
            gLinearSampler,
            arrayElement,
            gContext);
        return PackImageType(arrayElement, ImageUsage::eSampledReadWrite);

    case ImageUsage::eReadWrite:
        arrayElement = static_cast<u32>(gWriteableImages.size() - 1);
        Util::UpdateDescriptorImage(
            gDescriptor.set,
            image.imageView,
            gLinearSampler,
            arrayElement,
            gContext);
        gWriteableImages.emplace_back(image);
        return PackImageType(arrayElement, ImageUsage::eReadWrite);
    case ImageUsage::eSampled:
        gSamplerImages.emplace_back(image);
        arrayElement = static_cast<u32>(gSamplerImages.size() - 1);
        Util::UpdateDescriptorSampler(
            gDescriptor.set,
            image.imageView,
            gLinearSampler,
            arrayElement,
            gContext);
        return PackImageType(arrayElement, ImageUsage::eSampled);
    case ImageUsage::eTemporary:
        gTemporaryImages.emplace_back(image);
        arrayElement = static_cast<u32>(gTemporaryImages.size() - 1);
        return PackImageType(arrayElement, ImageUsage::eTemporary);
    }
    assert(false);
    return {};
}

ImageHandle Swift::LoadImageFromFile(
    const std::filesystem::path& filePath,
    const int mipLevel,
    const bool loadAllMipMaps,
    const std::string_view debugName,
    const bool tempImage,
    const ThreadHandle thread)
{
    Swift::BeginTransfer(thread);
    const auto image = Swift::LoadImageFromFileQueued(
        filePath,
        mipLevel,
        loadAllMipMaps,
        debugName,
        tempImage,
        thread);
    Swift::EndTransfer(thread);
    return image;
}

ImageHandle Swift::LoadImageFromFileQueued(
    const std::filesystem::path& filePath,
    const int mipLevel,
    const bool loadAllMipMaps,
    const std::string_view debugName,
    const bool tempImage,
    const ThreadHandle thread)
{
    Thread loadThread;
    if (thread != -1)
    {
        loadThread = gThreadDatas[thread];
    }

    const auto transferQueue = loadThread.command.commandBuffer ? loadThread.queue : gTransferQueue;
    const auto transferCommand =
        loadThread.command.commandBuffer ? loadThread.command : gTransferCommand;

    Image image;
    Buffer staging;
    if (filePath.extension() == ".dds")
    {
        std::tie(image, staging) = Init::CreateDDSImage(
            gContext,
            transferQueue,
            transferCommand,
            filePath,
            mipLevel,
            loadAllMipMaps,
            debugName);
    }
    gTransferStagingBuffers.emplace_back(staging);

    u32 arrayElement;
    if (tempImage)
    {
        gTemporaryImages.emplace_back(image);
        arrayElement = static_cast<u32>(gTemporaryImages.size() - 1);
        return PackImageType(arrayElement, ImageUsage::eTemporary);
    }

    gSamplerImages.emplace_back(image);
    arrayElement = static_cast<u32>(gSamplerImages.size() - 1);
    Util::UpdateDescriptorSampler(
        gDescriptor.set,
        gSamplerImages.back().imageView,
        gLinearSampler,
        arrayElement,
        gContext);
    return PackImageType(arrayElement, ImageUsage::eSampled);
}

ImageHandle Swift::LoadCubemapFromFile(
    const std::filesystem::path& filePath,
    const std::string_view debugName)
{
    Swift::BeginTransfer(-1);
    Image image;
    Buffer staging;
    assert(filePath.extension() == ".dds");
    if (filePath.extension() == ".dds")
    {
        std::tie(image, staging) = Init::CreateDDSImage(
            gContext,
            gTransferQueue,
            gTransferCommand,
            filePath,
            0,
            true,
            debugName);
    }
    Swift::EndTransfer(-1);
    staging.Destroy(gContext);

    gSamplerImages.emplace_back(image);
    const auto arrayElement = static_cast<u32>(gSamplerImages.size() - 1);
    Util::UpdateDescriptorSampler(
        gDescriptor.set,
        gSamplerImages.back().imageView,
        gLinearSampler,
        arrayElement,
        gContext);

    return PackImageType(arrayElement, ImageUsage::eSampled);
}

int Swift::GetMinLod(const ImageHandle image)
{
    return GetRealImage(image).minLod;
}

int Swift::GetMaxLod(const ImageHandle image)
{
    return GetRealImage(image).maxLod;
}

u32 Swift::GetImageArrayIndex(const ImageHandle imageHandle)
{
    return GetImageIndex(imageHandle);
}

glm::uvec2 Swift::GetImageSize(ImageHandle imageHandle)
{
    const auto& image = GetRealImage(imageHandle);
    return glm::uvec2(image.extent.width, image.extent.height);
}

std::string_view Swift::GetURI(const ImageHandle imageHandle)
{
    return GetRealImage(imageHandle).uri;
}
vk::ImageView Swift::GetImageView(ImageHandle imageHandle)
{
    const auto& realImage = GetRealImage(imageHandle);
    return realImage.imageView;
}

ImageHandle Swift::ReadOnlyImageFromIndex(const int imageIndex)
{
    return PackImageType(imageIndex, ImageUsage::eSampled);
}

void Swift::UpdateImage(
    const ImageHandle baseImage,
    const ImageHandle tempImage)
{
    auto& realBaseImage = GetRealImage(baseImage);
    realBaseImage.Destroy(gContext);
    const auto& realTempImage = GetRealImage(tempImage);
    realBaseImage = realTempImage;
    Util::UpdateDescriptorSampler(
        gDescriptor.set,
        realBaseImage.imageView,
        gLinearSampler,
        GetImageArrayIndex(baseImage),
        gContext);
}

void Swift::ClearTempImages()
{
    gTemporaryImages.clear();
}

void Swift::DestroyImage(const ImageHandle imageHandle)
{
    auto& realImage = GetRealImage(imageHandle);
    realImage.Destroy(gContext);
}

BufferHandle Swift::CreateBuffer(
    const BufferType bufferType,
    const u32 size,
    const std::string_view debugName)
{
    vk::BufferUsageFlags bufferUsageFlags = vk::BufferUsageFlagBits::eShaderDeviceAddress |
                                            vk::BufferUsageFlagBits::eTransferDst |
                                            vk::BufferUsageFlagBits::eTransferSrc;
    bool readback = false;
    switch (bufferType)
    {
    case BufferType::eUniform:
        bufferUsageFlags |= vk::BufferUsageFlagBits::eUniformBuffer;
        break;
    case BufferType::eStorage:
        bufferUsageFlags |= vk::BufferUsageFlagBits::eStorageBuffer;
        break;
    case BufferType::eIndex:
        bufferUsageFlags = vk::BufferUsageFlagBits::eIndexBuffer;
        break;
    case BufferType::eIndirect:
        bufferUsageFlags |= vk::BufferUsageFlagBits::eIndirectBuffer;
        break;
    case BufferType::eReadback:
        readback = true;
        break;
    }

    const auto buffer = Init::CreateBuffer(
        gContext,
        gGraphicsQueue.index,
        size,
        bufferUsageFlags,
        readback,
        debugName);
    gBuffers.emplace_back(buffer);
    const auto index = static_cast<u32>(gBuffers.size() - 1);
    return index;
}

void Swift::DestroyBuffer(const BufferHandle bufferHandle)
{
    const auto& realBuffer = gBuffers.at(bufferHandle);
    realBuffer.Destroy(gContext);
}

void* Swift::MapBuffer(const BufferHandle bufferHandle)
{
    const auto& realBuffer = gBuffers.at(bufferHandle);
    return Util::MapBuffer(gContext, realBuffer);
}

void Swift::UnmapBuffer(const BufferHandle bufferHandle)
{
    const auto& realBuffer = gBuffers.at(bufferHandle);
    Util::UnmapBuffer(gContext, realBuffer);
}

void Swift::UploadToBuffer(
    const BufferHandle& buffer,
    const void* data,
    const u64 offset,
    const u64 size)
{
    const auto& realBuffer = gBuffers.at(buffer);
    Util::UploadToBuffer(gContext, data, realBuffer, offset, size);
}

void Swift::UploadToMapped(
    void* mapped,
    const void* data,
    const u64 offset,
    const u64 size)
{
    Util::UploadToMapped(data, mapped, offset, size);
}

void Swift::DownloadBuffer(
    const BufferHandle& buffer,
    void* data,
    const u64 offset,
    const u64 size)
{
    const auto& realBuffer = gBuffers.at(buffer);
    vmaCopyAllocationToMemory(gContext.allocator, realBuffer.allocation, offset, data, size);
}

void Swift::UpdateSmallBuffer(
    const BufferHandle& buffer,
    const u64 offset,
    const u64 size,
    const void* data)
{
    const auto& realBuffer = gBuffers.at(buffer);
    const auto& commandBuffer = Render::GetCommandBuffer(gCurrentFrameData);
    commandBuffer.updateBuffer(realBuffer, offset, size, data);
}

void Swift::CopyBuffer(
    const BufferHandle srcBufferHandle,
    const BufferHandle dstBufferHandle,
    const u64 srcOffset,
    const u64 dstOffset,
    const u64 size)
{
    const auto region =
        vk::BufferCopy2().setSize(size).setSrcOffset(srcOffset).setDstOffset(dstOffset);
    const auto& realSrcBuffer = gBuffers.at(srcBufferHandle);
    const auto& realDstBuffer = gBuffers.at(dstBufferHandle);
    const auto& commandBuffer = Render::GetCommandBuffer(gCurrentFrameData);

    commandBuffer.copyBuffer2(
        vk::CopyBufferInfo2()
            .setSrcBuffer(realSrcBuffer)
            .setDstBuffer(realDstBuffer)
            .setRegions(region));
}

u64 Swift::GetBufferAddress(const BufferHandle& buffer)
{
    const auto& realBuffer = gBuffers.at(buffer);
    const auto addressInfo = vk::BufferDeviceAddressInfo().setBuffer(realBuffer.buffer);
    return gContext.device.getBufferAddress(addressInfo);
}

void Swift::BindIndexBuffer(const BufferHandle& bufferObject)
{
    const auto& realBuffer = gBuffers.at(bufferObject);
    const auto& commandBuffer = Render::GetCommandBuffer(gCurrentFrameData);
    commandBuffer.bindIndexBuffer(realBuffer, 0, vk::IndexType::eUint32);
}

void Swift::ClearImage(
    const ImageHandle image,
    const glm::vec4 color)
{
    const auto& commandBuffer = Render::GetCommandBuffer(gCurrentFrameData);
    auto& realImage = GetRealImage(image);
    const auto generalBarrier = Util::ImageBarrier(
        vk::ImageLayout::eUndefined,
        vk::ImageLayout::eGeneral,
        realImage,
        vk::ImageAspectFlagBits::eColor);
    Util::PipelineBarrier(commandBuffer, generalBarrier);

    const auto clearColor = vk::ClearColorValue(color.x, color.y, color.z, color.w);
    Util::ClearColorImage(commandBuffer, realImage, clearColor);

    const auto colorBarrier = Util::ImageBarrier(
        vk::ImageLayout::eGeneral,
        vk::ImageLayout::eColorAttachmentOptimal,
        realImage,
        vk::ImageAspectFlagBits::eColor);
    Util::PipelineBarrier(commandBuffer, colorBarrier);
}

void Swift::ClearSwapchainImage(const glm::vec4 color)
{
    const auto& commandBuffer = Render::GetCommandBuffer(gCurrentFrameData);
    const auto generalBarrier = Util::ImageBarrier(
        vk::ImageLayout::eUndefined,
        vk::ImageLayout::eGeneral,
        Render::GetSwapchainImage(gSwapchain),
        vk::ImageAspectFlagBits::eColor);
    Util::PipelineBarrier(commandBuffer, generalBarrier);

    const auto clearColor = vk::ClearColorValue(color.x, color.y, color.z, color.w);
    Util::ClearColorImage(commandBuffer, Render::GetSwapchainImage(gSwapchain), clearColor);

    const auto colorBarrier = Util::ImageBarrier(
        vk::ImageLayout::eGeneral,
        vk::ImageLayout::eColorAttachmentOptimal,
        Render::GetSwapchainImage(gSwapchain),
        vk::ImageAspectFlagBits::eColor);
    Util::PipelineBarrier(commandBuffer, colorBarrier);
}

void Swift::CopyImage(
    const ImageHandle srcImageHandle,
    const ImageHandle dstImageHandle,
    const glm::uvec2 extent)
{
    const auto& commandBuffer = Render::GetCommandBuffer(gCurrentFrameData);
    constexpr auto srcLayout = vk::ImageLayout::eTransferSrcOptimal;
    constexpr auto dstLayout = vk::ImageLayout::eTransferDstOptimal;
    auto& srcImage = GetRealImage(srcImageHandle);
    auto& dstImage = GetRealImage(dstImageHandle);
    const auto srcBarrier = Util::ImageBarrier(
        srcImage.currentLayout,
        srcLayout,
        srcImage,
        vk::ImageAspectFlagBits::eColor);
    const auto dstBarrier = Util::ImageBarrier(
        dstImage.currentLayout,
        dstLayout,
        dstImage,
        vk::ImageAspectFlagBits::eColor);
    Util::PipelineBarrier(commandBuffer, {srcBarrier, dstBarrier});
    Util::CopyImage(commandBuffer, srcImage, srcLayout, dstImage, dstLayout, Util::To2D(extent));
}

void Swift::CopyToSwapchain(
    const ImageHandle srcImageHandle,
    const glm::uvec2 extent)
{
    const auto& commandBuffer = Render::GetCommandBuffer(gCurrentFrameData);
    constexpr auto srcLayout = vk::ImageLayout::eTransferSrcOptimal;
    constexpr auto dstLayout = vk::ImageLayout::eTransferDstOptimal;
    auto& srcImage = GetRealImage(srcImageHandle);
    auto& dstImage = Render::GetSwapchainImage(gSwapchain);
    const auto srcBarrier = Util::ImageBarrier(
        srcImage.currentLayout,
        srcLayout,
        srcImage,
        vk::ImageAspectFlagBits::eColor);
    const auto dstBarrier = Util::ImageBarrier(
        dstImage.currentLayout,
        dstLayout,
        dstImage,
        vk::ImageAspectFlagBits::eColor);
    Util::PipelineBarrier(commandBuffer, {srcBarrier, dstBarrier});
    Util::CopyImage(commandBuffer, srcImage, srcLayout, dstImage, dstLayout, Util::To2D(extent));
}

void Swift::BlitImage(
    const ImageHandle srcImageHandle,
    const ImageHandle dstImageHandle,
    const glm::uvec2 srcOffset,
    const glm::uvec2 dstOffset)
{
    const auto commandBuffer = Render::GetCommandBuffer(gCurrentFrameData);
    constexpr auto srcLayout = vk::ImageLayout::eTransferSrcOptimal;
    constexpr auto dstLayout = vk::ImageLayout::eTransferDstOptimal;

    auto& srcImage = GetRealImage(srcImageHandle);
    auto& dstImage = GetRealImage(dstImageHandle);
    const auto srcBarrier = Util::ImageBarrier(
        srcImage.currentLayout,
        srcLayout,
        srcImage,
        vk::ImageAspectFlagBits::eColor);
    const auto dstBarrier = Util::ImageBarrier(
        dstImage.currentLayout,
        dstLayout,
        dstImage,
        vk::ImageAspectFlagBits::eColor);
    Util::PipelineBarrier(commandBuffer, {srcBarrier, dstBarrier});
    Util::BlitImage(
        commandBuffer,
        srcImage,
        srcLayout,
        dstImage,
        dstLayout,
        Util::To2D(srcOffset),
        Util::To2D(dstOffset));
}

void Swift::BlitToSwapchain(
    const ImageHandle srcImageHandle,
    const glm::uvec2 srcExtent)
{
    const auto commandBuffer = Render::GetCommandBuffer(gCurrentFrameData);
    constexpr auto srcLayout = vk::ImageLayout::eTransferSrcOptimal;
    constexpr auto dstLayout = vk::ImageLayout::eTransferDstOptimal;

    auto& srcImage = GetRealImage(srcImageHandle);
    auto& dstImage = Render::GetSwapchainImage(gSwapchain);
    const auto srcBarrier = Util::ImageBarrier(
        srcImage.currentLayout,
        srcLayout,
        srcImage,
        vk::ImageAspectFlagBits::eColor);
    const auto dstBarrier = Util::ImageBarrier(
        dstImage.currentLayout,
        dstLayout,
        dstImage,
        vk::ImageAspectFlagBits::eColor);
    Util::PipelineBarrier(commandBuffer, {srcBarrier, dstBarrier});
    Util::BlitImage(
        commandBuffer,
        srcImage,
        srcLayout,
        dstImage,
        dstLayout,
        Util::To2D(srcExtent),
        gSwapchain.extent);
}

void Swift::DispatchCompute(
    const u32 x,
    const u32 y,
    const u32 z)
{
    const auto commandBuffer = Render::GetCommandBuffer(gCurrentFrameData);
    commandBuffer.dispatch(x, y, z);
}

void Swift::PushConstant(
    const void* value,
    const u32 size)
{
    const auto& commandBuffer = Render::GetCommandBuffer(gCurrentFrameData);
    const auto& [shaders, stageFlags, pipeline, pipelineLayout] = gShaders.at(gCurrentShader);

    vk::ShaderStageFlags pushStageFlags;
    if (std::ranges::find(stageFlags, vk::ShaderStageFlagBits::eVertex) != stageFlags.end())
    {
        pushStageFlags = vk::ShaderStageFlagBits::eAllGraphics;
    }
    else if (std::ranges::find(stageFlags, vk::ShaderStageFlagBits::eCompute) != stageFlags.end())
    {
        pushStageFlags = vk::ShaderStageFlagBits::eCompute;
    }

    commandBuffer.pushConstants(pipelineLayout, pushStageFlags, 0, size, value);
}

void Swift::BeginTransfer(const ThreadHandle threadHandle)
{
    if (threadHandle != -1)
    {
        Util::BeginOneTimeCommand(gThreadDatas[threadHandle].command);
    }
    else
    {
        Util::BeginOneTimeCommand(gTransferCommand);
    }
}

void Swift::EndTransfer(const ThreadHandle threadHandle)
{
    Command transferCommand;
    Queue transferQueue;
    vk::Fence transferFence;
    if (threadHandle != -1)
    {
        transferQueue = gThreadDatas[threadHandle].queue;
        transferCommand = gThreadDatas[threadHandle].command;
        transferFence = gThreadDatas[threadHandle].fence;
    }
    else
    {
        transferQueue = gTransferQueue;
        transferCommand = gTransferCommand;
        transferFence = gTransferFence;
    }
    Util::EndCommand(transferCommand);
    Util::SubmitQueueHost(transferQueue, transferCommand, transferFence);
    Util::WaitFence(gContext, transferFence);
    Util::ResetFence(gContext, transferFence);
    for (const auto& buffer : gTransferStagingBuffers)
    {
        buffer.Destroy(gContext);
    }
    gTransferStagingBuffers.clear();
}

ThreadHandle Swift::CreateGraphicsThreadContext()
{
    if (!SupportsGraphicsMultithreading())
        assert(false);
    const u32 size = static_cast<u32>(gThreadDatas.size());
    const auto queue = Init::GetQueue(gContext, gGraphicsQueue.index, 1, "Thread Queue");
    const auto threadQueue = Queue().SetQueue(queue).SetIndex(gGraphicsQueue.index);

    const auto commandPool =
        Init::CreateCommandPool(gContext, gGraphicsQueue.index, "Thread Command Pool");
    const auto commandBuffer =
        Init::CreateCommandBuffer(gContext, commandPool, "Thread Command Buffer");
    const auto command = Command().SetCommandBuffer(commandBuffer).SetCommandPool(commandPool);

    const auto fence = Init::CreateFence(gContext, {}, "Thread Fence");

    gThreadDatas.emplace_back(Thread().SetQueue(threadQueue).SetCommand(command).SetFence(fence));

    return size;
}

void Swift::DestroyGraphicsThreadContext(const ThreadHandle threadHandle)
{
    if (!SupportsGraphicsMultithreading())
        assert(false);
    gThreadDatas[threadHandle].Destroy(gContext);
    gThreadDatas.erase(gThreadDatas.begin() + threadHandle);
}
