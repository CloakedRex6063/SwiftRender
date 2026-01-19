#pragma once

#include "swift_command.hpp"
#include "swift_macros.hpp"
#include "d3d12_descriptor.hpp"

namespace Swift::D3D12
{
    class Context;

    class Command final : public ICommand
    {
    public:
        Command(IContext* context, const std::shared_ptr<DescriptorHeap>& cbv_heap, QueueType type);
        ~Command() override;
        SWIFT_NO_COPY(Command);
        SWIFT_NO_MOVE(Command);

        void* GetCommandList() override { return m_list; }
        void* GetCommandAllocator() override { return m_allocator; }

        void Begin() override;
        void End() override;
        void SetViewport(const Viewport& viewport) override;
        void SetScissor(const Scissor& scissor) override;
        void BindConstantBuffer(const std::shared_ptr<IBuffer>& buffer, uint32_t slot) override;
        void PushConstants(const void* data, uint32_t size, uint32_t offset) override;
        void BindShader(const std::shared_ptr<IShader>& shader) override;
        void DispatchMesh(uint32_t group_x, uint32_t group_y, uint32_t group_z) override;
        void DispatchCompute(uint32_t group_x, uint32_t group_y, uint32_t group_z) override;
        void CopyBufferToTexture(const std::shared_ptr<IContext>& context, const BufferTextureCopyRegion& region) override;
        void CopyBufferRegion(const BufferCopyRegion& region) override;
        void CopyTextureRegion(const TextureCopyRegion& region) override;
        void BindRenderTargets(std::span<const std::shared_ptr<ITexture>> render_targets,
                               const std::shared_ptr<ITexture>& depth_stencil) override;
        void ClearRenderTarget(const std::shared_ptr<ITexture>& texture, const std::array<float, 4>& color) override;
        void ClearDepthStencil(const std::shared_ptr<ITexture>& texture, float depth, uint8_t stencil) override;
        void TransitionResource(const std::shared_ptr<IResource>& resource_handle, ResourceState new_state) override;

    private:
        Context* m_context;
        QueueType m_type;
        ID3D12GraphicsCommandList10* m_list = nullptr;
        ID3D12CommandAllocator* m_allocator = nullptr;
        std::shared_ptr<DescriptorHeap> m_cbv_srv_uav_heap;
    };
}  // namespace Swift::D3D12
