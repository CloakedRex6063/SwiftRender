#include "d3d12/d3d12_command.hpp"
#include "d3d12/d3d12_helpers.hpp"
#include "swift_buffer.hpp"
#include "swift_context.hpp"
#include "swift_texture.hpp"
#include "d3d12/d3d12_shader.hpp"
#include "d3d12/d3d12_texture.hpp"

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
    }
}

void Swift::D3D12::Command::End() { m_list->Close(); }

void Swift::D3D12::Command::SetViewport(const Viewport& viewport)
{
    const D3D12_VIEWPORT dx_viewport = {viewport.offset[0],
                                        viewport.offset[1],
                                        viewport.dimensions[0],
                                        viewport.dimensions[1],
                                        viewport.depth_range[0],
                                        viewport.depth_range[1]};
    m_list->RSSetViewports(1, &dx_viewport);
}

void Swift::D3D12::Command::SetScissor(const Scissor& scissor)
{
    const D3D12_RECT dx_scissor = {static_cast<int>(scissor.offset[0]),
                                   static_cast<int>(scissor.offset[1]),
                                   static_cast<int>(scissor.dimensions[0]),
                                   static_cast<int>(scissor.dimensions[1])};
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

    std::vector<D3D12_PLACED_SUBRESOURCE_FOOTPRINT> layouts(texture->GetMipLevels() * texture->GetArraySize());
    std::vector<uint32_t> num_rows(texture->GetMipLevels() * texture->GetArraySize());
    std::vector<uint64_t> row_size_in_bytes(texture->GetMipLevels() * texture->GetArraySize());
    uint64_t total_bytes = 0;

    // TODO: Handle msaa
    constexpr auto sample_desc = DXGI_SAMPLE_DESC{1, 0};

    const D3D12_RESOURCE_DESC texture_desc = {
        .Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D,
        .Alignment = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT,
        .Width = texture->GetSize()[0],
        .Height = texture->GetSize()[1],
        .DepthOrArraySize = static_cast<uint16_t>(texture->GetArraySize()),
        .MipLevels = mip_levels,
        .Format = ToDXGIFormat(texture->GetFormat()),
        .SampleDesc = sample_desc,
        .Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN,
    };

    device->GetCopyableFootprints(&texture_desc,
                                  0,
                                  mip_levels * array_size,
                                  0,
                                  layouts.data(),
                                  num_rows.data(),
                                  row_size_in_bytes.data(),
                                  &total_bytes);

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
void Swift::D3D12::Command::CopyImageToImage(ITexture* src_resource, ITexture* dst_resource)
{
    auto* src = static_cast<ID3D12Resource*>(src_resource->GetResource());
    auto* dst = static_cast<ID3D12Resource*>(dst_resource->GetResource());
    m_list->CopyResource(dst, src);
}

void Swift::D3D12::Command::CopyBufferRegion(const BufferCopyRegion& region)
{
    auto* dst_resource = static_cast<ID3D12Resource*>(region.dst_buffer->GetResource());
    auto* src_resource = static_cast<ID3D12Resource*>(region.src_buffer->GetResource());
    m_list->CopyBufferRegion(dst_resource, region.dst_offset, src_resource, region.src_offset, region.size);
}

void Swift::D3D12::Command::CopyTextureRegion(const TextureCopyRegion& region)
{
    auto* dst_resource = static_cast<ID3D12Resource*>(region.dst_texture->GetResource());
    auto* src_resource = static_cast<ID3D12Resource*>(region.src_texture->GetResource());

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

    m_list->CopyTextureRegion(&dst_location,
                              region.dst_offset[0],
                              region.dst_offset[1],
                              region.dst_offset[2],
                              &src_location,
                              &src_box);
}

void Swift::D3D12::Command::BindConstantBuffer(IBuffer* buffer, const uint32_t slot)
{
    m_list->SetGraphicsRootConstantBufferView(slot, buffer->GetVirtualAddress());
    m_list->SetComputeRootConstantBufferView(slot, buffer->GetVirtualAddress());
}

void Swift::D3D12::Command::BindRenderTargets(const std::span<IRenderTarget*> render_targets, IDepthStencil* depth_stencil)
{
    std::array<D3D12_CPU_DESCRIPTOR_HANDLE, 8> render_target_descriptors;
    for (int i = 0; i < render_targets.size(); i++)
    {
        auto* dx_render_target = static_cast<RenderTarget*>(render_targets[i]);
        render_target_descriptors[i] = dx_render_target->GetDescriptorData().cpu_handle;
    }

    if (depth_stencil)
    {
        auto* dx_depth_stencil = static_cast<DepthStencil*>(depth_stencil);
        m_list->OMSetRenderTargets(static_cast<uint32_t>(render_targets.size()),
                                   render_target_descriptors.data(),
                                   false,
                                   &dx_depth_stencil->GetDescriptorData().cpu_handle);
    }
    else
    {
        m_list->OMSetRenderTargets(static_cast<uint32_t>(render_targets.size()),
                                   render_target_descriptors.data(),
                                   false,
                                   nullptr);
    }
}

void Swift::D3D12::Command::ClearRenderTarget(IRenderTarget* render_target, const std::array<float, 4>& color)
{
    auto* dx_render_target = static_cast<RenderTarget*>(render_target);
    m_list->ClearRenderTargetView(dx_render_target->GetDescriptorData().cpu_handle, color.data(), 0, nullptr);
}

void Swift::D3D12::Command::ClearDepthStencil(IDepthStencil* depth_stencil, const float depth, const uint8_t stencil)
{
    auto* dx_depth_stencil = static_cast<DepthStencil*>(depth_stencil);
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