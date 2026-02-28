#include "d3d12/d3d12_command.hpp"
#include "d3d12/d3d12_helpers.hpp"
#include "swift_buffer.hpp"
#include "d3d12/d3d12_context.hpp"
#include "swift_texture.hpp"
#include "d3d12/d3d12_shader.hpp"
#include "array"
#include "d3d12/d3d12_texture_view.hpp"

Swift::D3D12::Command::Command(IContext* context,
                               DescriptorHeap* cbv_heap,
                               DescriptorHeap* sampler_heap,
                               ID3D12RootSignature* root_signature,
                               const QueueType type,
                               std::string_view debug_name)
    : m_context(static_cast<Context*>(context)),
      m_type(type),
      m_cbv_srv_uav_heap(cbv_heap),
      m_sampler_heap(sampler_heap),
      m_root_signature(root_signature)
{
    auto* const device = static_cast<ID3D12Device14*>(context->GetDevice());

    device->CreateCommandAllocator(ToCommandType(type), IID_PPV_ARGS(&m_allocator));
    device->CreateCommandList(0, ToCommandType(type), m_allocator, nullptr, IID_PPV_ARGS(&m_list));

    std::wstring name{debug_name.begin(), debug_name.end()};
    m_list->SetName(name.c_str());

    m_list->Close();
}

Swift::D3D12::Command::~Command()
{
    m_allocator->Release();
    m_list->Release();
}

void Swift::D3D12::Command::Begin()
{
    m_allocator->Reset();

    m_list->Reset(m_allocator, nullptr);

    if (m_list->GetType() != D3D12_COMMAND_LIST_TYPE_COPY)
    {
        const auto descriptor_heaps = std::array{m_cbv_srv_uav_heap->GetHeap(), m_sampler_heap->GetHeap()};
        m_list->SetDescriptorHeaps(2, descriptor_heaps.data());
        m_list->SetGraphicsRootSignature(m_root_signature);
        m_list->SetComputeRootSignature(m_root_signature);

        m_list->SetGraphicsRootDescriptorTable(4, m_cbv_srv_uav_heap->GetHeap()->GetGPUDescriptorHandleForHeapStart());
        m_list->SetGraphicsRootDescriptorTable(5, m_sampler_heap->GetHeap()->GetGPUDescriptorHandleForHeapStart());
    }
}

void Swift::D3D12::Command::End() { m_list->Close(); }

void Swift::D3D12::Command::SetViewport(const Viewport& viewport)
{
    const D3D12_VIEWPORT dx_viewport = {
        viewport.offset.x,
        viewport.offset.y,
        viewport.dimensions.x,
        viewport.dimensions.y,
        viewport.depth_range.x,
        viewport.depth_range.y,
    };
    m_list->RSSetViewports(1, &dx_viewport);
}

void Swift::D3D12::Command::SetScissor(const Scissor& scissor)
{
    const D3D12_RECT dx_scissor = {static_cast<int>(scissor.offset.x),
                                   static_cast<int>(scissor.offset.y),
                                   static_cast<int>(scissor.dimensions.x),
                                   static_cast<int>(scissor.dimensions.y)};
    m_list->RSSetScissorRects(1, &dx_scissor);
}

void Swift::D3D12::Command::PushConstants(const void* data, const uint32_t size, const uint32_t offset)
{
    switch (m_shader->GetShaderType())
    {
        case ShaderType::eGraphics:
            m_list->SetGraphicsRoot32BitConstants(0, size / sizeof(uint32_t), data, offset);
            break;
        case ShaderType::eCompute:
            m_list->SetComputeRoot32BitConstants(0, size / sizeof(uint32_t), data, offset);
            break;
    }
}

void Swift::D3D12::Command::BindShader(IShader* shader)
{
    m_shader = shader;
    m_list->SetPipelineState(static_cast<ID3D12PipelineState*>(shader->GetPipeline()));
}

void Swift::D3D12::Command::DispatchMesh(const uint32_t group_x, const uint32_t group_y, const uint32_t group_z)
{
    m_list->DispatchMesh(group_x, group_y, group_z);
}

void Swift::D3D12::Command::DispatchCompute(const uint32_t group_x, const uint32_t group_y, const uint32_t group_z)
{
    m_list->Dispatch(group_x, group_y, group_z);
}

void Swift::D3D12::Command::CopyBufferToTexture(IContext* context,
                                                IBuffer* buffer,
                                                ITexture* texture,
                                                const uint16_t mip_levels,
                                                const uint16_t array_size)
{
    auto* dst_resource = static_cast<ID3D12Resource*>(texture->GetResource());
    auto* src_resource = static_cast<ID3D12Resource*>(buffer->GetResource());

    auto* const device = static_cast<ID3D12Device14*>(context->GetDevice());

    auto [layouts, num_rows, row_size_in_bytes, total_bytes] = GetTextureCopyData(device, texture->GetCreateInfo());

    for (uint32_t array_slice = 0; array_slice < array_size; ++array_slice)
    {
        for (uint32_t mip_level = 0; mip_level < mip_levels; ++mip_level)
        {
            const uint32_t subresource_index = mip_level + array_slice * mip_levels;

            const D3D12_TEXTURE_COPY_LOCATION src_location = {
                .pResource = src_resource,
                .Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT,
                .PlacedFootprint = layouts[subresource_index],
            };

            const D3D12_TEXTURE_COPY_LOCATION dst_location = {
                .pResource = dst_resource,
                .Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX,
                .SubresourceIndex = subresource_index,
            };

            m_list->CopyTextureRegion(&dst_location, 0, 0, 0, &src_location, nullptr);
        }
    }
}

