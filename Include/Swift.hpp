#pragma once
#include "SwiftStructs.hpp"
#include "Vulkan/VulkanStructs.hpp"
#include "SwiftEnums.hpp"

namespace Swift
{
    void Init(const InitInfo& initInfo);
    void Shutdown();

    InitInfo GetInitInfo();
    Vulkan::Context GetContext();
    Vulkan::Queue GetGraphicsQueue();
    Vulkan::Queue GetTransferQueue();
    Vulkan::Queue GetComputeQueue();
    Vulkan::Command GetGraphicsCommand();
    vk::Sampler GetDefaultSampler();

    void WaitIdle();

    bool SupportsGraphicsMultithreading();

    inline bool IsValid(const BufferHandle handle)
    {
        return handle != InvalidHandle;
    }

    void BeginFrame(const DynamicInfo& dynamicInfo);
    void EndFrame(const DynamicInfo& dynamicInfo);
    
    void BeginRendering(bool bLoadPreviousColor = false, bool bLoadPreviousDepth = false);
    void EndRendering();

    void BeginRendering(
        const glm::uvec2& extent,
        const std::vector<ImageHandle>& colorImages,
        const ImageHandle& depthImage,
        bool bLoadPreviousColor = false,
        bool bLoadPreviousDepth = false);
    void EndRendering(ImageHandle image);

    void SetCullMode(const CullMode& cullMode);
    void SetDepthCompareOp(DepthCompareOp depthCompareOp);
    void SetPolygonMode(PolygonMode polygonMode);
    void SetLineWidth(float lineWidth);
    void SetTopology(Topology topology);
    void SetViewportAndScissor(const glm::uvec2& viewport);

    void Draw(
        u32 vertexCount,
        u32 instanceCount,
        u32 firstVertex,
        u32 firstInstance);
    void DrawIndexed(
        u32 indexCount,
        u32 instanceCount,
        u32 firstIndex,
        int vertexOffset,
        u32 firstInstance);
    void DrawIndexedIndirect(
        const BufferHandle& buffer,
        u64 offset,
        u32 drawCount,
        u32 stride);
    void DrawIndexedIndirectCount(
        const BufferHandle& buffer,
        u64 offset,
        const BufferHandle& countBuffer,
        u64 countOffset,
        u32 maxDrawCount,
        u32 stride);

    SamplerHandle CreateSampler(Filter magFilter, Filter minFilter, Wrap wrapS, Wrap wrapT);
    
    ShaderHandle CreateGraphicsShader(
        std::string_view vertexPath,
        std::string_view fragmentPath,
        std::string_view debugName);

    ShaderHandle CreateComputeShader(
        const std::string& computePath,
        std::string_view debugName);

    void BindShader(const ShaderHandle& shaderHandle);

    void PushConstant(
        const void* value,
        u32 size);

    template <typename T>
    void PushConstant(T value)
    {
        PushConstant(&value, sizeof(T));
    }

    void DispatchCompute(
        u32 x,
        u32 y,
        u32 z);

    void TransitionImage(ImageHandle handle, ImageTransition transition);
    ImageHandle CreateImage(
        ImageUsage usage,
        ImageFormat format,
        glm::uvec2 size,
        std::string_view debugName);
    void DestroyImage(ImageHandle imageHandle);
    ImageHandle LoadImageFromFile(
        const std::filesystem::path& filePath,
        int mipLevel,
        bool loadAllMipMaps,
        std::string_view debugName,
        SamplerHandle samplerHandle = InvalidHandle,
        bool tempImage = false,
        ThreadHandle thread = -1);
    ImageHandle LoadImageFromFileQueued(
        const std::filesystem::path& filePath,
        int mipLevel,
        bool loadAllMipMaps,
        std::string_view debugName,
        SamplerHandle samplerHandle = InvalidHandle,
        bool tempImage = false,
        ThreadHandle thread = -1);
    ImageHandle LoadCubemapFromFile(
        const std::filesystem::path& filePath,
        std::string_view debugName);
    
    int GetMinLod(ImageHandle image);
    int GetMaxLod(ImageHandle image);
    u32 GetImageArrayIndex(ImageHandle imageHandle);
    glm::uvec2 GetImageSize(ImageHandle imageHandle);
    std::string_view GetURI(ImageHandle imageHandle);
    vk::ImageView GetImageView(ImageHandle imageHandle);
    ImageHandle ReadOnlyImageFromIndex(int imageIndex);
    void UpdateImage(
        ImageHandle baseImage,
        ImageHandle tempImage);
    void ClearTempImages();

    BufferHandle CreateBuffer(
        BufferType bufferType,
        u32 size,
        std::string_view debugName);
    void DestroyBuffer(BufferHandle bufferHandle);

    void* MapBuffer(BufferHandle bufferHandle);
    void UnmapBuffer(BufferHandle bufferHandle);
    void UploadToBuffer(
        const BufferHandle& buffer,
        const void* data,
        u64 offset,
        u64 size);
    void UploadToMapped(
        void* mapped,
        const void* data,
        u64 offset,
        u64 size);
    void DownloadBuffer(
        const BufferHandle& buffer,
        void* data,
        u64 offset,
        u64 size);
    void UpdateSmallBuffer(
        const BufferHandle& buffer,
        u64 offset,
        u64 size,
        const void* data);
    void CopyBuffer(
        BufferHandle srcBufferHandle,
        BufferHandle dstBufferHandle,
        u64 srcOffset,
        u64 dstOffset,
        u64 size);

    u64 GetBufferAddress(const BufferHandle& buffer);
    void BindIndexBuffer(const BufferHandle& bufferObject);

    void ClearImage(
        ImageHandle image,
        glm::vec4 color);
    void ClearSwapchainImage(glm::vec4 color);
    void CopyImage(
        ImageHandle srcImageHandle,
        ImageHandle dstImageHandle,
        glm::uvec2 extent);
    void CopyToSwapchain(
        ImageHandle srcImageHandle,
        glm::uvec2 extent);
    void BlitImage(
        ImageHandle srcImageHandle,
        ImageHandle dstImageHandle,
        glm::uvec2 srcOffset,
        glm::uvec2 dstOffset);
    void BlitToSwapchain(
        ImageHandle srcImageHandle,
        glm::uvec2 srcExtent);

    void BeginTransfer(ThreadHandle threadHandle = -1);
    void EndTransfer(ThreadHandle threadHandle = -1);

    ThreadHandle CreateGraphicsThreadContext();
    void DestroyGraphicsThreadContext(ThreadHandle threadHandle);
} // namespace Swift
