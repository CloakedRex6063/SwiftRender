#pragma once
#include "string"
#include "memory"
#include "optional"
#include "enum_flags.hpp"
#include "vector"
#include "array"
#include "span"

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

        eBC1_UNORM,
        eBC1_UNORM_SRGB,
        eBC2_UNORM,
        eBC2_UNORM_SRGB,
        eBC3_UNORM,
        eBC3_UNORM_SRGB,
        eBC4_UNORM,
        eBC4_SNORM,
        eBC5_UNORM,
        eBC5_SNORM,
        eBC6H_UF16,
        eBC6H_SF16,
        eBC7_UNORM,
        eBC7_UNORM_SRGB,
    };

    struct BufferSRVCreateInfo
    {
        uint32_t num_elements = 1;
        uint32_t element_size = 0;
        uint32_t first_element = 0;
    };

    using BufferUAVCreateInfo = BufferSRVCreateInfo;

    struct MSAA
    {
        uint32_t samples;
        uint32_t quality;
    };

    enum class BufferType : uint32_t
    {
        eDefault,
        eUpload,
        eReadback,
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

    enum class BorderColor
    {
        eBlack,
        eWhite,
    };

    enum class ComparisonFunc
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

    struct SamplerDescriptor
    {
        Filter min_filter = Filter::eLinear;
        Filter mag_filter = Filter::eLinear;
        Wrap wrap_u = Wrap::eRepeat;
        Wrap wrap_y = Wrap::eRepeat;
        Wrap wrap_w = Wrap::eRepeat;
        float min_lod = 0;
        float max_lod = 13;
        BorderColor border_color = BorderColor::eBlack;
        ComparisonFunc comparison_func = ComparisonFunc::eNever;
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
        int depth_bias = 0;
        float depth_bias_clamp = 0;
        float slope_scaled_depth_bias = 0;
        bool depth_clip_enable = false;
    };

    struct GraphicsShaderCreateInfo
    {
        std::span<const Format> rtv_formats;
        std::optional<Format> dsv_format;
        std::span<const uint8_t> amplify_code;
        std::span<const uint8_t> mesh_code;
        std::span<const uint8_t> pixel_code;
        DepthStencilState depth_stencil_state;
        RasterizerState rasterizer_state;
        PolygonMode polygon_mode = PolygonMode::eTriangle;
        std::span<const Descriptor> descriptors;
        std::span<const SamplerDescriptor> static_samplers;
        std::string_view name;
    };

    struct ComputeShaderCreateInfo
    {
        std::span<const uint8_t> code;
        std::span<const Descriptor> descriptors;
        std::span<const SamplerDescriptor> static_samplers;
        std::string_view name;
    };

    enum class HeapType
    {
        eUpload,
        eGPU,
        eGPU_Upload,
        eReadback
    };

    struct HeapCreateInfo
    {
        HeapType type;
        uint64_t size;
        std::string_view debug_name = " ";
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
        IBuffer* src_buffer;
        IBuffer* dst_buffer;
        uint64_t src_offset;
        uint64_t dst_offset;
        uint64_t size;
    };

    struct TextureRegion
    {
        std::array<uint32_t, 3> size;
        std::array<uint32_t, 3> offset;
        uint32_t mip_level;
    };

    struct TextureUpdateRegion
    {
        std::vector<TextureRegion> regions;
        const void* data;
    };

    struct TextureCopyRegion
    {
        ITexture* src_texture;
        ITexture* dst_texture;
        TextureRegion src_region;
        std::array<uint32_t, 3> dst_offset;
    };

    enum class TextureFlags
    {
        eNone = 0,
        eRenderTarget = 1 << 1,
        eDepthStencil = 1 << 2,
        eShaderResource = 1 << 3,
        eUnorderedAccess = 1 << 4,
    };

    struct TextureCreateInfo
    {
        uint32_t width = 0;
        uint32_t height = 0;
        uint16_t mip_levels = 1;
        uint16_t array_size = 1;
        Format format = Format::eRGBA8_UNORM;
        bool gen_mipmaps = false;
        const void* data = nullptr;
        std::optional<MSAA> msaa = std::nullopt;
        EnumFlags<TextureFlags> flags;
        std::shared_ptr<IResource> resource = nullptr;
        std::string_view name;
    };

    struct BufferCreateInfo
    {
        uint32_t size = 0;
        const void* data = nullptr;
        BufferType type = BufferType::eDefault;
        std::shared_ptr<IResource> resource = nullptr;
        std::string_view name;
    };

    struct QueueCreateInfo
    {
        QueueType type;
        QueuePriority priority;
        std::string_view name;
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
