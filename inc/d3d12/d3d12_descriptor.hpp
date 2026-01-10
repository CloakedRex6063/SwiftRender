#pragma once

#include "directx/d3d12.h"
#include "vector"

namespace Swift::D3D12
{
    struct Descriptor
    {
        D3D12_CPU_DESCRIPTOR_HANDLE cpu_handle;
        D3D12_GPU_DESCRIPTOR_HANDLE gpu_handle;
        uint32_t index;
    };

    class DescriptorHeap
    {
    public:
        DescriptorHeap(ID3D12Device14* device, D3D12_DESCRIPTOR_HEAP_TYPE heap_type, uint32_t count);

        Descriptor Allocate();
        void Free(const Descriptor& descriptor);
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
        std::vector<Descriptor> m_descriptors;
        std::vector<uint32_t> m_free_descriptors;
    };
}