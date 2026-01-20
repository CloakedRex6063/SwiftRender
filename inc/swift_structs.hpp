#pragma once
#include "string"
#include "memory"
#include "optional"
#include "enum_flags.hpp"
#include "vector"
#include "array"

namespace Swift
{
    class IResource;
    class IBuffer;
    class ITexture;

    enum class BackendType
    {
#ifdef SWIFT_WINDOWS
        eD3D12,
#endif
        eVulkan,
    };

    enum class QueueType : uint8_t
    {
        eGraphics,
        eCompute,
        eTransfer,
    };

    enum class QueuePriority : uint8_t
    {
        eHigh,
        eNormal,
    };

    enum class Format : uint32_t
    {
        eRGBA8_UNORM,
        eRGBA16F,
        eRGBA32F,
        eD32F,
    };

    struct MSAA
    {
        uint32_t samples;
        uint32_t quality;
    };

    enum class BufferType : uint32_t
    {
        eNone,
        eReadback,
        eConstantBuffer,
        eStructuredBuffer,
    };

    enum class TextureFlags : uint32_t
    {
        eNone = 0,
        eRenderTarget = 1 << 1,
        eShaderResource = 1 << 2,
        eDepthStencil = 1 << 3,
    };

    struct ContextCreateInfo
    {
        BackendType backend_type;
        uint32_t width;
        uint32_t height;
        void* native_window_handle;
        void* native_display_handle;
    };

    enum class PolygonMode
    {
        ePoint,
        eLine,
        eTriangle
    };

    enum class FrontFace
    {
        eClockwise,
        eCounterClockwise,
    };

    enum class CullMode
    {
        eBack,
        eFront,
        eNone
    };

    enum class FillMode
    {
        eWireframe,
        eSolid
    };

    enum class ShaderVisibility
    {
        eAll,
        eVertex,
        eHull,
        eDomain,
        eGeometry,
        ePixel,
        eAmplification,
        eMesh
    };

    enum class DescriptorType
    {
        eConstant,
        eShaderResource,
        eUnorderedAccess,
    };

    struct Descriptor
    {
        uint32_t shader_register = 0;
        uint32_t register_space = 0;
        DescriptorType descriptor_type = DescriptorType::eConstant;
        ShaderVisibility shader_visibility = ShaderVisibility::eAll;
    };

    enum class Filter
    {
        eNearest,
        eLinear,
        eNearestMipNearest,
        eLinearMipNearest,
        eNearestMipLinear,
        eLinearMipLinear,
    };

    enum class Wrap
    {
        eRepeat,
        eMirroredRepeat,
        eClampToEdge,
    };

    struct SamplerDescriptor
    {
        Filter min_filter;
        Filter mag_filter;
        Wrap wrap_u;
        Wrap wrap_y;
        Wrap wrap_w;
    };

    enum class DepthTest
    {
        eAlways,
        eNever,
        eLess,
        eLessEqual,
        eGreater,
        eGreaterEqual,
        eEqual,
        eNotEqual,
    };

    struct DepthStencilState
    {
        bool depth_enable;
        bool depth_write_enable;
        DepthTest depth_test;
        bool stencil_enable;
    };

    struct RasterizerState
    {
        FillMode fill_mode = FillMode::eSolid;
        CullMode cull_mode = CullMode::eBack;
        FrontFace front_face = FrontFace::eCounterClockwise;
    };

    struct GraphicsShaderCreateInfo
    {
        std::vector<Format> rtv_formats;
        std::optional<Format> dsv_format;
        std::vector<uint8_t> amplify_code;
        std::vector<uint8_t> mesh_code;
        std::vector<uint8_t> pixel_code;
        DepthStencilState depth_stencil_state;
        RasterizerState rasterizer_state;
        PolygonMode polygon_mode = Swift::PolygonMode::eTriangle;
        std::vector<Descriptor> descriptors;
        std::vector<SamplerDescriptor> static_samplers;
    };

    struct ComputeShaderCreateInfo
    {
        std::vector<uint8_t> code;
        std::vector<Descriptor> descriptors;
        std::vector<SamplerDescriptor> static_samplers;
    };

    enum class ResourceState
    {
        eCommon,
        eConstant,
        eUnorderedAccess,
        eIndexBuffer,
        eRenderTarget,
        eDepthWrite,
        eDepthRead,
        eShaderResource,
        ePresent,
        eCopyDest,
        eCopySource,
    };

    struct Viewport
    {
        std::array<float, 2> dimensions{};
        std::array<float, 2> offset{};
        std::array<float, 2> depth_range = {0.0f, 1.0f};
    };

    struct Scissor
    {
        std::array<uint32_t, 2> dimensions;
        std::array<uint32_t, 2> offset;
    };

    struct BufferCopyRegion
    {
        std::shared_ptr<IBuffer> src_buffer;
        std::shared_ptr<IBuffer> dst_buffer;
        uint64_t src_offset;
        uint64_t dst_offset;
        uint64_t size;
    };

    struct BufferTextureCopyRegion
    {
        std::shared_ptr<IBuffer> src_buffer;
        std::shared_ptr<ITexture> dst_texture;
    };

    struct TextureRegion
    {
        std::array<uint32_t, 3> size;
        std::array<uint32_t, 3> offset;
    };

    struct TextureCopyRegion
    {
        std::shared_ptr<ITexture> src_texture;
        std::shared_ptr<ITexture> dst_texture;
        TextureRegion src_region;
        std::array<uint32_t, 3> dst_offset;
    };

    struct TextureCreateInfo
    {
        uint32_t width = 0;
        uint32_t height = 0;
        uint16_t mip_levels = 1;
        uint16_t array_size = 1;
        Format format = Format::eRGBA8_UNORM;
        EnumFlags<TextureFlags> flags = TextureFlags::eNone;
        const void* data = nullptr;
        std::optional<MSAA> msaa = std::nullopt;
        bool gen_mipmaps = false;
        std::shared_ptr<IResource> resource = nullptr;
    };

    struct BufferCreateInfo
    {
        uint32_t num_elements = 0;
        uint32_t element_size = 0;
        uint32_t first_element = 0;
        const void* data = nullptr;
        BufferType type = BufferType::eConstantBuffer;
        std::shared_ptr<IResource> resource = nullptr;
    };

    struct QueueCreateInfo
    {
        QueueType type;
        QueuePriority priority;
    };

    enum class ShaderError
    {
        eNone,
        eInvalidFile,
        eCompileError,
    };

    enum class AlphaMode : uint32_t
    {
        eOpaque,
        eTransparent,
    };

    struct AdapterDescription
    {
        std::string name;
        float system_memory;
        float dedicated_video_memory;
    };
}  // namespace Swift