void Swift::D3D12::Command::CopyBufferRegion(const BufferCopyRegion& region)
{
    auto* dst_resource = static_cast<ID3D12Resource*>(region.dst_buffer->GetResource());
    auto* src_resource = static_cast<ID3D12Resource*>(region.src_buffer->GetResource());
    m_list->CopyBufferRegion(dst_resource, region.dst_offset, src_resource, region.src_offset, region.size);
}

void Swift::D3D12::Command::BindConstantBuffer(IBuffer* buffer, const uint32_t slot)
{
    m_list->SetGraphicsRootConstantBufferView(slot, buffer->GetVirtualAddress());
    m_list->SetComputeRootConstantBufferView(slot, buffer->GetVirtualAddress());
}

void Swift::D3D12::Command::BeginRender(const std::span<const ColorAttachmentInfo> color_attachments,
                                        const std::optional<const DepthAttachmentInfo>& depth_attachment)
{
    std::array<D3D12_RENDER_PASS_RENDER_TARGET_DESC, 8> render_target_descriptors;
    for (int i = 0; i < color_attachments.size(); i++)
    {
        auto [render_target, load_op, store_op, clear_color] = color_attachments[i];
        render_target_descriptors[i] = D3D12_RENDER_PASS_RENDER_TARGET_DESC{
            .cpuDescriptor = static_cast<TextureView*>(render_target)->GetDescriptorData().cpu_handle,
            .BeginningAccess = ToBeginAccess(render_target->GetTexture()->GetFormat(), load_op, clear_color),
            .EndingAccess = ToEndAccess(store_op),
        };
    }
    const D3D12_RENDER_PASS_DEPTH_STENCIL_DESC* depth_desc = nullptr;
    if (depth_attachment.has_value())
    {
        auto [depth_stencil, load_op, store_op, clear_depth, clear_stencil] = depth_attachment.value();
        const D3D12_RENDER_PASS_DEPTH_STENCIL_DESC depth_stencil_desc{
            .cpuDescriptor = static_cast<TextureView*>(depth_stencil)->GetDescriptorData().cpu_handle,
            .DepthBeginningAccess =
                ToBeginAccess(depth_stencil->GetTexture()->GetFormat(), load_op, clear_depth, clear_stencil),
            .StencilBeginningAccess =
                {D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE_NO_ACCESS},  // TODO: handle stencil based on format
            .DepthEndingAccess = ToEndAccess(store_op),
            .StencilEndingAccess = {D3D12_RENDER_PASS_ENDING_ACCESS_TYPE_NO_ACCESS},  // TODO: handle stencil based on format
        };
        depth_desc = &depth_stencil_desc;
    }
    m_list->BeginRenderPass(color_attachments.size(),
                            render_target_descriptors.data(),
                            depth_desc,
                            D3D12_RENDER_PASS_FLAG_NONE);
}

void Swift::D3D12::Command::EndRender() { m_list->EndRenderPass(); }

void Swift::D3D12::Command::ClearRenderTarget(ITextureView* render_target, const Float4& color)
{
    auto* dx_render_target = static_cast<TextureView*>(render_target);
    m_list->ClearRenderTargetView(dx_render_target->GetDescriptorData().cpu_handle,
                                  reinterpret_cast<const float*>(&color),
                                  0,
                                  nullptr);
}

void Swift::D3D12::Command::ClearDepthStencil(ITextureView* depth_stencil, const float depth, const uint8_t stencil)
{
    auto* dx_depth_stencil = static_cast<TextureView*>(depth_stencil);
    m_list->ClearDepthStencilView(dx_depth_stencil->GetDescriptorData().cpu_handle,
                                  D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL,
                                  depth,
                                  stencil,
                                  0,
                                  nullptr);
}

void Swift::D3D12::Command::TransitionImage(ITexture* image, const ResourceState new_state)
{
    const auto new_dx_state = ToResourceState(new_state);
    if (image->GetState() == new_state) return;
    const auto barrier = D3D12_RESOURCE_BARRIER{.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION,
                                                .Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE,
                                                .Transition = {
                                                    .pResource = static_cast<ID3D12Resource*>(image->GetResource()),
                                                    .Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES,
                                                    .StateBefore = ToResourceState(image->GetState()),
                                                    .StateAfter = new_dx_state,
                                                }};
    image->SetState(new_state);
    m_list->ResourceBarrier(1, &barrier);
}

void Swift::D3D12::Command::TransitionBuffer(IBuffer* buffer, ResourceState new_state)
{
    const auto new_dx_state = ToResourceState(new_state);
    if (buffer->GetState() == new_state) return;
    const auto barrier = D3D12_RESOURCE_BARRIER{.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION,
                                                .Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE,
                                                .Transition = {
                                                    .pResource = static_cast<ID3D12Resource*>(buffer->GetResource()),
                                                    .Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES,
                                                    .StateBefore = ToResourceState(buffer->GetState()),
                                                    .StateAfter = new_dx_state,
                                                }};
    buffer->SetState(new_state);
    m_list->ResourceBarrier(1, &barrier);
}
void Swift::D3D12::Command::UAVBarrier(IBuffer* buffer)
{
    const auto barrier = D3D12_RESOURCE_BARRIER{.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV,
                                                .Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE,
                                                .UAV = {
                                                    .pResource = static_cast<ID3D12Resource*>(buffer->GetResource()),
                                                }};
    m_list->ResourceBarrier(1, &barrier);
}

void Swift::D3D12::Command::UAVBarrier(ITexture* texture)
{
    const auto barrier = D3D12_RESOURCE_BARRIER{.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV,
                                                .Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE,
                                                .UAV = {
                                                    .pResource = static_cast<ID3D12Resource*>(texture->GetResource()),
                                                }};
    m_list->ResourceBarrier(1, &barrier);
}