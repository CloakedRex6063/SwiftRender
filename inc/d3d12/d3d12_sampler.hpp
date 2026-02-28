#pragma once
#include "d3d12_descriptor.hpp"
#include "swift_sampler.hpp"

namespace Swift::D3D12
{
    class Context;

    class Sampler : public ISampler {
    public:
        Sampler(Context* context, const SamplerCreateInfo& create_info);
        ~Sampler() override;
        [[nodiscard]] uint32_t GetDescriptorIndex() const override { return m_data.index; }
        DescriptorData& GetDescriptorData() { return m_data; }

    private:
        DescriptorData m_data;
        Context* m_context = nullptr;
    };
}
