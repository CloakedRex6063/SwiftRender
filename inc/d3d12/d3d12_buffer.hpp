#pragma once
#include "d3d12_context.hpp"
#include "swift_buffer.hpp"

namespace Swift::D3D12
{
    class Buffer final : public IBuffer
    {
    public:
        Buffer(Context* context, const BufferCreateInfo& info);
        SWIFT_NO_COPY(Buffer);
        SWIFT_NO_MOVE(Buffer);
        SWIFT_DESTRUCT(Buffer);
        void Write(const void* data, uint64_t offset, uint64_t size, bool one_time = false) override;
        void Read(uint32_t offset, uint32_t size, void* data) override;
        void Map(void* readback = nullptr, uint32_t size = 0) override;
        void Unmap() override;
        static D3D12_RESOURCE_DESC GetResourceDesc(const BufferCreateInfo& info);
        static ID3D12Resource* CreateCommittedResource(ID3D12Device14* device, const BufferCreateInfo& info);
        [[nodiscard]] void* GetResource() override { return m_resource; }
        [[nodiscard]] uint64_t GetVirtualAddress() override { return m_resource->GetGPUVirtualAddress(); }

    private:
        Context* m_context;
        ID3D12Resource* m_resource = nullptr;
    };
}  // namespace Swift::D3D12
