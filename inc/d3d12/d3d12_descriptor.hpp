#pragma once

#include "swift_descriptor.hpp"
#include "swift_structs.hpp"
#include "directx/d3d12.h"
#include "vector"

namespace Swift::D3D12
{
    struct DescriptorData
    {
        D3D12_CPU_DESCRIPTOR_HANDLE cpu_handle;
        D3D12_GPU_DESCRIPTOR_HANDLE gpu_handle;
        uint32_t index;
    };

    class Context;

    class D3D12Descriptor
    {
    public:
        D3D12Descriptor(Context* context) : m_context(context) {};
        DescriptorData& GetDescriptorData() { return m_data; }

    protected:
        DescriptorData m_data{};
        Context* m_context = nullptr;
    };

    class RenderTarget : public IRenderTarget, public D3D12Descriptor
    {
    public:
        RenderTarget(Context* context, ITexture* texture, uint32_t mip = 0);
        ~RenderTarget() override;
        [[nodiscard]] uint32_t GetDescriptorIndex() const override { return m_data.index; }
    };

    class DepthStencil : public IDepthStencil, public D3D12Descriptor
    {
    public:
        DepthStencil(Context* context, ITexture* texture, uint32_t mip = 0);
        ~DepthStencil() override;
        [[nodiscard]] uint32_t GetDescriptorIndex() const override { return m_data.index; }
    };

    class TextureSRV : public ITextureSRV, public D3D12Descriptor
    {
    public:
        TextureSRV(Context* context,
                   ITexture* texture,
                   uint32_t most_detailed_mip = 0,
                   uint32_t mip_levels = 0);
        ~TextureSRV() override;
        [[nodiscard]] uint32_t GetDescriptorIndex() const override { return m_data.index; }
    };

    class BufferSRV : public IBufferSRV, public D3D12Descriptor
    {
    public:
        BufferSRV(Context* context, IBuffer* buffer, const BufferSRVCreateInfo& info);
        ~BufferSRV() override;
        [[nodiscard]] uint32_t GetDescriptorIndex() const override { return m_data.index; }
    };

    class TextureUAV : public ITextureUAV, public D3D12Descriptor
    {
    public:
        TextureUAV(Context* context, ITexture* texture, uint32_t mip = 0);
        ~TextureUAV() override;
        [[nodiscard]] uint32_t GetDescriptorIndex() const override { return m_data.index; }
    };

    class BufferUAV : public IBufferUAV, public D3D12Descriptor
    {
    public:
        BufferUAV(Context* context, IBuffer* buffer, const BufferUAVCreateInfo& info);
        ~BufferUAV() override;
        [[nodiscard]] uint32_t GetDescriptorIndex() const override { return m_data.index; }
    };

    class BufferCBV : public IBufferCBV, public D3D12Descriptor
    {
    public:
        BufferCBV(Context* context, IBuffer* buffer, uint32_t size, uint32_t offset = 0);
        ~BufferCBV() override;
        [[nodiscard]] uint32_t GetDescriptorIndex() const override { return m_data.index; }
    };

    class DescriptorHeap
    {
    public:
        DescriptorHeap(ID3D12Device14* device, D3D12_DESCRIPTOR_HEAP_TYPE heap_type, uint32_t count);
        ~DescriptorHeap();

        DescriptorData Allocate();
        void Free(const DescriptorData& descriptor);
        [[nodiscard]] ID3D12DescriptorHeap* GetHeap() const { return m_heap; }
        [[nodiscard]] uint32_t GetStride() const { return m_stride; }
        [[nodiscard]] D3D12_CPU_DESCRIPTOR_HANDLE GetCpuBaseHandle() const { return m_cpu_base; }
        [[nodiscard]] D3D12_GPU_DESCRIPTOR_HANDLE GetGpuBaseHandle() const { return m_gpu_base; }

    private:
        ID3D12DescriptorHeap* m_heap = nullptr;
        D3D12_CPU_DESCRIPTOR_HANDLE m_cpu_base{};
        D3D12_GPU_DESCRIPTOR_HANDLE m_gpu_base{};
        D3D12_DESCRIPTOR_HEAP_TYPE m_heap_type;
        uint32_t m_stride = 0;
        uint32_t m_index = 0;
        std::vector<DescriptorData> m_descriptors;
        std::vector<uint32_t> m_free_descriptors;
    };
}  // namespace Swift::D3D12