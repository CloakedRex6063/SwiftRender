#pragma once
#include "swift_helpers.hpp"
#include "swift_structs.hpp"
#include "swift_buffer.hpp"
#include "directx/d3d12.h"

namespace Swift::D3D12
{
    constexpr D3D12_COMMAND_LIST_TYPE ToCommandType(const QueueType type) noexcept
    {
        switch (type)
        {
            case QueueType::eGraphics:
                return D3D12_COMMAND_LIST_TYPE_DIRECT;
            case QueueType::eCompute:
                return D3D12_COMMAND_LIST_TYPE_COMPUTE;
            case QueueType::eTransfer:
                return D3D12_COMMAND_LIST_TYPE_COPY;
        }
        return D3D12_COMMAND_LIST_TYPE_DIRECT;
    }

    constexpr D3D12_HEAP_TYPE ToHeapType(const HeapType type) noexcept
    {
        switch (type)
        {
            case HeapType::eUpload:
                return D3D12_HEAP_TYPE_UPLOAD;
            case HeapType::eGPU:
                return D3D12_HEAP_TYPE_DEFAULT;
            case HeapType::eGPU_Upload:
                return D3D12_HEAP_TYPE_GPU_UPLOAD;
            case HeapType::eReadback:
                return D3D12_HEAP_TYPE_READBACK;
        }
        return D3D12_HEAP_TYPE_GPU_UPLOAD;
    }

    constexpr D3D12_ROOT_PARAMETER_TYPE ToDescriptorType(const DescriptorType type) noexcept
    {
        switch (type)
        {
            case DescriptorType::eConstant:
                return D3D12_ROOT_PARAMETER_TYPE_CBV;
            case DescriptorType::eShaderResource:
                return D3D12_ROOT_PARAMETER_TYPE_SRV;
            case DescriptorType::eUnorderedAccess:
                return D3D12_ROOT_PARAMETER_TYPE_UAV;
        }
        return D3D12_ROOT_PARAMETER_TYPE_CBV;
    }

    constexpr D3D12_SHADER_VISIBILITY ToShaderVisibility(const ShaderVisibility shader_visibility) noexcept
    {
        switch (shader_visibility)
        {
            case ShaderVisibility::eAll:
                return D3D12_SHADER_VISIBILITY_ALL;
            case ShaderVisibility::eVertex:
                return D3D12_SHADER_VISIBILITY_VERTEX;
            case ShaderVisibility::eHull:
                return D3D12_SHADER_VISIBILITY_HULL;
            case ShaderVisibility::eDomain:
                return D3D12_SHADER_VISIBILITY_DOMAIN;
            case ShaderVisibility::eGeometry:
                return D3D12_SHADER_VISIBILITY_GEOMETRY;
            case ShaderVisibility::ePixel:
                return D3D12_SHADER_VISIBILITY_PIXEL;
            case ShaderVisibility::eAmplification:
                return D3D12_SHADER_VISIBILITY_AMPLIFICATION;
            case ShaderVisibility::eMesh:
                return D3D12_SHADER_VISIBILITY_MESH;
        }
        return D3D12_SHADER_VISIBILITY_ALL;
    }

    constexpr D3D12_COMMAND_QUEUE_PRIORITY ToCommandPriority(const QueuePriority priority) noexcept
    {
        switch (priority)
        {
            case QueuePriority::eHigh:
                return D3D12_COMMAND_QUEUE_PRIORITY_HIGH;
            case QueuePriority::eNormal:
                return D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
        }
        return D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
    }

