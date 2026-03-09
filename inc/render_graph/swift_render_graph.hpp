#pragma once
#include "swift_buffer_view.hpp"
#include "swift_texture_view.hpp"
#include "swift_command.hpp"
#include "vector"
#include "variant"
#include "functional"
#include "unordered_map"

namespace Swift::RG
{
    struct ResourceHandle
    {
        ResourceHandle() : view(std::monostate{}) {};
        template <typename T, typename = std::enable_if_t<std::is_same_v<T, ITextureView*> || std::is_same_v<T, IBufferView*>>>
        ResourceHandle(T v) : view(v)
        {
        }
        std::variant<std::monostate, ITextureView*, IBufferView*> view;
    };

    class RenderNode
    {
    public:
        RenderNode() = default;
        RenderNode(IShader* shader, ICommand* command) : m_shader(shader), m_command(command) {}
        RenderNode& WriteRenderTarget(const ResourceHandle& resource_handle)
        {
            m_render_target_handle = resource_handle;
            return *this;
        }
        RenderNode& WriteDepthStencil(const ResourceHandle& resource_handle)
        {
            m_depth_stencil_handle = resource_handle;
            return *this;
        }
        RenderNode& Read(ResourceHandle resource_handle)
        {
            m_input_resources.emplace_back(resource_handle);
            return *this;
        }
        RenderNode& Write(ResourceHandle resource_handle)
        {
            m_output_resources.emplace_back(resource_handle);
            return *this;
        }
        RenderNode& SetRenderExtents(const Float2 extents)
        {
            m_dimensions = extents;
            return *this;
        }
        RenderNode& SetOffset(const Float2 offset)
        {
            m_offset = offset;
            return *this;
        }
        RenderNode& SetDepthRange(const Float2 depth_range)
        {
            m_depth_range = depth_range;
            return *this;
        }
        RenderNode& SetExecute(const std::function<void(ICommand*)>& execute)
        {
            m_execute = execute;
            return *this;
        }
        RenderNode& SetRenderLoadOp(const LoadOp load_op)
        {
            m_render_load_op = load_op;
            return *this;
        }
        RenderNode& SetRenderStoreOp(const StoreOp store_op)
        {
            m_render_store_op = store_op;
            return *this;
        }
        RenderNode& SetDepthLoadOp(const LoadOp load_op)
        {
            m_depth_load_op = load_op;
            return *this;
        }
        RenderNode& SetDepthStoreOp(const StoreOp store_op)
        {
            m_depth_store_op = store_op;
            return *this;
        }
        RenderNode& SetClearColor(const Float4 color)
        {
            m_clear_color = color;
            return *this;
        }
        RenderNode& SetClearDepth(const float depth)
        {
            m_clear_depth = depth;
            return *this;
        }
        RenderNode& SetClearStencil(const uint8_t stencil)
        {
            m_clear_stencil = stencil;
            return *this;
        }

    private:
        friend class RenderGraph;
        std::function<void(ICommand*)> m_execute;
        IShader* m_shader;
        ICommand* m_command;
        Float2 m_dimensions{};
        Float2 m_offset{};
        Float2 m_depth_range{0, 1};
        LoadOp m_render_load_op = LoadOp::eLoad;
        StoreOp m_render_store_op = StoreOp::eStore;
        LoadOp m_depth_load_op = LoadOp::eLoad;
        StoreOp m_depth_store_op = StoreOp::eStore;
        Float4 m_clear_color{};
        float m_clear_depth = 1.0f;
        uint8_t m_clear_stencil = 0;
        std::vector<ResourceHandle> m_input_resources;
        std::vector<ResourceHandle> m_output_resources;
        ResourceHandle m_render_target_handle{};
        ResourceHandle m_depth_stencil_handle{};
    };

    class CopyNode
    {
    public:
        CopyNode() = default;
        CopyNode& SetSrcBuffer(IBuffer* buffer)
        {
            m_src_resource = buffer;
            return *this;
        }
        CopyNode& SetDstBuffer(IBuffer* buffer)
        {
            m_dst_resource = buffer;
            return *this;
        }
        CopyNode& SetSrcTexture(ITexture* texture)
        {
            m_src_resource = texture;
            return *this;
        }
        CopyNode& SetDstTexture(ITexture* texture)
        {
            m_dst_resource = texture;
            return *this;
        }
        CopyNode& SetSrcMip(const uint32_t src_mip)
        {
            m_src_mip = src_mip;
            return *this;
        }
        CopyNode& SetDstMip(const uint32_t dst_mip)
        {
            m_dst_mip = dst_mip;
            return *this;
        }
        CopyNode& SetSrcOffset(const UInt3 offset)
        {
            m_src_offset = offset;
            return *this;
        }
        CopyNode& SetDstOffset(const UInt3 offset)
        {
            m_dst_offset = offset;
            return *this;
        }
        CopyNode& SetSrcOffset(const uint32_t offset)
        {
            m_src_buffer_offset = offset;
            return *this;
        }
        CopyNode& SetDstOffset(const uint32_t offset)
        {
            m_dst_buffer_offset = offset;
            return *this;
        }
        CopyNode& SetSize(const UInt3 size)
        {
            m_size = size;
            return *this;
        }
        CopyNode& SetSize(const uint32_t size)
        {
            m_buffer_size = size;
            return *this;
        }
        CopyNode& SetMipLevels(const uint32_t mip_levels)
        {
            m_mip_levels = mip_levels;
            return *this;
        }
        CopyNode& SetArraySize(const uint32_t array_size)
        {
            m_array_size = array_size;
            return *this;
        }


    private:
        friend class RenderGraph;
        std::variant<ITexture*, IBuffer*> m_src_resource;
        std::variant<ITexture*, IBuffer*> m_dst_resource;
        uint32_t m_src_mip = 0;
        uint32_t m_dst_mip = 0;
        UInt3 m_src_offset;
        UInt3 m_dst_offset;
        UInt3 m_size;
        uint32_t m_src_buffer_offset;
        uint32_t m_dst_buffer_offset;
        uint32_t m_buffer_size;
        uint32_t m_mip_levels;
        uint32_t m_array_size;
    };

    class RenderGraph
    {
    public:
        RenderGraph();
        void NewFrame(ICommand* command)
        {
            m_command = command;
            m_nodes.clear();
        }
        RenderNode& AddPass(const std::string& name, IShader* shader)
        {
            m_nodes[name] = RenderNode(shader, m_command);
            return std::get<RenderNode>(m_nodes[name]);
        }
        CopyNode& AddCopyPass(const std::string& name)
        {
            m_nodes[name] = CopyNode();
            return std::get<CopyNode>(m_nodes[name]);
        }
        void Execute();

    private:
        std::unordered_map<std::string, std::variant<RenderNode, CopyNode>> m_nodes;
        std::vector<ITextureView*> m_render_targets;
        ICommand* m_command = nullptr;
    };
}  // namespace Swift::RG