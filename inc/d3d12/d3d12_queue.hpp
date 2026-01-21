#pragma once
#include "swift_queue.hpp"
#include "swift_structs.hpp"
#include "directx/d3d12.h"

namespace Swift::D3D12
{
    class Queue final : public IQueue
    {
    public:
        SWIFT_NO_COPY(Queue);
        SWIFT_NO_MOVE(Queue);

        Queue(ID3D12Device14* device, const QueueCreateInfo& info);
        ~Queue() override;

        void* GetQueue() override { return m_queue; }
        void Wait(uint64_t fence_value) override;
        void WaitIdle() override;
        uint64_t Execute(std::span<ICommand*> commands) override;

    private:
        uint64_t m_fence_value = 0;
        ID3D12CommandQueue* m_queue = nullptr;
        ID3D12Fence* m_fence = nullptr;
    };
}  // namespace Swift::D3D12
