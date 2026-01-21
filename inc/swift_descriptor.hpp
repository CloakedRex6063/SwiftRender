#pragma once
#include "cstdint"
#include "swift_buffer.hpp"
#include "swift_macros.hpp"
#include "swift_texture.hpp"

namespace Swift
{
    class IDescriptor
    {
    public:
        SWIFT_DESTRUCT(IDescriptor);
        SWIFT_NO_MOVE(IDescriptor);
        SWIFT_NO_COPY(IDescriptor);
        [[nodiscard]] virtual uint32_t GetDescriptorIndex() const = 0;

    protected:
        SWIFT_CONSTRUCT(IDescriptor);
    };

    class IRenderTarget : public IDescriptor
    {
    public:
        SWIFT_DESTRUCT(IRenderTarget);
        SWIFT_NO_MOVE(IRenderTarget);
        SWIFT_NO_COPY(IRenderTarget);
        ITexture* GetTexture() const { return m_texture; }

    protected:
        explicit IRenderTarget(ITexture* texture) : m_texture(texture) {}

    private:
        ITexture* m_texture;
    };

    class IDepthStencil : public IDescriptor
    {
    public:
        SWIFT_DESTRUCT(IDepthStencil);
        SWIFT_NO_MOVE(IDepthStencil);
        SWIFT_NO_COPY(IDepthStencil);
        ITexture* GetTexture() const { return m_texture; }

    protected:
        explicit IDepthStencil(ITexture* texture) : m_texture(texture) {}

    private:
        ITexture* m_texture;
    };

    class ITextureSRV : public IDescriptor
    {
    public:
        SWIFT_DESTRUCT(ITextureSRV);
        SWIFT_NO_MOVE(ITextureSRV);
        SWIFT_NO_COPY(ITextureSRV);
        ITexture* GetTexture() const { return m_texture; }

    protected:
        explicit ITextureSRV(ITexture* texture) : m_texture(texture) {}

    private:
        ITexture* m_texture;
    };

    class IBufferSRV : public IDescriptor
    {
    public:
        SWIFT_DESTRUCT(IBufferSRV);
        SWIFT_NO_MOVE(IBufferSRV);
        SWIFT_NO_COPY(IBufferSRV);
        IBuffer* GetBuffer() const { return m_buffer; }

    protected:
        explicit IBufferSRV(IBuffer* buffer) : m_buffer(buffer) {}

    private:
        IBuffer* m_buffer;
    };

    class IBufferCBV : public IDescriptor
    {
    public:
        SWIFT_DESTRUCT(IBufferCBV);
        SWIFT_NO_MOVE(IBufferCBV);
        SWIFT_NO_COPY(IBufferCBV);
        IBuffer* GetBuffer() const { return m_buffer; }

    protected:
        explicit IBufferCBV(IBuffer* buffer) : m_buffer(buffer) {}

    private:
        IBuffer* m_buffer;
    };

    class IBufferUAV : public IDescriptor
    {
    public:
        SWIFT_DESTRUCT(IBufferUAV);
        SWIFT_NO_MOVE(IBufferUAV);
        SWIFT_NO_COPY(IBufferUAV);
        IBuffer* GetBuffer() const { return m_buffer; }

    protected:
        explicit IBufferUAV(IBuffer* buffer) : m_buffer(buffer) {}

    private:
        IBuffer* m_buffer;
    };

    class ITextureUAV : public IDescriptor
    {
    public:
        SWIFT_DESTRUCT(ITextureUAV);
        SWIFT_NO_MOVE(ITextureUAV);
        SWIFT_NO_COPY(ITextureUAV);
        ITexture* GetTexture() const { return m_texture; }

    protected:
        explicit ITextureUAV(ITexture* texture) : m_texture(texture) {}

    private:
        ITexture* m_texture;
    };
}  // namespace Swift