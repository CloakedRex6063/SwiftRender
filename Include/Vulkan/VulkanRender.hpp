#pragma once
#include "VulkanStructs.hpp"

namespace Swift::Vulkan::Render
{
    u32 AcquireNextImage(
        const Queue& queue,
        const Context& context,
        Swapchain& swapchain,
        vk::Semaphore semaphore,
        vk::Extent2D extent);

    void Present(
        const Context& context,
        Swapchain& swapchain,
        Queue queue,
        vk::Semaphore semaphore,
        vk::Extent2D extent);

    inline Image& GetSwapchainImage(Swapchain& swapchain)
    {
        return swapchain.images.at(swapchain.imageIndex);
    }

    inline vk::Semaphore& GetRenderSemaphore(FrameData& frameData)
    {
        return frameData.renderSemaphore;
    }

    inline vk::Semaphore& GetPresentSemaphore(FrameData& frameData)
    {
        return frameData.presentSemaphore;
    }

    inline vk::Fence& GetRenderFence(FrameData& frameData)
    {
        return frameData.renderFence;
    }

    inline vk::CommandBuffer& GetCommandBuffer(FrameData& frameData)
    {
        return frameData.renderCommand.commandBuffer;
    }

    inline vk::CommandPool& GetCommandPool(FrameData& frameData)
    {
        return frameData.renderCommand.commandPool;
    }

    void BeginRendering(
        const vk::CommandBuffer commandBuffer,
        Swapchain& swapchain,
        const bool enableDepth);

    void BeginRendering(
        vk::CommandBuffer commandBuffer,
        vk::Extent2D extent,
        const std::span<Image>& colorImage,
        const Image& depthImage,
        bool enableDepth);

    void BeginRendering(
        const vk::CommandBuffer commandBuffer,
        const vk::ImageView& renderImageView,
        const vk::ImageView& depthImageView,
        const vk::Extent2D extent,
        const u32 viewMask = 0,
        const int layerCount = 1);

    inline void EndRendering(const vk::CommandBuffer commandBuffer)
    {
        commandBuffer.endRendering();
    }

    inline void BindShader(
        const vk::CommandBuffer commandBuffer,
        const Context& context,
        const BindlessDescriptor& descriptor,
        Shader shader,
        const bool bUsePipelines)
    {
        const auto it = std::ranges::find(shader.stageFlags, vk::ShaderStageFlagBits::eCompute);
        auto pipelineBindPoint = vk::PipelineBindPoint::eCompute;
        if (it == std::end(shader.stageFlags))
        {
            pipelineBindPoint = vk::PipelineBindPoint::eGraphics;
        }

        if (bUsePipelines)
        {
            commandBuffer.bindPipeline(pipelineBindPoint, shader.pipeline);
        }
        else
        {
            commandBuffer.bindShadersEXT(shader.stageFlags, shader.shaders, context.dynamicLoader);
        }

        commandBuffer
            .bindDescriptorSets(pipelineBindPoint, shader.pipelineLayout, 0, descriptor.set, {});
    }

    // --------------------------------------------------------------------------------------------
    // --------------------------------------------------------------------------------------------
    // ------------------------------------ Pipeline State ----------------------------------------
    // --------------------------------------------------------------------------------------------
    // --------------------------------------------------------------------------------------------

    inline void SetViewportAndScissor(
        const vk::CommandBuffer commandBuffer,
        const vk::Extent2D extent)
    {
        commandBuffer.setViewportWithCount(
            vk::Viewport().setWidth(extent.width).setHeight(extent.height).setMaxDepth(1.f));
        commandBuffer.setScissorWithCount(vk::Rect2D().setExtent(extent));
    }

    // --------------------------------------------------------------------------------------------
    // ---------------------------------Color Blend Attachment-------------------------------------
    // --------------------------------------------------------------------------------------------