    constexpr DXGI_FORMAT ToViewDXGIFormat(const Format format) noexcept
    {
        switch (format)
        {
            case Format::eRGBA8_UNORM:
                return DXGI_FORMAT_R8G8B8A8_UNORM;
            case Format::eRGBA16F:
                return DXGI_FORMAT_R16G16B16A16_FLOAT;
            case Format::eRGBA32F:
                return DXGI_FORMAT_R32G32B32A32_FLOAT;
            case Format::eD32F:
                return DXGI_FORMAT_R32_FLOAT;
            case Format::eBC1_UNORM:
                return DXGI_FORMAT_BC1_UNORM;
            case Format::eBC1_UNORM_SRGB:
                return DXGI_FORMAT_BC1_UNORM_SRGB;
            case Format::eBC2_UNORM:
                return DXGI_FORMAT_BC2_UNORM;
            case Format::eBC2_UNORM_SRGB:
                return DXGI_FORMAT_BC2_UNORM_SRGB;
            case Format::eBC3_UNORM:
                return DXGI_FORMAT_BC3_UNORM;
            case Format::eBC3_UNORM_SRGB:
                return DXGI_FORMAT_BC3_UNORM_SRGB;
            case Format::eBC4_UNORM:
                return DXGI_FORMAT_BC4_UNORM;
            case Format::eBC4_SNORM:
                return DXGI_FORMAT_BC4_SNORM;
            case Format::eBC5_UNORM:
                return DXGI_FORMAT_BC5_UNORM;
            case Format::eBC5_SNORM:
                return DXGI_FORMAT_BC5_SNORM;
            case Format::eBC6H_UF16:
                return DXGI_FORMAT_BC6H_UF16;
            case Format::eBC6H_SF16:
                return DXGI_FORMAT_BC6H_SF16;
            case Format::eBC7_UNORM:
                return DXGI_FORMAT_BC7_UNORM;
            case Format::eBC7_UNORM_SRGB:
                return DXGI_FORMAT_BC7_UNORM_SRGB;
            case Format::eR8_UNORM:
                return DXGI_FORMAT_R8_UNORM;
            case Format::eR8G8_UNORM:
                return DXGI_FORMAT_R8G8_UNORM;
        }

        return DXGI_FORMAT_UNKNOWN;
    }

    constexpr DXGI_FORMAT ToResourceDXGIFormat(const Format format) noexcept
    {
        switch (format)
        {
            case Format::eRGBA8_UNORM:
                return DXGI_FORMAT_R8G8B8A8_UNORM;
            case Format::eRGBA16F:
                return DXGI_FORMAT_R16G16B16A16_FLOAT;
            case Format::eRGBA32F:
                return DXGI_FORMAT_R32G32B32A32_FLOAT;
            case Format::eD32F:
                return DXGI_FORMAT_R32_TYPELESS;
            case Format::eBC1_UNORM:
                return DXGI_FORMAT_BC1_UNORM;
            case Format::eBC1_UNORM_SRGB:
                return DXGI_FORMAT_BC1_UNORM_SRGB;
            case Format::eBC2_UNORM:
                return DXGI_FORMAT_BC2_UNORM;
            case Format::eBC2_UNORM_SRGB:
                return DXGI_FORMAT_BC2_UNORM_SRGB;
            case Format::eBC3_UNORM:
                return DXGI_FORMAT_BC3_UNORM;
            case Format::eBC3_UNORM_SRGB:
                return DXGI_FORMAT_BC3_UNORM_SRGB;
            case Format::eBC4_UNORM:
                return DXGI_FORMAT_BC4_UNORM;
            case Format::eBC4_SNORM:
                return DXGI_FORMAT_BC4_SNORM;
            case Format::eBC5_UNORM:
                return DXGI_FORMAT_BC5_UNORM;
            case Format::eBC5_SNORM:
                return DXGI_FORMAT_BC5_SNORM;
            case Format::eBC6H_UF16:
                return DXGI_FORMAT_BC6H_UF16;
            case Format::eBC6H_SF16:
                return DXGI_FORMAT_BC6H_SF16;
            case Format::eBC7_UNORM:
                return DXGI_FORMAT_BC7_UNORM;
            case Format::eBC7_UNORM_SRGB:
                return DXGI_FORMAT_BC7_UNORM_SRGB;
            case Format::eR8_UNORM:
                return DXGI_FORMAT_R8_UNORM;
            case Format::eR8G8_UNORM:
                return DXGI_FORMAT_R8G8_UNORM;
        }

        return DXGI_FORMAT_UNKNOWN;
    }

