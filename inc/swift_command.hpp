#pragma once
#include "span"
#include "swift_descriptor.hpp"
#include "swift_macros.hpp"
#include "swift_shader.hpp"
#include "swift_structs.hpp"

namespace Swift
{
    class IContext;
    class ICommand
    {
    public:
        SWIFT_DESTRUCT(ICommand);
        SWIFT_NO_MOVE(ICommand);
        SWIFT_NO_COPY(ICommand);

        virtual void* GetCommandList() = 0;
        virtual void* GetCommandAllocator() = 0;
        virtual void Begin() = 0;
        virtual void End() = 0;
        virtual void SetViewport(const Viewport& viewport) = 0;
        virtual void SetScissor(const Scissor& scissor) = 0;
        virtual void PushConstants(const void* data, uint32_t size, uint32_t offset = 0) = 0;
        virtual void BindShader(IShader* shader) = 0;
        virtual void DispatchMesh(uint32_t group_x, uint32_t group_y, uint32_t group_z) = 0;
        virtual void DispatchCompute(uint32_t group_x, uint32_t group_y, uint32_t group_z) = 0;
        virtual void CopyBufferToTexture( IContext* context,
                                          IBuffer* buffer,
                                          ITexture* texture,
                                         uint16_t mip_levels = 1,
                                         uint16_t array_size = 1) = 0;
        virtual void CopyBufferRegion(const BufferCopyRegion& region) = 0;
        virtual void CopyImageToImage(ITexture* src_resource, ITexture* dst_resource) = 0;
        virtual void CopyTextureRegion(const TextureCopyRegion& region) = 0;
        virtual void BindRenderTargets(std::span<IRenderTarget*> render_targets, IDepthStencil* depth_stencil) = 0;
        virtual void BindRenderTargets(IRenderTarget* render_target, IDepthStencil* depth_stencil)
        {
            if (render_target != nullptr)
            {
                BindRenderTargets(std::span{&render_target, 1}, depth_stencil);
            }
            else
            {
                BindRenderTargets(std::span{&render_target, 0}, depth_stencil);
            }
        }
        virtual void ClearRenderTarget(IRenderTarget* texture_handle, const std::array<float, 4>& color) = 0;
        virtual void ClearDepthStencil(IDepthStencil* texture_handle, float depth, uint8_t stencil) = 0;
        virtual void TransitionImage(ITexture* image, ResourceState new_state) = 0;
        virtual void TransitionBuffer(IBuffer* buffer, ResourceState new_state) = 0;
        virtual void UAVBarrier(IBuffer* buffer) = 0;
        virtual void UAVBarrier(ITexture* texture) = 0;

    protected:
        SWIFT_CONSTRUCT(ICommand);
    };
}  // namespace Swift