    inline void SetColorBlendDefault(
        const Context& context,
        const vk::CommandBuffer commandBuffer)
    {
        commandBuffer.setColorWriteMaskEXT(
            0,
            {vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG |
             vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA},
            context.dynamicLoader);
        bool colorBlendEnabled = false;

        constexpr auto colorBlendEquation =
            vk::ColorBlendEquationEXT()
                .setAlphaBlendOp(vk::BlendOp::eAdd)
                .setSrcAlphaBlendFactor(vk::BlendFactor::eOne)
                .setDstAlphaBlendFactor(vk::BlendFactor::eZero)
                .setColorBlendOp(vk::BlendOp::eAdd)
                .setSrcColorBlendFactor(vk::BlendFactor::eSrcAlpha)
                .setDstColorBlendFactor(vk::BlendFactor::eOneMinusSrcAlpha);
        commandBuffer.setColorBlendEquationEXT(0, colorBlendEquation, context.dynamicLoader);
        commandBuffer.setColorBlendEnableEXT(0, colorBlendEnabled, context.dynamicLoader);
    }

    inline void EnableTransparencyBlending(
        const Context& context,
        const vk::CommandBuffer commandBuffer)
    {
        constexpr auto colorBlendEquation =
            vk::ColorBlendEquationEXT()
                .setAlphaBlendOp(vk::BlendOp::eAdd)
                .setSrcAlphaBlendFactor(vk::BlendFactor::eOne)
                .setDstAlphaBlendFactor(vk::BlendFactor::eZero)
                .setColorBlendOp(vk::BlendOp::eAdd)
                .setSrcColorBlendFactor(vk::BlendFactor::eSrcAlpha)
                .setDstColorBlendFactor(vk::BlendFactor::eOneMinusSrcAlpha);
        commandBuffer.setColorBlendEquationEXT(0, colorBlendEquation, context.dynamicLoader);
        commandBuffer.setColorBlendEnableEXT(0, true, context.dynamicLoader);
    }
    inline void DisableBlending(
        const Context& context,
        const vk::CommandBuffer commandBuffer)
    {
        constexpr auto colorBlendEquation =
            vk::ColorBlendEquationEXT()
                .setAlphaBlendOp(vk::BlendOp::eAdd)
                .setSrcAlphaBlendFactor(vk::BlendFactor::eOne)
                .setDstAlphaBlendFactor(vk::BlendFactor::eZero)
                .setColorBlendOp(vk::BlendOp::eAdd)
                .setSrcColorBlendFactor(vk::BlendFactor::eSrcAlpha)
                .setDstColorBlendFactor(vk::BlendFactor::eOneMinusSrcAlpha);
        commandBuffer.setColorBlendEquationEXT(0, colorBlendEquation, context.dynamicLoader);
        commandBuffer.setColorBlendEnableEXT(0, 0, nullptr, context.dynamicLoader);
    }

    // --------------------------------------------------------------------------------------------
    // -------------------------------------Input Assembly-----------------------------------------
    // --------------------------------------------------------------------------------------------

    inline void SetInputAssemblyDefault(const vk::CommandBuffer commandBuffer)
    {
        commandBuffer.setPrimitiveTopology(vk::PrimitiveTopology::eTriangleList);
        commandBuffer.setPrimitiveRestartEnable(false);
    }

    inline void SetPrimitiveTopology(
        const vk::CommandBuffer commandBuffer,
        const vk::PrimitiveTopology topology)
    {
        commandBuffer.setPrimitiveTopology(topology);
    }

    // --------------------------------------------------------------------------------------------
    // ---------------------------------Vertex Input State-----------------------------------------
    // --------------------------------------------------------------------------------------------

    inline void SetVertexInputDefault(
        const Context& context,
        const vk::CommandBuffer commandBuffer)
    {
        commandBuffer.setVertexInputEXT({}, {}, context.dynamicLoader);
    }

    // --------------------------------------------------------------------------------------------
    // -------------------------------------Depth Stencil------------------------------------------
    // --------------------------------------------------------------------------------------------

    inline void SetDepthStencilDefault(const vk::CommandBuffer commandBuffer)
    {
        commandBuffer.setDepthTestEnable(true);
        commandBuffer.setDepthCompareOp(vk::CompareOp::eLess);
        commandBuffer.setDepthWriteEnable(true);
        commandBuffer.setStencilTestEnable(false);
        commandBuffer.setDepthBounds(0, 1);
    }