    constexpr DXGI_FORMAT ToDXGIFormat(const Format format) noexcept
    {
        switch (format)
        {
            case Format::eRGBA8_UNORM:
                return DXGI_FORMAT_R8G8B8A8_UNORM;
            case Format::eRGBA16F:
                return DXGI_FORMAT_R16G16B16A16_FLOAT;
            case Format::eRGBA32F:
                return DXGI_FORMAT_R32G32B32A32_FLOAT;
            case Format::eD32F:
                return DXGI_FORMAT_D32_FLOAT;
            case Format::eBC1_UNORM:
                return DXGI_FORMAT_BC1_UNORM;
            case Format::eBC1_UNORM_SRGB:
                return DXGI_FORMAT_BC1_UNORM_SRGB;
            case Format::eBC2_UNORM:
                return DXGI_FORMAT_BC2_UNORM;
            case Format::eBC2_UNORM_SRGB:
                return DXGI_FORMAT_BC2_UNORM_SRGB;
            case Format::eBC3_UNORM:
                return DXGI_FORMAT_BC3_UNORM;
            case Format::eBC3_UNORM_SRGB:
                return DXGI_FORMAT_BC3_UNORM_SRGB;
            case Format::eBC4_UNORM:
                return DXGI_FORMAT_BC4_UNORM;
            case Format::eBC4_SNORM:
                return DXGI_FORMAT_BC4_SNORM;
            case Format::eBC5_UNORM:
                return DXGI_FORMAT_BC5_UNORM;
            case Format::eBC5_SNORM:
                return DXGI_FORMAT_BC5_SNORM;
            case Format::eBC6H_UF16:
                return DXGI_FORMAT_BC6H_UF16;
            case Format::eBC6H_SF16:
                return DXGI_FORMAT_BC6H_SF16;
            case Format::eBC7_UNORM:
                return DXGI_FORMAT_BC7_UNORM;
            case Format::eBC7_UNORM_SRGB:
                return DXGI_FORMAT_BC7_UNORM_SRGB;
            case Format::eR8_UNORM:
                return DXGI_FORMAT_R8_UNORM;
            case Format::eR8G8_UNORM:
                return DXGI_FORMAT_R8G8_UNORM;
        }

        return DXGI_FORMAT_UNKNOWN;
    }

    constexpr D3D12_CULL_MODE ToCullMode(const CullMode mode) noexcept
    {
        switch (mode)
        {
            case CullMode::eBack:
                return D3D12_CULL_MODE_BACK;
            case CullMode::eFront:
                return D3D12_CULL_MODE_FRONT;
            case CullMode::eNone:
                return D3D12_CULL_MODE_NONE;
        }
        return D3D12_CULL_MODE_NONE;
    }

    constexpr bool ToFrontFace(const FrontFace front_face) noexcept
    {
        switch (front_face)
        {
            case FrontFace::eClockwise:
                return false;
            case FrontFace::eCounterClockwise:
                return true;
        }
        return true;
    }

    constexpr D3D12_FILL_MODE ToFillMode(const FillMode mode) noexcept
    {
        switch (mode)
        {
            case FillMode::eWireframe:
                return D3D12_FILL_MODE_WIREFRAME;
            case FillMode::eSolid:
                return D3D12_FILL_MODE_SOLID;
        }
        return D3D12_FILL_MODE_WIREFRAME;
    }

    constexpr D3D12_PRIMITIVE_TOPOLOGY_TYPE ToPolygonMode(const PolygonMode mode) noexcept
    {
        switch (mode)
        {
            case PolygonMode::ePoint:
                return D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT;
            case PolygonMode::eLine:
                return D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE;
            case PolygonMode::eTriangle:
                return D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
        }
        return D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    }

    constexpr D3D12_COMPARISON_FUNC ToDepthTest(const DepthTest test)
    {
        switch (test)
        {
            case DepthTest::eAlways:
                return D3D12_COMPARISON_FUNC_ALWAYS;
            case DepthTest::eNever:
                return D3D12_COMPARISON_FUNC_NEVER;
            case DepthTest::eLess:
                return D3D12_COMPARISON_FUNC_LESS;
            case DepthTest::eLessEqual:
                return D3D12_COMPARISON_FUNC_LESS_EQUAL;
            case DepthTest::eGreater:
                return D3D12_COMPARISON_FUNC_GREATER;
            case DepthTest::eGreaterEqual:
                return D3D12_COMPARISON_FUNC_GREATER_EQUAL;
            case DepthTest::eEqual:
                return D3D12_COMPARISON_FUNC_EQUAL;
            case DepthTest::eNotEqual:
                return D3D12_COMPARISON_FUNC_NOT_EQUAL;
        }
        return D3D12_COMPARISON_FUNC_ALWAYS;
    }

