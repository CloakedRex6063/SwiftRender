#pragma once
#include "span"
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
        virtual void BindConstantBuffer(const std::shared_ptr<IBuffer>& buffer, uint32_t slot) = 0;
        virtual void PushConstants(const void* data, uint32_t size, uint32_t offset = 0) = 0;
        virtual void BindShader(const std::shared_ptr<IShader>& shader) = 0;
        virtual void DispatchMesh(uint32_t group_x, uint32_t group_y, uint32_t group_z) = 0;
        virtual void DispatchCompute(uint32_t group_x, uint32_t group_y, uint32_t group_z) = 0;
        virtual void CopyBufferToTexture(const std::shared_ptr<IContext>& context, const BufferTextureCopyRegion& region) = 0;
        virtual void CopyBufferRegion(const BufferCopyRegion& region) = 0;
        virtual void CopyTextureRegion(const TextureCopyRegion& region) = 0;
        virtual void BindRenderTargets(std::span<const std::shared_ptr<ITexture>> render_targets,
                                       const std::shared_ptr<ITexture>& depth_stencil) = 0;
        virtual void ClearRenderTarget(const std::shared_ptr<ITexture>& texture_handle, const std::array<float, 4>& color) = 0;
        virtual void ClearDepthStencil(const std::shared_ptr<ITexture>& texture_handle, float depth, uint8_t stencil) = 0;
        virtual void TransitionResource(const std::shared_ptr<IResource>& resource_handle, ResourceState new_state) = 0;

    protected:
        SWIFT_CONSTRUCT(ICommand);
    };
}  // namespace Swift
