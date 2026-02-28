#include "d3d12/d3d12_buffer_view.hpp"

#include "swift_buffer.hpp"
#include "d3d12/d3d12_context.hpp"

Swift::D3D12::BufferView::BufferView(Context* m_context, IBuffer* buffer, const BufferViewCreateInfo& create_info)
    : IBufferView(buffer, create_info.type), m_context(m_context)
{
    auto* heap = m_context->GetCBVSRVUAVHeap();
    m_data = heap->Allocate();
    auto* device = static_cast<ID3D12Device*>(m_context->GetDevice());
    auto* resource = static_cast<ID3D12Resource*>(buffer->GetResource());
    const D3D12_SHADER_RESOURCE_VIEW_DESC desc{.Format = DXGI_FORMAT_UNKNOWN,
                                               .ViewDimension = D3D12_SRV_DIMENSION_BUFFER,
                                               .Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING,
                                               .Buffer = {
                                                   .FirstElement = create_info.first_element,
                                                   .NumElements = create_info.num_elements,
                                                   .StructureByteStride = create_info.element_size,
                                               }};
    device->CreateShaderResourceView(resource, &desc, m_data.cpu_handle);
}

Swift::D3D12::BufferView::~BufferView()
{
    auto* heap = m_context->GetCBVSRVUAVHeap();
    heap->Free(m_data);
}