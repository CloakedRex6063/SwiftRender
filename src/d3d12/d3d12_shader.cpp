#include "d3d12/d3d12_shader.hpp"
#include "d3d12/d3d12_helpers.hpp"
#include "directx/d3dx12_pipeline_state_stream.h"

Swift::D3D12::Shader::Shader(ID3D12Device14* device,
                             ID3D12RootSignature* root_signature,
                             const GraphicsShaderCreateInfo& create_info)
    : IShader(ShaderType::eGraphics)
{
    const D3D12_SHADER_BYTECODE as_code{
        .pShaderBytecode = create_info.amplify_code.data(),
        .BytecodeLength = create_info.amplify_code.size() * sizeof(uint8_t),
    };
    const D3D12_SHADER_BYTECODE ms_code{
        .pShaderBytecode = create_info.mesh_code.data(),
        .BytecodeLength = create_info.mesh_code.size() * sizeof(uint8_t),
    };
    const D3D12_SHADER_BYTECODE ps_code{
        .pShaderBytecode = create_info.pixel_code.data(),
        .BytecodeLength = create_info.pixel_code.size() * sizeof(uint8_t),
    };

    D3DX12_MESH_SHADER_PIPELINE_STATE_DESC mesh_desc = {
        .pRootSignature = root_signature,
        .AS = as_code,
        .MS = ms_code,
        .PS = ps_code,
        .SampleMask = D3D12_DEFAULT_SAMPLE_MASK,
        .RasterizerState =
            {
                .FillMode = ToFillMode(create_info.rasterizer_state.fill_mode),
                .CullMode = ToCullMode(create_info.rasterizer_state.cull_mode),
                .FrontCounterClockwise = ToFrontFace(create_info.rasterizer_state.front_face),
                .DepthBias = create_info.rasterizer_state.depth_bias,
                .DepthBiasClamp = create_info.rasterizer_state.depth_bias_clamp,
                .SlopeScaledDepthBias = create_info.rasterizer_state.slope_scaled_depth_bias,
                .DepthClipEnable = create_info.rasterizer_state.depth_clip_enable,
            },
        .DepthStencilState =
            D3D12_DEPTH_STENCIL_DESC{
                .DepthEnable = create_info.depth_stencil_state.depth_enable,
                .DepthWriteMask = static_cast<D3D12_DEPTH_WRITE_MASK>(create_info.depth_stencil_state.depth_enable),
                .DepthFunc = ToDepthTest(create_info.depth_stencil_state.depth_test),
                .StencilEnable = create_info.depth_stencil_state.stencil_enable,
                .StencilReadMask = D3D12_DEFAULT_STENCIL_READ_MASK,
                .StencilWriteMask = D3D12_DEFAULT_STENCIL_WRITE_MASK,
            },
        .PrimitiveTopologyType = ToPolygonMode(create_info.polygon_mode),
        .NumRenderTargets = static_cast<uint32_t>(create_info.rtv_formats.size()),
        .DSVFormat = create_info.dsv_format ? ToDXGIFormat(create_info.dsv_format.value()) : DXGI_FORMAT_UNKNOWN,
        .SampleDesc = {1, 0},
    };

    for (int i = 0; i < create_info.rtv_formats.size(); ++i)
    {
        mesh_desc.RTVFormats[i] = ToDXGIFormat(create_info.rtv_formats[i]);
        mesh_desc.BlendState.RenderTarget[i] = {
            .RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL,
        };
    }

    auto pso_stream = CD3DX12_PIPELINE_MESH_STATE_STREAM(mesh_desc);
    const D3D12_PIPELINE_STATE_STREAM_DESC stream_desc = {.SizeInBytes = sizeof(pso_stream),
                                                          .pPipelineStateSubobjectStream = &pso_stream};
    [[maybe_unused]]
    const auto result = device->CreatePipelineState(&stream_desc, IID_PPV_ARGS(&m_pso));
}

Swift::D3D12::Shader::Shader(ID3D12Device14* device,
                             ID3D12RootSignature* root_signature,
                             const ComputeShaderCreateInfo& create_info)
    : IShader(ShaderType::eCompute)
{
    const D3D12_SHADER_BYTECODE bytecode = {
        .pShaderBytecode = create_info.code.data(),
        .BytecodeLength = create_info.code.size() * sizeof(uint8_t),
    };

    const D3D12_COMPUTE_PIPELINE_STATE_DESC pso_desc = {
        .pRootSignature = root_signature,
        .CS = bytecode,
    };
    [[maybe_unused]]
    const auto result = device->CreateComputePipelineState(&pso_desc, IID_PPV_ARGS(&m_pso));
    if (result)
    {
        printf("%ld", result);
    }
}

Swift::D3D12::Shader::~Shader()
{
    if (m_pso)
    {
        m_pso->Release();
    }
}
