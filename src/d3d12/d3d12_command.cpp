#include "d3d12/d3d12_command.hpp"
#include "d3d12/d3d12_helpers.hpp"
#include "swift_buffer.hpp"
#include "swift_context.hpp"
#include "swift_texture.hpp"
#include "d3d12/d3d12_resource.hpp"
#include "d3d12/d3d12_shader.hpp"
#include "d3d12/d3d12_texture.hpp"

Swift::D3D12::Command::Command(IContext *context, const std::shared_ptr<DescriptorHeap> &cbv_heap,
                               const QueueType type) : m_context(dynamic_cast<
                                                           Context *>(context)),
                                                       m_cbv_srv_uav_heap(cbv_heap)
{
    const auto device = static_cast<ID3D12Device14 *>(context->GetDevice());
    m_type = type;
    [[maybe_unused]]
            auto result = device->CreateCommandAllocator(ToCommandType(type),
                                                         IID_PPV_ARGS(&m_allocator));

    result = device->CreateCommandList(0, ToCommandType(type), m_allocator, nullptr,
                                       IID_PPV_ARGS(&m_list));

    result = m_list->Close();
}

Swift::D3D12::Command::~Command()
{
    m_allocator->Release();
    m_list->Release();
}

void Swift::D3D12::Command::Begin()
{
    [[maybe_unused]]
            auto result = m_allocator->Reset();

    result = m_list->Reset(m_allocator, nullptr);

    if (m_list->GetType() != D3D12_COMMAND_LIST_TYPE_COPY)
    {
        const auto descriptor_heaps = std::array{m_cbv_srv_uav_heap->GetHeap()};
        m_list->SetDescriptorHeaps(1, descriptor_heaps.data());
    }
}

void Swift::D3D12::Command::End()
{
    if (m_list->GetType() == D3D12_COMMAND_LIST_TYPE_DIRECT)
    {
        TransitionResource(m_context->GetCurrentSwapchainTexture()->GetResource(), ResourceState::ePresent);
    }
    [[maybe_unused]]
            const auto result = m_list->Close();
}

void Swift::D3D12::Command::SetViewport(const Viewport &viewport)
{
    const D3D12_VIEWPORT dx_viewport = {
        viewport.offset[0], viewport.offset[1], viewport.dimensions[0],
        viewport.dimensions[1], viewport.depth_range[0],
        viewport.depth_range[1]
    };
    m_list->RSSetViewports(1, &dx_viewport);
}

void Swift::D3D12::Command::SetScissor(const Scissor &scissor)
{
    const D3D12_RECT dx_scissor = {
        (int) scissor.offset[0], (int) scissor.offset[1], (int) scissor.dimensions[0],
        (int) scissor.dimensions[1]
    };
    m_list->RSSetScissorRects(1, &dx_scissor);
}

void Swift::D3D12::Command::BindConstantBuffer(const std::shared_ptr<IBuffer> &buffer, const uint32_t slot)
{
    const D3D12_GPU_VIRTUAL_ADDRESS gpu_address = buffer->GetResource()->GetVirtualAddress();
    m_list->SetGraphicsRootConstantBufferView(slot, gpu_address);
}

void Swift::D3D12::Command::PushConstants(const void *data, const uint32_t size, const uint32_t offset)
{
    m_list->SetGraphicsRoot32BitConstants(0, size / sizeof(uint32_t), data, offset);
}

void Swift::D3D12::Command::BindShader(const std::shared_ptr<IShader> &shader)
{
    const auto dx_shader = static_cast<Shader *>(shader.get()); // NOLINT(*-pro-type-static-cast-downcast)
    if (m_type != QueueType::eTransfer)
    {
        const auto descriptor_heaps = std::array{m_cbv_srv_uav_heap->GetHeap()};
        m_list->SetDescriptorHeaps(1, descriptor_heaps.data());
        m_list->SetComputeRootSignature(dx_shader->m_root_signature);
    }
    if (m_type == QueueType::eGraphics)
    {
        m_list->SetGraphicsRootSignature(dx_shader->m_root_signature);
    }
    m_list->SetPipelineState(static_cast<ID3D12PipelineState *>(shader->GetPipeline()));
}

void Swift::D3D12::Command::DispatchMesh(const uint32_t group_x, const uint32_t group_y,
                                         const uint32_t group_z)
{
    m_list->DispatchMesh(group_x, group_y, group_z);
}

void Swift::D3D12::Command::DispatchCompute(const uint32_t group_x, const uint32_t group_y,
                                            const uint32_t group_z)
{
    m_list->Dispatch(group_x, group_y, group_z);
}

void Swift::D3D12::Command::CopyBufferToTexture(const std::shared_ptr<IContext> &context,
                                                const BufferTextureCopyRegion &region)
{
    auto *dst_resource = static_cast<ID3D12Resource *>(region.dst_texture->GetResource()->
        GetResource());
    auto *src_resource = static_cast<ID3D12Resource *>(region.src_buffer->GetResource()->
        GetResource());

    const auto device = static_cast<ID3D12Device14 *>(context->GetDevice());

    D3D12_PLACED_SUBRESOURCE_FOOTPRINT layout;
    UINT numRows;
    UINT64 rowSizeInBytes;
    UINT64 totalBytes;

    auto texture = region.dst_texture;

    // TODO: Handle msaa
    constexpr auto sample_desc = DXGI_SAMPLE_DESC{1, 0};

    const D3D12_RESOURCE_DESC texture_desc = {
        .Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D,
        .Alignment = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT,
        .Width = texture->GetSize()[0],
        .Height = texture->GetSize()[1],
        .DepthOrArraySize = static_cast<uint16_t>(texture->GetArraySize()),
        .MipLevels = static_cast<uint16_t>(texture->GetMipLevels()),
        .Format = ToDXGIFormat(texture->GetFormat()),
        .SampleDesc = sample_desc,
        .Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN,
    };

    device->GetCopyableFootprints(&texture_desc, 0, 1, 0,
                                  &layout, &numRows, &rowSizeInBytes, &totalBytes);

    const D3D12_TEXTURE_COPY_LOCATION src_location = {
        .pResource = src_resource,
        .Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT,
        .PlacedFootprint = layout,
    };

    D3D12_TEXTURE_COPY_LOCATION dst_location = {
        .pResource = dst_resource,
        .Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX,
        .SubresourceIndex = 0,
    };

    m_list->CopyTextureRegion(&dst_location, 0, 0, 0, &src_location, nullptr);
}

