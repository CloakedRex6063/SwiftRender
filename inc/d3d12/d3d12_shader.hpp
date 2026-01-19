#pragma once
#include "swift_shader.hpp"
#include "swift_structs.hpp"
#include "directx/d3d12.h"
#include "span"

namespace Swift::D3D12
{
    class Shader final : public IShader
    {
    public:
        Shader(ID3D12Device14* device, const GraphicsShaderCreateInfo& create_info);

        Shader(ID3D12Device14* device, const ComputeShaderCreateInfo& create_info);

        ~Shader() override;

        [[nodiscard]] void* GetPipeline() const override { return m_pso; }

    private:
        friend class Command;

        void CreateRootSignature(ID3D12Device14* device,
                                 std::span<const SamplerDescriptor> samplers,
                                 std::span<const Descriptor> descriptors);

        ID3D12RootSignature* m_root_signature = nullptr;
        ID3D12PipelineState* m_pso = nullptr;
    };
}  // namespace Swift::D3D12