    constexpr D3D12_COMPARISON_FUNC ToComparisonFunc(const ComparisonFunc test)
    {
        switch (test)
        {
            case ComparisonFunc::eAlways:
                return D3D12_COMPARISON_FUNC_ALWAYS;
            case ComparisonFunc::eNever:
                return D3D12_COMPARISON_FUNC_NEVER;
            case ComparisonFunc::eLess:
                return D3D12_COMPARISON_FUNC_LESS;
            case ComparisonFunc::eLessEqual:
                return D3D12_COMPARISON_FUNC_LESS_EQUAL;
            case ComparisonFunc::eGreater:
                return D3D12_COMPARISON_FUNC_GREATER;
            case ComparisonFunc::eGreaterEqual:
                return D3D12_COMPARISON_FUNC_GREATER_EQUAL;
            case ComparisonFunc::eEqual:
                return D3D12_COMPARISON_FUNC_EQUAL;
            case ComparisonFunc::eNotEqual:
                return D3D12_COMPARISON_FUNC_NOT_EQUAL;
        }
        return D3D12_COMPARISON_FUNC_ALWAYS;
    }

    constexpr D3D12_RESOURCE_STATES ToResourceState(const ResourceState state) noexcept
    {
        switch (state)
        {
            case ResourceState::eCommon:
                return D3D12_RESOURCE_STATE_COMMON;
            case ResourceState::eCopyDest:
                return D3D12_RESOURCE_STATE_COPY_DEST;
            case ResourceState::eCopySource:
                return D3D12_RESOURCE_STATE_COPY_SOURCE;
            case ResourceState::eIndexBuffer:
                return D3D12_RESOURCE_STATE_INDEX_BUFFER;
            case ResourceState::eUnorderedAccess:
                return D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
            case ResourceState::eRenderTarget:
                return D3D12_RESOURCE_STATE_RENDER_TARGET;
            case ResourceState::eDepthWrite:
                return D3D12_RESOURCE_STATE_DEPTH_WRITE;
            case ResourceState::eDepthRead:
                return D3D12_RESOURCE_STATE_DEPTH_READ;
            case ResourceState::eShaderResource:
                return D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE;
            case ResourceState::ePresent:
                return D3D12_RESOURCE_STATE_COMMON;
            case ResourceState::eConstant:
                return D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;
        }
        return D3D12_RESOURCE_STATE_COMMON;
    }

    constexpr D3D12_FILTER ToFilter(const Filter min_filter,
                                    const Filter mag_filter,
                                    const FilterType type = FilterType::eStandard) noexcept
    {
        constexpr auto get_base = [](const Filter f) -> int
        {
            switch (f)
            {
                case Filter::eNearest:
                case Filter::eNearestMipNearest:
                case Filter::eNearestMipLinear:
                    return 0;
                case Filter::eLinear:
                case Filter::eLinearMipNearest:
                case Filter::eLinearMipLinear:
                    return 1;
            }
            return 0;
        };

        constexpr auto get_mip = [](const Filter f) -> int
        {
            switch (f)
            {
                case Filter::eNearest:
                case Filter::eLinear:
                case Filter::eNearestMipNearest:
                case Filter::eLinearMipNearest:
                    return 0;
                case Filter::eNearestMipLinear:
                case Filter::eLinearMipLinear:
                    return 1;
            }
            return 0;
        };

        const int min_type = get_base(min_filter);
        const int mag_type = get_base(mag_filter);
        const int mip_type = get_mip(min_filter);

        const D3D12_FILTER_REDUCTION_TYPE reduction = [&]() -> D3D12_FILTER_REDUCTION_TYPE
        {
            switch (type)
            {
                case FilterType::eStandard:
                    return D3D12_FILTER_REDUCTION_TYPE_STANDARD;
                case FilterType::eComparison:
                    return D3D12_FILTER_REDUCTION_TYPE_COMPARISON;
                case FilterType::eMinimum:
                    return D3D12_FILTER_REDUCTION_TYPE_MINIMUM;
                case FilterType::eMaximum:
                    return D3D12_FILTER_REDUCTION_TYPE_MAXIMUM;
            }
            return D3D12_FILTER_REDUCTION_TYPE_STANDARD;
        }();

        return D3D12_ENCODE_BASIC_FILTER(static_cast<D3D12_FILTER_TYPE>(min_type),
                                         static_cast<D3D12_FILTER_TYPE>(mag_type),
                                         static_cast<D3D12_FILTER_TYPE>(mip_type),
                                         reduction);
    }

