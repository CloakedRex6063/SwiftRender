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
        Command(IContext* context, DescriptorHeap* cbv_heap, QueueType type, std::string_view debug_name);
        ~Command() override;
        SWIFT_NO_COPY(Command);
        SWIFT_NO_MOVE(Command);

        void* GetCommandList() override { return m_list; }
        void* GetCommandAllocator() override { return m_allocator; }

        void Begin() override;
        void End() override;
        void SetViewport(const Viewport& viewport) override;
        void SetScissor(const Scissor& scissor) override;
        void BindConstantBuffer(IBuffer* buffer, uint32_t slot) override;
        void PushConstants(const void* data, uint32_t size, uint32_t offset) override;
        void BindShader(IShader* shader) override;
        void DispatchMesh(uint32_t group_x, uint32_t group_y, uint32_t group_z) override;
        void DispatchCompute(uint32_t group_x, uint32_t group_y, uint32_t group_z) override;
        void CopyBufferToTexture(const IContext* context, const BufferTextureCopyRegion& region) override;
        void CopyBufferRegion(const BufferCopyRegion& region) override;
        void CopyTextureRegion(const TextureCopyRegion& region) override;
        void BindRenderTargets(std::span<IRenderTarget*> render_targets, IDepthStencil* depth_stencil) override;
        void ClearRenderTarget(IRenderTarget* render_target, const std::array<float, 4>& color)  override;
        void ClearDepthStencil(IDepthStencil* depth_stencil, float depth, uint8_t stencil)  override;
        void ResourceBarrier(const std::shared_ptr<IResource>& resource_handle, ResourceState new_state)  override;
        void UAVBarrier(const std::shared_ptr<IResource>& resource_handle)  override;

    private:
        Context* m_context;
        QueueType m_type;
        ID3D12GraphicsCommandList10* m_list = nullptr;
        ID3D12CommandAllocator* m_allocator = nullptr;
        DescriptorHeap* m_cbv_srv_uav_heap;
    };
}  // namespace Swift::D3D12
