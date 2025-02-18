#pragma once

namespace Swift
{
    enum class ImageFormat : uint8_t
    {
        eR16G16B16A16_SRGB,
        eR32G32B32A32_SRGB,
        eD32
    };

    enum class ImageTransition : uint8_t
    {
        eUndefined,
        eGeneral,
        eColorAttachmentOptimal,
        eDepthStencilAttachmentOptimal,
        eShaderReadOnlyOptimal,
        eTransferSrcOptimal,
        eTransferDstOptimal,
        eDepthAttachmentOptimal,
        eStencilAttachmentOptimal,
        eReadOnlyOptimal,
        ePresentSrcKHR
    };

    enum class ImageUsage : uint8_t
    {
        eSampledReadWrite,
        eReadWrite,
        eSampled,
        eTemporary,
    };

    enum class ImageType : uint8_t
    {
        e1D,
        e2D,
        e3D,
        eCube
    };

    enum class BufferType : uint8_t
    {
        eUniform,
        eStorage,
        eIndex,
        eIndirect,
        eReadback
    };

    enum class CullMode
    {
        eNone,
        eFront,
        eBack,
        eFrontAndBack
    };

    enum class DepthCompareOp
    {
        eNever,
        eLess,
        eEqual,
        eLessOrEqual,
        eGreater,
        eNotEqual,
        eGreaterOrEqual,
        eAlways
    };

    enum class PolygonMode
    {
        eFill,
        eLine,
        ePoint,
    };

    enum class Topology
    {
        ePointList,
        eLineList,
        eLineStrip,
        eTriangleList,
        eTriangleStrip,
        eTriangleFan
    };

} // namespace Swift