    // Standard
    static_assert(ToFilter(Filter::eNearestMipNearest, Filter::eNearest) == D3D12_FILTER_MIN_MAG_MIP_POINT);
    static_assert(ToFilter(Filter::eNearestMipLinear, Filter::eNearest) == D3D12_FILTER_MIN_MAG_POINT_MIP_LINEAR);
    static_assert(ToFilter(Filter::eNearestMipNearest, Filter::eLinear) == D3D12_FILTER_MIN_POINT_MAG_LINEAR_MIP_POINT);
    static_assert(ToFilter(Filter::eNearestMipLinear, Filter::eLinear) == D3D12_FILTER_MIN_POINT_MAG_MIP_LINEAR);
    static_assert(ToFilter(Filter::eLinearMipNearest, Filter::eNearest) == D3D12_FILTER_MIN_LINEAR_MAG_MIP_POINT);
    static_assert(ToFilter(Filter::eLinearMipLinear, Filter::eNearest) == D3D12_FILTER_MIN_LINEAR_MAG_POINT_MIP_LINEAR);
    static_assert(ToFilter(Filter::eLinearMipNearest, Filter::eLinear) == D3D12_FILTER_MIN_MAG_LINEAR_MIP_POINT);
    static_assert(ToFilter(Filter::eLinearMipLinear, Filter::eLinear) == D3D12_FILTER_MIN_MAG_MIP_LINEAR);

    // Comparison
    static_assert(ToFilter(Filter::eNearestMipNearest, Filter::eNearest, FilterType::eComparison) ==
                  D3D12_FILTER_COMPARISON_MIN_MAG_MIP_POINT);
    static_assert(ToFilter(Filter::eNearestMipLinear, Filter::eNearest, FilterType::eComparison) ==
                  D3D12_FILTER_COMPARISON_MIN_MAG_POINT_MIP_LINEAR);
    static_assert(ToFilter(Filter::eNearestMipNearest, Filter::eLinear, FilterType::eComparison) ==
                  D3D12_FILTER_COMPARISON_MIN_POINT_MAG_LINEAR_MIP_POINT);
    static_assert(ToFilter(Filter::eNearestMipLinear, Filter::eLinear, FilterType::eComparison) ==
                  D3D12_FILTER_COMPARISON_MIN_POINT_MAG_MIP_LINEAR);
    static_assert(ToFilter(Filter::eLinearMipNearest, Filter::eNearest, FilterType::eComparison) ==
                  D3D12_FILTER_COMPARISON_MIN_LINEAR_MAG_MIP_POINT);
    static_assert(ToFilter(Filter::eLinearMipLinear, Filter::eNearest, FilterType::eComparison) ==
                  D3D12_FILTER_COMPARISON_MIN_LINEAR_MAG_POINT_MIP_LINEAR);
    static_assert(ToFilter(Filter::eLinearMipNearest, Filter::eLinear, FilterType::eComparison) ==
                  D3D12_FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT);
    static_assert(ToFilter(Filter::eLinearMipLinear, Filter::eLinear, FilterType::eComparison) ==
                  D3D12_FILTER_COMPARISON_MIN_MAG_MIP_LINEAR);

