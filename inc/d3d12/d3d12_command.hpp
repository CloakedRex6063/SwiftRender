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
        Command(IContext* context,
                DescriptorHeap* cbv_heap,
                DescriptorHeap* sampler_heap,
                ID3D12RootSignature* root_signature,
                QueueType type,
                std::string_view debug_name);
        ~Command() override;
        SWIFT_NO_COPY(Command);
        SWIFT_NO_MOVE(Command);

        void* GetCommandList() override { return m_list; }
        void* GetCommandAllocator() override { return m_allocator; }

        void Begin() override;
        void End() override;
        void SetViewport(const Viewport& viewport) override;
        void SetScissor(const Scissor& scissor) override;
        void PushConstants(const void* data, uint32_t size, uint32_t offset) override;
        void BindShader(IShader* shader) override;
        void DispatchMesh(uint32_t group_x, uint32_t group_y, uint32_t group_z) override;
        void DispatchCompute(uint32_t group_x, uint32_t group_y, uint32_t group_z) override;
        void CopyBufferToTexture(IContext* context,
                                 IBuffer* buffer,
                                 ITexture* texture,
                                 uint16_t mip_levels = 1,
                                 uint16_t array_size = 1) override;
        void CopyImageToImage(ITexture* src_resource, ITexture* dst_resource) override;
        void CopyBufferRegion(const BufferCopyRegion& region) override;
        void CopyTextureRegion(const TextureCopyRegion& region) override;
        void BindConstantBuffer(IBuffer* buffer, uint32_t slot) override;
        void BindRenderTargets(std::span<IRenderTarget*> render_targets, IDepthStencil* depth_stencil) override;
        void ClearRenderTarget(IRenderTarget* render_target, const std::array<float, 4>& color) override;
        void ClearDepthStencil(IDepthStencil* depth_stencil, float depth, uint8_t stencil) override;
        void TransitionImage(ITexture* image, ResourceState new_state) override;
        void TransitionBuffer(IBuffer* buffer, ResourceState new_state) override;
        void UAVBarrier(IBuffer* buffer) override;
        void UAVBarrier(ITexture* texture) override;

    private:
        Context* m_context;
        QueueType m_type;
        ID3D12GraphicsCommandList10* m_list = nullptr;
        ID3D12CommandAllocator* m_allocator = nullptr;
        DescriptorHeap* m_cbv_srv_uav_heap = nullptr;
        DescriptorHeap* m_sampler_heap = nullptr;
        ID3D12RootSignature* m_root_signature = nullptr;
        IShader* m_shader = nullptr;
    };
}  // namespace Swift::D3D12
