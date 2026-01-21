#include "d3d12/d3d12_shader.hpp"
#include "d3d12/d3d12_helpers.hpp"
#include "directx/d3dx12_pipeline_state_stream.h"

Swift::D3D12::Shader::Shader(ID3D12Device14* device, const GraphicsShaderCreateInfo& create_info)
{
    CreateRootSignature(device, create_info.static_samplers, create_info.descriptors);

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
        .pRootSignature = m_root_signature,
        .AS = as_code,
        .MS = ms_code,
        .PS = ps_code,
        .SampleMask = D3D12_DEFAULT_SAMPLE_MASK,
        .RasterizerState =
            {
                .FillMode = ToFillMode(create_info.rasterizer_state.fill_mode),
                .CullMode = ToCullMode(create_info.rasterizer_state.cull_mode),
                .FrontCounterClockwise = ToFrontFace(create_info.rasterizer_state.front_face),
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

Swift::D3D12::Shader::Shader(ID3D12Device14* device, const ComputeShaderCreateInfo& create_info)
{
    CreateRootSignature(device, create_info.static_samplers, create_info.descriptors);
    const D3D12_SHADER_BYTECODE bytecode = {
        .pShaderBytecode = create_info.code.data(),
        .BytecodeLength = create_info.code.size() * sizeof(uint8_t),
    };

    const D3D12_COMPUTE_PIPELINE_STATE_DESC pso_desc = {
        .pRootSignature = m_root_signature,
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

void Swift::D3D12::Shader::CreateRootSignature(ID3D12Device14* device,
                                               std::span<const SamplerDescriptor> samplers,
                                               std::span<const Descriptor> descriptors)
{
    std::vector<D3D12_ROOT_PARAMETER1> root_params;
    root_params.reserve(descriptors.size() + 1);
    D3D12_ROOT_PARAMETER1 push_constants{
        .ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS,
        .Constants =
            {
                .ShaderRegister = 0,
                .RegisterSpace = 0,
                .Num32BitValues = 32,
            },
        .ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL,
    };
    root_params.emplace_back(push_constants);
    for (const auto [shader_register, register_space, descriptor_type, shader_visibility] : descriptors)
    {
        D3D12_ROOT_PARAMETER1 param_desc = {
            .ParameterType = ToDescriptorType(descriptor_type),
            .Descriptor =
                {
                    .ShaderRegister = shader_register,
                    .RegisterSpace = register_space,
                },
            .ShaderVisibility = ToShaderVisibility(shader_visibility),
        };
        root_params.emplace_back(param_desc);
    }
    std::vector<D3D12_STATIC_SAMPLER_DESC> sampler_descs;

    for (uint32_t i = 0; i < samplers.size(); ++i)
    {
        const auto& [min_filter, mag_filter, wrap_u, wrap_y, wrap_w] = samplers[i];
        D3D12_STATIC_SAMPLER_DESC static_sampler_desc = {
            .Filter = ToFilter(min_filter, mag_filter),
            .AddressU = ToWrap(wrap_u),
            .AddressV = ToWrap(wrap_y),
            .AddressW = ToWrap(wrap_w),
            .MipLODBias = 0,
            .MaxAnisotropy = D3D12_DEFAULT_MAX_ANISOTROPY,
            .ComparisonFunc = D3D12_COMPARISON_FUNC_ALWAYS,
            .BorderColor = D3D12_STATIC_BORDER_COLOR_OPAQUE_BLACK,
            .MinLOD = 0,
            .MaxLOD = 13,
            .ShaderRegister = i,
            .RegisterSpace = 0,
            .ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL,
        };
        sampler_descs.emplace_back(static_sampler_desc);
    }
    const D3D12_VERSIONED_ROOT_SIGNATURE_DESC root_signature_desc = {
        .Version = D3D_ROOT_SIGNATURE_VERSION_1_1,
        .Desc_1_1 = {
            .NumParameters = static_cast<uint32_t>(root_params.size()),
            .pParameters = root_params.data(),
            .NumStaticSamplers = static_cast<uint32_t>(sampler_descs.size()),
            .pStaticSamplers = sampler_descs.data(),
            .Flags = D3D12_ROOT_SIGNATURE_FLAG_CBV_SRV_UAV_HEAP_DIRECTLY_INDEXED,
        }};

    ID3DBlob* error_blob = nullptr;
    ID3DBlob* root_signature_blob = nullptr;
    if (const auto result = D3D12SerializeVersionedRootSignature(&root_signature_desc, &root_signature_blob, &error_blob);
        result != S_OK)
    {
        error_blob->Release();
    }

    [[maybe_unused]]
    const auto result = device->CreateRootSignature(0,
                                                    root_signature_blob->GetBufferPointer(),
                                                    root_signature_blob->GetBufferSize(),
                                                    IID_PPV_ARGS(&m_root_signature));
    root_signature_blob->Release();
}
