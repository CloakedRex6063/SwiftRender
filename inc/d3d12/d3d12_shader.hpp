#pragma once
#include "swift_shader.hpp"
#include "swift_structs.hpp"
#define NOMINMAX
#include "directx/d3d12.h"
#include "span"

namespace Swift::D3D12
{
    class Shader final : public IShader
    {
    public:
        Shader(ID3D12Device14* device, ID3D12RootSignature* root_signature, const GraphicsShaderCreateInfo& create_info);
        Shader(ID3D12Device14* device, ID3D12RootSignature* root_signature, const ComputeShaderCreateInfo& create_info);

        ~Shader() override;

        [[nodiscard]] void* GetPipeline() const override { return m_pso; }

    private:
        friend class Command;

        ID3D12PipelineState* m_pso = nullptr;
    };
}  // namespace Swift::D3D12
