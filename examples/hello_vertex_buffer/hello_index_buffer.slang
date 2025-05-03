#version 460

struct VertexInput
{
    float2 position;
    float3 color;
}

struct VertexOutput
{
    float4 position : SV_POSITION;
    float3 color    : COLOR0;
};

[shader("vertex")]
VertexOutput VsMain(VertexInput input)
{
    VertexOutput output;
    output.position = float4(input.position, 0.0, 1.0);
    output.color = float3(input.color);
    return output;
}

[shader("fragment")]
float4 FsMain(VertexOutput input) : SV_TARGET
{
    return float4(input.color, 1.0);
}
