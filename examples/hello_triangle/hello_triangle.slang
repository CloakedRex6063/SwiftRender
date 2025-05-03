#version 460

const float2 positions[3] = {
    float2(-0.5, -0.5),  // Vertex 1: position (-0.5, -0.5)
    float2( 0.5, -0.5),  // Vertex 2: position (0.5, -0.5)
    float2( 0.0,  0.5)   // Vertex 3: position (0.0, 0.5)
};

const float3 colors[3] = {
    float3(1.0, 0.0, 0.0),  // Vertex 1: red color
    float3(0.0, 1.0, 0.0),  // Vertex 2: green color
    float3(0.0, 0.0, 1.0)   // Vertex 3: blue color
};

struct VertexOutput
{
    float4 position : SV_POSITION;
    float3 color    : COLOR0;
};

[shader("vertex")]
VertexOutput VsMain(uint vertexIndex : SV_VertexID)
{
    VertexOutput output;
    output.position = float4(positions[vertexIndex], 0.0, 1.0);
    output.color = float3(colors[vertexIndex]);
    return output;
}

[shader("fragment")]
float4 FsMain(VertexOutput input) : SV_TARGET
{
    return float4(input.color, 1.0);
}