    // Minimum
    static_assert(ToFilter(Filter::eNearestMipNearest, Filter::eNearest, FilterType::eMinimum) ==
                  D3D12_FILTER_MINIMUM_MIN_MAG_MIP_POINT);
    static_assert(ToFilter(Filter::eNearestMipLinear, Filter::eNearest, FilterType::eMinimum) ==
                  D3D12_FILTER_MINIMUM_MIN_MAG_POINT_MIP_LINEAR);
    static_assert(ToFilter(Filter::eNearestMipNearest, Filter::eLinear, FilterType::eMinimum) ==
                  D3D12_FILTER_MINIMUM_MIN_POINT_MAG_LINEAR_MIP_POINT);
    static_assert(ToFilter(Filter::eNearestMipLinear, Filter::eLinear, FilterType::eMinimum) ==
                  D3D12_FILTER_MINIMUM_MIN_POINT_MAG_MIP_LINEAR);
    static_assert(ToFilter(Filter::eLinearMipNearest, Filter::eNearest, FilterType::eMinimum) ==
                  D3D12_FILTER_MINIMUM_MIN_LINEAR_MAG_MIP_POINT);
    static_assert(ToFilter(Filter::eLinearMipLinear, Filter::eNearest, FilterType::eMinimum) ==
                  D3D12_FILTER_MINIMUM_MIN_LINEAR_MAG_POINT_MIP_LINEAR);
    static_assert(ToFilter(Filter::eLinearMipNearest, Filter::eLinear, FilterType::eMinimum) ==
                  D3D12_FILTER_MINIMUM_MIN_MAG_LINEAR_MIP_POINT);
    static_assert(ToFilter(Filter::eLinearMipLinear, Filter::eLinear, FilterType::eMinimum) ==
                  D3D12_FILTER_MINIMUM_MIN_MAG_MIP_LINEAR);

    // Maximum
    static_assert(ToFilter(Filter::eNearestMipNearest, Filter::eNearest, FilterType::eMaximum) ==
                  D3D12_FILTER_MAXIMUM_MIN_MAG_MIP_POINT);
    static_assert(ToFilter(Filter::eNearestMipLinear, Filter::eNearest, FilterType::eMaximum) ==
                  D3D12_FILTER_MAXIMUM_MIN_MAG_POINT_MIP_LINEAR);
    static_assert(ToFilter(Filter::eNearestMipNearest, Filter::eLinear, FilterType::eMaximum) ==
                  D3D12_FILTER_MAXIMUM_MIN_POINT_MAG_LINEAR_MIP_POINT);
    static_assert(ToFilter(Filter::eNearestMipLinear, Filter::eLinear, FilterType::eMaximum) ==
                  D3D12_FILTER_MAXIMUM_MIN_POINT_MAG_MIP_LINEAR);
    static_assert(ToFilter(Filter::eLinearMipNearest, Filter::eNearest, FilterType::eMaximum) ==
                  D3D12_FILTER_MAXIMUM_MIN_LINEAR_MAG_MIP_POINT);
    static_assert(ToFilter(Filter::eLinearMipLinear, Filter::eNearest, FilterType::eMaximum) ==
                  D3D12_FILTER_MAXIMUM_MIN_LINEAR_MAG_POINT_MIP_LINEAR);
    static_assert(ToFilter(Filter::eLinearMipNearest, Filter::eLinear, FilterType::eMaximum) ==
                  D3D12_FILTER_MAXIMUM_MIN_MAG_LINEAR_MIP_POINT);
    static_assert(ToFilter(Filter::eLinearMipLinear, Filter::eLinear, FilterType::eMaximum) ==
                  D3D12_FILTER_MAXIMUM_MIN_MAG_MIP_LINEAR);

    constexpr D3D12_TEXTURE_ADDRESS_MODE ToWrap(const Wrap wrap) noexcept
    {
        switch (wrap)
        {
            case Wrap::eRepeat:
                return D3D12_TEXTURE_ADDRESS_MODE_WRAP;
            case Wrap::eMirroredRepeat:
                return D3D12_TEXTURE_ADDRESS_MODE_MIRROR;
            case Wrap::eClampToEdge:
                return D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
            case Wrap::eBorder:
                return D3D12_TEXTURE_ADDRESS_MODE_BORDER;
                break;
        }
        return D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    }

