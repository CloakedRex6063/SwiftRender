#pragma once
#include "X11/Xlib.h"
#include "cstdint"
#include "expected"
#include "limits"
#include "span"
#include "string"
#include "vector"

namespace Swift
{
    template<typename T>
    constexpr T operator|(T lhs, T rhs)
    {
        using U = std::underlying_type_t<T>;
        return static_cast<T>(static_cast<U>(lhs) | static_cast<U>(rhs));
    }

    template<typename T>
    constexpr T operator&(T lhs, T rhs)
    {
        using U = std::underlying_type_t<T>;
        return static_cast<T>(static_cast<U>(lhs) & static_cast<U>(rhs));
    }

    enum class Error
    {
        eInstanceCreateFailed,
        eGPUSelectionFailed,
        eQueueNotFound,
        eDeviceCreateFailed,
        eAllocatorCreateFailed,
        eSwapchainCreateFailed,
        eSurfaceCreateFailed,
        eCommandPoolCreateFailed,
        eCommandBufferCreateFailed,
        eImageViewCreateFailed,
        eImageCreateFailed,
        eBufferCreateFailed,
        eBufferMapFailed,
        eShaderCreateFailed,
        eDescriptorLayoutCreateFailed,
        ePipelineLayoutCreateFailed,
        eGraphicsPipelineCreateFailed,
        eSemaphoreCreateFailed,
        eFenceCreateFailed,
        eAcquireNextImageFailed,
        eSubmitFailed,
        ePresentFailed,
        eFenceWaitFailed,
        eDeviceWaitFailed,
        eCommandBeginFailed,
        eCommandEndFailed,
    };
    enum class ContextHandle : uint32_t
    {
        eNull = std::numeric_limits<uint>::max()
    };

    enum class QueueHandle : uint32_t
    {
        eNull = std::numeric_limits<uint32_t>::max()
    };

    enum class CommandHandle : uint32_t
    {
        eNull = std::numeric_limits<uint32_t>::max()
    };

    enum class ShaderLayoutHandle : uint32_t
    {
        eNull = std::numeric_limits<uint32_t>::max()
    };

    enum class ShaderHandle : uint32_t
    {
        eNull = std::numeric_limits<uint32_t>::max()
    };

    enum class TextureHandle : uint32_t
    {
        eNull = std::numeric_limits<uint32_t>::max()
    };

    enum class BufferHandle : uint32_t
    {
        eNull = std::numeric_limits<uint32_t>::max()
    };

    enum class DevicePreference
    {
        eHighPerformance = 2,
        ePowerSaving = 1,
    };

    struct WindowData
    {
        Display* DisplayHandle;
        Window WindowHandle;
    };

    struct ContextCreateInfo
    {
        std::string EngineName;
        std::string AppName;
        DevicePreference Preference = DevicePreference::eHighPerformance;
        int Width;
        int Height;
        WindowData Window;
    };

    enum class QueueType
    {
        eGraphics,
        eCompute,
        eTransfer,
    };

    enum class Format
    {
        eUnknown = 0,
        eRGBA8Srgb = 43,
        eBGRA8Unorm = 44,
        eRGBA16Float = 97,
        eR32Float = 98,
        eRG32Float = 103,
        eRGB32Float = 106,
        eRGBA32Float = 109,
        eD32Float = 126,
    };

    enum class VertexInputRate
    {
        eVertex,
        eInstance,
    };

    struct VertexBinding
    {
        uint32_t Binding;
        uint32_t Stride;
        VertexInputRate InputRate;
    };

    struct VertexAttribute
    {
        uint32_t Location;
        uint32_t Binding;
        Format AttributeFormat;
        uint32_t Offset;
    };

    enum class PrimitiveTopology
    {
        ePointList = 0,
        eLineList = 1,
        eLineStrip = 2,
        eTriangleList = 3,
        eTriangleStrip = 4,
        eTriangleFan = 5,
    };

    enum class PolygonMode
    {
        eFill = 0,
        eLine = 1,
        ePoint = 2
    };

    enum class CullMode
    {
        eNone = 0,
        eFront = 0x00000001,
        eBack = 0x00000002,
        eFrontAndBack = 0x00000003,
    };

    enum class FrontFace
    {
        eCounterClockwise = 0,
        eClockwise = 1,
    };

    enum class LogicOp
    {
        eClear = 0,
        eAnd = 1,
        eAndReverse = 2,
        eCopy = 3,
        eAndInverted = 4,
        eNoOp = 5,
        eXOr = 6,
        eOr = 7,
        eNor = 8,
        eEquivalent = 9,
        eInvert = 10,
        eOrReverse = 11,
        eCopyInverted = 12,
        eOrInverted = 13,
        eNand = 14,
        eSet = 15,
    };

