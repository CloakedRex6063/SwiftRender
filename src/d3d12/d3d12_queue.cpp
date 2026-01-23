#include "d3d12/d3d12_queue.hpp"
#include "d3d12_helpers.hpp"

Swift::D3D12::Queue::Queue(ID3D12Device14* device, const QueueCreateInfo& info)
{
    const D3D12_COMMAND_QUEUE_DESC queue_desc = {
        .Type = ToCommandType(info.type),
        .Priority = ToCommandPriority(info.priority),
        .Flags = D3D12_COMMAND_QUEUE_FLAG_NONE,
        .NodeMask = 0,
    };
    device->CreateCommandQueue(&queue_desc, IID_PPV_ARGS(&m_queue));
    device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_fence));
    std::wstring name{info.name.begin(), info.name.end()};
    m_queue->SetName(name.c_str());
}

Swift::D3D12::Queue::~Queue()
{
    m_fence->Release();
    m_queue->Release();
}

void Swift::D3D12::Queue::Wait(const uint64_t fence_value)
{
    if (m_fence->GetCompletedValue() < fence_value)
    {
        m_fence->SetEventOnCompletion(fence_value, nullptr);
    }
}

void Swift::D3D12::Queue::WaitIdle()
{
    m_fence_value++;
    m_queue->Signal(m_fence, m_fence_value);
    m_fence->SetEventOnCompletion(m_fence_value, nullptr);
}

uint64_t Swift::D3D12::Queue::Execute(const std::span<ICommand*> commands)
{
    std::array<ID3D12CommandList*, 8> command_lists;
    for (size_t i = 0; i < commands.size(); i++)
    {
        command_lists[i] = static_cast<ID3D12CommandList*>(commands[i]->GetCommandList());
    }
    m_queue->ExecuteCommandLists(static_cast<uint32_t>(commands.size()), command_lists.data());
    m_fence_value++;
    m_queue->Signal(m_fence, m_fence_value);
    return m_fence_value;
}
