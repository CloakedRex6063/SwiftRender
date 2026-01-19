#pragma once
#include "d3d12_context.hpp"
#include "swift_buffer.hpp"

namespace Swift::D3D12
{
    class Buffer final : public IBuffer
    {
    public:
        Buffer(const std::shared_ptr<Context>& context,
               const std::shared_ptr<DescriptorHeap>& cbv_srv_uav_heap,
               const BufferCreateInfo& info);
        ~Buffer() override;
        void Write(const void* data, uint64_t offset, uint64_t size, bool one_time = false) override;
        void Read(uint32_t offset, uint32_t size, void* data) override;
        [[nodiscard]] uint32_t GetDescriptorIndex() const override { return m_descriptor.index; }

    private:
        std::shared_ptr<DescriptorHeap> m_cbv_srv_uav_heap;
        Descriptor m_descriptor{};
        std::shared_ptr<Context> m_context;
    };
}  // namespace Swift::D3D12
