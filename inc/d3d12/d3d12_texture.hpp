#pragma once
#include "d3d12_context.hpp"
#include "swift_texture.hpp"

namespace Swift::D3D12
{
    class Texture final : public ITexture
    {
    public:
        Texture(Context* context,
                const std::shared_ptr<DescriptorHeap>& rtv_heap,
                const std::shared_ptr<DescriptorHeap>& dsv_heap,
                const std::shared_ptr<DescriptorHeap>& srv_heap,
                const TextureCreateInfo& info);
        SWIFT_NO_COPY(Texture);
        SWIFT_NO_MOVE(Texture);
        ~Texture() override;

        const std::optional<Descriptor>& GetRTVDescriptor() const { return m_rtv_descriptor; }
        const std::optional<Descriptor>& GetDSVDescriptor() const { return m_dsv_descriptor; }
        const std::optional<Descriptor>& GetSRVDescriptor() const { return m_srv_descriptor; }
        [[nodiscard]] uint32_t GetDescriptorIndex() override { return m_srv_descriptor->index; }

    private:
        std::shared_ptr<DescriptorHeap> m_rtv_heap;
        std::shared_ptr<DescriptorHeap> m_dsv_heap;
        std::shared_ptr<DescriptorHeap> m_srv_heap;
        std::optional<Descriptor> m_rtv_descriptor = std::nullopt;
        std::optional<Descriptor> m_dsv_descriptor = std::nullopt;
        std::optional<Descriptor> m_srv_descriptor = std::nullopt;
    };
}  // namespace Swift::D3D12