    inline const char* GetResultMessage(const HRESULT hr)
    {
        LPSTR errorText = nullptr;
        FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_IGNORE_INSERTS,
                      nullptr,
                      hr,
                      MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                      reinterpret_cast<LPSTR>(&errorText),
                      0,
                      nullptr);
        return errorText;
    }

    inline uint32_t GetBufferSize(ID3D12Device* device, const BufferCreateInfo& info)
    {
        uint64_t total_size = 0;

        const D3D12_RESOURCE_DESC resource_info = {
            .Dimension = D3D12_RESOURCE_DIMENSION_BUFFER,
            .Alignment = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT,
            .Width = info.size,
            .Height = 1,
            .DepthOrArraySize = 1,
            .MipLevels = 1,
            .Format = DXGI_FORMAT_UNKNOWN,
            .SampleDesc = {1, 0},
            .Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR,
            .Flags = D3D12_RESOURCE_FLAG_NONE,
        };

        device->GetCopyableFootprints(&resource_info, 0, 1, 0, nullptr, nullptr, nullptr, &total_size);

        return total_size;
    }

    inline uint32_t GetTextureSize(ID3D12Device14* device, const TextureCreateInfo& create_info)
    {
        uint64_t total_size = 0;

        auto info = create_info;

        auto flags = D3D12_RESOURCE_FLAG_NONE;
        if (info.flags & TextureFlags::eRenderTarget)
        {
            flags |= D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
        }
        if (info.flags & TextureFlags::eDepthStencil)
        {
            flags |= D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
        }
        const auto sample_desc = info.msaa ? DXGI_SAMPLE_DESC{info.msaa->samples, info.msaa->quality} : DXGI_SAMPLE_DESC{1, 0};

        if (info.mip_levels == 0)
        {
            info.mip_levels = CalculateMaxMips(create_info.width, create_info.height);
        }

        const D3D12_RESOURCE_DESC resource_info = {
            .Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D,
            .Alignment = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT,
            .Width = info.width,
            .Height = info.height,
            .DepthOrArraySize = info.array_size,
            .MipLevels = info.mip_levels,
            .Format = ToDXGIFormat(info.format),
            .SampleDesc = sample_desc,
            .Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN,
            .Flags = flags,
        };

        device->GetCopyableFootprints(&resource_info,
                                      0,
                                      info.mip_levels * info.array_size,
                                      0,
                                      nullptr,
                                      nullptr,
                                      nullptr,
                                      &total_size);

        return total_size;
    }

    inline void CopyTextureData(ID3D12Device14* device, Swift::IBuffer* buffer, const Swift::TextureCreateInfo& create_info)
    {
        uint32_t num_subresources = create_info.array_size * create_info.mip_levels;

        std::vector<D3D12_PLACED_SUBRESOURCE_FOOTPRINT> footprints(num_subresources);
        std::vector<UINT> num_rows(num_subresources);
        std::vector<UINT64> row_sizes(num_subresources);
        UINT64 total_size = 0;

        auto* src = static_cast<const char*>(create_info.data);
        constexpr auto sample_desc = DXGI_SAMPLE_DESC{1, 0};

        const D3D12_RESOURCE_DESC resource_desc = {
            .Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D,
            .Alignment = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT,
            .Width = create_info.width,
            .Height = create_info.height,
            .DepthOrArraySize = static_cast<uint16_t>(create_info.array_size),
            .MipLevels = create_info.mip_levels,
            .Format = ToDXGIFormat(create_info.format),
            .SampleDesc = sample_desc,
            .Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN,
        };
        device->GetCopyableFootprints(&resource_desc,
                                      0,
                                      num_subresources,
                                      0,
                                      footprints.data(),
                                      num_rows.data(),
                                      row_sizes.data(),
                                      &total_size);

        for (uint32_t i = 0; i < num_subresources; ++i)
        {
            for (uint32_t row = 0; row < num_rows[i]; ++row)
            {
                buffer->Write(src + row * row_sizes[i],
                              footprints[i].Offset + row * footprints[i].Footprint.RowPitch,
                              row_sizes[i]);
            }

            src += num_rows[i] * row_sizes[i];
        }
    }
}  // namespace Swift::D3D12