    enum class BlendFactor
    {
        eZero = 0,
        eOne = 1,
        eSrcColor = 2,
        eOneMinusSrcColor = 3,
        eDstColor = 4,
        eOneMinusDstColor = 5,
        eSrcAlpha = 6,
        eOneMinusSrcAlpha = 7,
        eDstAlpha = 8,
        eOneMinusDstAlpha = 9,
        eConstantColor = 10,
        eOneMinusConstantColor = 11,
        eConstantAlpha = 12,
        eOneMinusConstantAlpha = 13,
        eSrcAlpha_SATURATE = 14,
        eSrc1Color = 15,
        eOneMinusSrc1Color = 16,
        eSrc1Alpha = 17,
        eOneMinusSrc1Alpha = 18,
    };

    enum class ColorComponents
    {
        eR = 0x00000001,
        eG = 0x00000002,
        eB = 0x00000004,
        eA = 0x00000008,
    };
    template ColorComponents operator| <ColorComponents>(ColorComponents lhs, ColorComponents rhs);
    template ColorComponents operator& <ColorComponents>(ColorComponents lhs, ColorComponents rhs);

    enum class BlendOp
    {
        eAdd = 0,
        eSubtract = 1,
        eReverseSubtract = 2,
        eMin = 3,
        eMax = 4,
    };

    struct BlendAttachment
    {
        bool BlendEnable;
        BlendOp ColorBlendOp;
        BlendOp AlphaBlendOp;
        BlendFactor SrcColorBlendFactor;
        BlendFactor SrcAlphaBlendFactor;
        BlendFactor DstColorBlendFactor;
        BlendFactor DstAlphaBlendFactor;
        ColorComponents ColorBlendMask;
    };

    struct BlendState
    {
        std::vector<BlendAttachment> BlendAttachments;
        LogicOp Logic = LogicOp::eAnd;
        bool LogicOpEnable{};
        std::array<const float, 4> BlendConstants{};
    };

    enum class DescriptorType
    {
        eSampler = 0,
        eCombinedImageSampler = 1,
        eSampledImage = 2,
        eStorageImage = 3,
        eUniformTexelBuffer = 4,
        eStorageTexelBuffer = 5,
        eUniformBuffer = 6,
        eStorageBuffer = 7,
        eUniformBufferDynamic = 8,
        eStorageBufferDynamic = 9,
        eInputAttachment = 10,
    };

    struct LayoutBinding
    {
        uint32_t Binding;
        DescriptorType Type;
        uint32_t Count;
    };

    struct ShaderLayoutCreateInfo
    {
        std::vector<LayoutBinding> LayoutBindings;
    };

    struct GraphicsShaderCreateInfo
    {
        std::span<const char> VertexCode = {};
        std::string VertexEntryPoint{};
        std::span<const char> FragmentCode = {};
        std::string FragmentEntryPoint{};
        std::span<const Format> ColorFormats{};
        Format DepthFormat{};
        PrimitiveTopology Topology = PrimitiveTopology::eTriangleList;
        BlendState BlendStateDesc;

        std::vector<VertexBinding> VertexBindings;
        std::vector<VertexAttribute> VertexAttributes;
    };

    enum class BufferUsage
    {
        eTransferSrc = 0x00000001,
        eTransferDst = 0x00000002,
        eUniformTexelBuffer = 0x00000004,
        eStorageTexelBuffer = 0x00000008,
        eUniformBuffer = 0x00000010,
        eStorageBuffer = 0x00000020,
        eIndexBuffer = 0x00000040,
        eVertexBuffer = 0x00000080,
        eIndirectBuffer = 0x00000100,
        eShaderDeviceAddress = 0x00020000,
    };
    template BufferUsage operator| <BufferUsage>(BufferUsage lhs, BufferUsage rhs);
    template BufferUsage operator& <BufferUsage>(BufferUsage lhs, BufferUsage rhs);

    struct BufferCreateInfo
    {
        uint64_t Size;
        BufferUsage Usage;
        void* InitialData;
    };

    enum class IndexType
    {
        eUInt16 = 0,
        eUInt32 = 1,
    };

    enum class AttachmentLoadOp : uint8_t
    {
        eLoad = 0,
        eClear = 1,
        eDontCare = 2,
    };

    enum class AttachmentStoreOp : uint8_t
    {
        eStore = 0,
        eDontCare = 1,
    };

    struct Vec4
    {
        float X;
        float Y;
        float Z;
        float W;
    };

    struct Vec2
    {
        float X;
        float Y;
    };

    struct Viewport
    {
        Vec2 Offset;
        Vec2 Extent;
        Vec2 DepthRange;
    };

    struct Rect
    {
        Vec2 Offset;
        Vec2 Extent;
    };

    struct RenderInfo
    {
        std::span<const TextureHandle> RenderTargets;
        TextureHandle DepthStencil = TextureHandle::eNull;

        Rect RenderArea;

        Vec4 ClearColor;
        float ClearDepth;
        AttachmentLoadOp ColorLoadOp = AttachmentLoadOp::eLoad;
        AttachmentStoreOp ColorStoreOp = AttachmentStoreOp::eStore;
        AttachmentLoadOp DepthLoadOp = AttachmentLoadOp::eClear;
        AttachmentStoreOp DepthStoreOp = AttachmentStoreOp::eStore;
    };
}  // namespace Swift