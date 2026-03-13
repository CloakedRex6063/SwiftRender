#include "render_graph/swift_render_graph.hpp"
#include "ranges"

Swift::RG::RenderGraph::RenderGraph() {}

template <class... Ts>
struct overloads : Ts...
{
    using Ts::operator()...;
};

void Swift::RG::RenderGraph::Execute()
{
    if (!m_command) return;
    for (auto& var_node : m_nodes | std::views::values)
    {
        std::visit(
            overloads{
                [&](RenderNode& node)
                {
                    if (node.m_dimensions.x != 0 && node.m_dimensions.y != 0 && node.m_depth_range.y != 0)
                    {
                        m_command->SetViewport(Viewport{
                            .dimensions = node.m_dimensions,
                            .offset = node.m_offset,
                            .depth_range = node.m_depth_range,
                        });
                        m_command->SetScissor(Scissor{
                            .dimensions = UInt2(node.m_dimensions.x, node.m_dimensions.y),
                            .offset = UInt2(node.m_offset.x, node.m_offset.y),
                        });
                    }
                    m_command->BindShader(node.m_shader);

                    for (auto& input : node.m_input_resources)
                    {
                        std::visit(
                            [&](auto&& view)
                            {
                                using T = std::decay_t<decltype(view)>;
                                if constexpr (std::is_same_v<T, ITextureView*>)
                                {
                                    m_command->TransitionImage(T(view)->GetTexture(), ResourceState::eShaderResource);
                                }
                                else if constexpr (std::is_same_v<T, IBufferView*>)
                                {
                                    m_command->TransitionBuffer(T(view)->GetBuffer(), ResourceState::eShaderResource);
                                }
                            },
                            input.view);
                    }

                    std::optional<RenderAttachmentInfo> color_attachment_info{std::nullopt};
                    if (std::holds_alternative<ITextureView*>(node.m_render_target_handle.view))
                    {
                        m_command->TransitionImage(std::get<ITextureView*>(node.m_render_target_handle.view)->GetTexture(),
                                                   ResourceState::eRenderTarget);
                        color_attachment_info = RenderAttachmentInfo{
                            .render_target = std::get<ITextureView*>(node.m_render_target_handle.view),
                            .load_op = node.m_render_load_op,
                            .store_op = node.m_render_store_op,
                            .clear_color = node.m_clear_color,
                        };
                    }

                    std::optional<DepthAttachmentInfo> depth_attachment_info{std::nullopt};
                    if (std::holds_alternative<ITextureView*>(node.m_depth_stencil_handle.view))
                    {
                        m_command->TransitionImage(std::get<ITextureView*>(node.m_depth_stencil_handle.view)->GetTexture(),
                                                   ResourceState::eDepthWrite);
                        depth_attachment_info = {
                            .depth_stencil = std::get<ITextureView*>(node.m_depth_stencil_handle.view),
                            .load_op = node.m_depth_load_op,
                            .store_op = node.m_depth_store_op,
                            .clear_depth = node.m_clear_depth,
                            .clear_stencil = node.m_clear_stencil,
                        };
                    }
                    m_command->BeginRender(color_attachment_info, depth_attachment_info);
                    node.m_execute(m_command);
                    m_command->EndRender();
                },
                [&](CopyNode& node)
                {
                    std::visit(
                        overloads{
                            [&](ITexture* src, ITexture* dst)
                            {
                                m_command->CopyTextureToTexture(src,
                                                                dst,
                                                                TextureCopyRegion{
                                                                    .src_mip = node.m_src_mip,
                                                                    .dst_mip = node.m_dst_mip,
                                                                    .src_offset = node.m_src_offset,
                                                                    .dst_offset = node.m_dst_offset,
                                                                    .size = node.m_size,
                                                                });
                            },
                            [&](IBuffer* src, IBuffer* dst)
                            {
                                m_command->CopyBufferToBuffer(src,
                                                              dst,
                                                              BufferCopyRegion{
                                                                  .src_offset = node.m_src_buffer_offset,
                                                                  .dst_offset = node.m_dst_buffer_offset,
                                                                  .size = node.m_buffer_size,
                                                              });
                            },
                            [&](IBuffer* src, ITexture* dst)
                            { m_command->CopyBufferToTexture(src, dst, node.m_mip_levels, node.m_array_size); },
                            [&](ITexture* /*src*/, IBuffer* /*dst*/)
                            {

                            },
                        },
                        node.m_src_resource,
                        node.m_dst_resource);
                }},
            var_node);
    }
}