void Swift::D3D12::Command::CopyBufferRegion(const BufferCopyRegion &region)
{
    auto *dst_resource = static_cast<ID3D12Resource *>(region.dst_buffer->GetResource()->
        GetResource());
    auto *src_resource = static_cast<ID3D12Resource *>(region.src_buffer->GetResource()->
        GetResource());
    m_list->CopyBufferRegion(dst_resource, region.dst_offset, src_resource, region.src_offset,
                             region.size);
}

void Swift::D3D12::Command::CopyTextureRegion(const TextureCopyRegion &region)
{
    auto *dst_resource = static_cast<ID3D12Resource *>(region.dst_texture->GetResource()->
        GetResource());
    auto *src_resource = static_cast<ID3D12Resource *>(region.src_texture->GetResource()->
        GetResource());

    TransitionResource(region.dst_texture->GetResource(), ResourceState::eCopyDest);
    TransitionResource(region.src_texture->GetResource(), ResourceState::eCopySource);

    const D3D12_TEXTURE_COPY_LOCATION src_location = {
        .pResource = src_resource,
        .Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX,
        .SubresourceIndex = 0,
    };

    const D3D12_TEXTURE_COPY_LOCATION dst_location = {
        .pResource = dst_resource,
        .Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX,
        .SubresourceIndex = 0,
    };

    const D3D12_BOX src_box = {
        .left = region.src_region.offset[0],
        .top = region.src_region.offset[1],
        .front = region.src_region.offset[2],
        .right = region.src_region.size[0],
        .bottom = region.src_region.size[1],
        .back = region.src_region.size[2],
    };

    m_list->CopyTextureRegion(&dst_location, region.dst_offset[0], region.dst_offset[1],
                              region.dst_offset[2], &src_location, &src_box);
}

void Swift::D3D12::Command::BindRenderTargets(
    const std::span<const std::shared_ptr<ITexture>> render_targets,
    const std::shared_ptr<ITexture> &depth_stencil)
{
    std::array<D3D12_CPU_DESCRIPTOR_HANDLE, 8> render_target_descriptors;
    for (int i = 0; i < render_targets.size(); i++)
    {
        const auto *dx_render_target = static_cast<Texture *>(render_targets[i].get());
        render_target_descriptors[i] = dx_render_target->GetRTVDescriptor()->cpu_handle;
    }

    if (depth_stencil)
    {
        const auto *dx_depth_stencil = static_cast<Texture *>(depth_stencil.get());
        m_list->OMSetRenderTargets(static_cast<uint32_t>(render_targets.size()),
                                   render_target_descriptors.data(),
                                   false, &dx_depth_stencil->GetDSVDescriptor()->cpu_handle);
    } else
    {
        m_list->OMSetRenderTargets(static_cast<uint32_t>(render_targets.size()),
                                   render_target_descriptors.data(),
                                   false, nullptr);
    }
}

void Swift::D3D12::Command::ClearRenderTarget(const std::shared_ptr<ITexture> &texture,
                                              const std::array<float, 4> &color)
{
    const auto *dx_texture = static_cast<Texture *>(texture.get());
    TransitionResource(dx_texture->GetResource(), ResourceState::eRenderTarget);
    m_list->ClearRenderTargetView(dx_texture->GetRTVDescriptor()->cpu_handle, color.data(),
                                  0, nullptr);
}

void Swift::D3D12::Command::ClearDepthStencil(const std::shared_ptr<ITexture> &texture, float depth,
                                              const uint8_t stencil)
{
    const auto *dx_texture = static_cast<Texture *>(texture.get());
    TransitionResource(texture->GetResource(), ResourceState::eDepthWrite);
    m_list->ClearDepthStencilView(dx_texture->GetDSVDescriptor()->cpu_handle,
                                  D3D12_CLEAR_FLAG_DEPTH /*TODO: Handle stencil later*/, depth,
                                  stencil, 0, nullptr);
}

void Swift::D3D12::Command::TransitionResource(const std::shared_ptr<IResource> &resource_handle,
                                               const ResourceState new_state)
{
    auto *dx_resource = static_cast<Resource *>(resource_handle.get());
    const auto new_dx_state = ToResourceState(new_state);
    if (dx_resource->GetState() == new_state) return;
    const auto barrier = D3D12_RESOURCE_BARRIER{
        .Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION,
        .Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE,
        .Transition = {
            .pResource = static_cast<ID3D12Resource *>(dx_resource->GetResource()),
            .Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES,
            .StateBefore = ToResourceState(dx_resource->GetState()),
            .StateAfter = new_dx_state,
        }
    };
    dx_resource->SetState(new_state);
    m_list->ResourceBarrier(1, &barrier);
}