    inline void DisableDepth(const vk::CommandBuffer commandBuffer)
    {
        commandBuffer.setDepthTestEnable(false);
        commandBuffer.setDepthWriteEnable(false);
    }

    inline void EnableDepth(const vk::CommandBuffer commandBuffer)
    {
        commandBuffer.setDepthTestEnable(true);
        commandBuffer.setDepthCompareOp(vk::CompareOp::eLess);
        commandBuffer.setDepthWriteEnable(true);
    }

    inline void SetDepthCompareOp(
        const vk::CommandBuffer commandBuffer,
        const vk::CompareOp compareOp)
    {
        commandBuffer.setDepthCompareOp(compareOp);
    }

    // --------------------------------------------------------------------------------------------
    // -------------------------------------Rasterization------------------------------------------
    // --------------------------------------------------------------------------------------------

    inline void SetRasterizerDefault(
        const Context& context,
        const vk::CommandBuffer commandBuffer)
    {
        commandBuffer.setRasterizerDiscardEnable(false);
        commandBuffer.setRasterizationSamplesEXT(
            vk::SampleCountFlagBits::e1,
            context.dynamicLoader);
        commandBuffer.setLineWidth(1.f);
        commandBuffer.setPolygonModeEXT(vk::PolygonMode::eFill, context.dynamicLoader);
        commandBuffer.setCullMode(vk::CullModeFlagBits::eBack);
        commandBuffer.setFrontFace(vk::FrontFace::eCounterClockwise);
        commandBuffer.setDepthBias(0, 0, 0);
        commandBuffer.setDepthBiasEnable(false);
    }
    inline void SetCullMode(
        const vk::CommandBuffer commandBuffer,
        const vk::CullModeFlagBits mode)
    {
        commandBuffer.setCullMode(mode);
    }
    inline void SetFrontFace(
        const vk::CommandBuffer commandBuffer,
        const vk::FrontFace frontFace)
    {
        commandBuffer.setFrontFace(frontFace);
    }
    inline void SetPolygonMode(
        const Context& context,
        const vk::CommandBuffer commandBuffer,
        const vk::PolygonMode polygonMode)
    {
        commandBuffer.setPolygonModeEXT(polygonMode, context.dynamicLoader);
    }

    // --------------------------------------------------------------------------------------------
    // -------------------------------------Multi Sample State-------------------------------------
    // --------------------------------------------------------------------------------------------

    inline void SetMultiSampleDefault(
        const Context& context,
        const vk::CommandBuffer commandBuffer)
    {
        commandBuffer.setSampleMaskEXT(
            vk::SampleCountFlagBits::e1,
            0xFFFFFFFF,
            context.dynamicLoader);
        commandBuffer.setAlphaToCoverageEnableEXT(false, context.dynamicLoader);
        commandBuffer.setAlphaToOneEnableEXT(false, context.dynamicLoader);
    }

    inline void SetPipelineDefault(
        const Context& context,
        const vk::CommandBuffer commandBuffer,
        const vk::Extent2D extent,
        const bool bUsePipeline)
    {
        if (bUsePipeline)
        {
            const auto viewport = vk::Viewport()
                                      .setWidth(float(extent.width))
                                      .setHeight(float(extent.height))
                                      .setMinDepth(0.f)
                                      .setMaxDepth(1.f);
            commandBuffer.setViewport(0, viewport);
            commandBuffer.setScissor(0, vk::Rect2D().setExtent(extent));
        }
        SetViewportAndScissor(commandBuffer, extent);
        SetColorBlendDefault(context, commandBuffer);
        SetInputAssemblyDefault(commandBuffer);
        SetVertexInputDefault(context, commandBuffer);
        SetDepthStencilDefault(commandBuffer);
        SetRasterizerDefault(context, commandBuffer);
        SetMultiSampleDefault(context, commandBuffer);
    }
} // namespace Swift::Vulkan::Render
