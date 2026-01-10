struct VertexOutput
{
    float4 position : SV_Position;
    float3 color : COLOR;
};

[shader("mesh")]
[numthreads(1, 1, 1)]
[outputtopology("triangle")]
void mesh_main(
    uint gtid : SV_GroupThreadID,
    uint gid : SV_GroupID,
    OutputVertices<VertexOutput, 3> verts,
    OutputIndices<uint3, 1> tris)
{
    const float3 positions[3] = {
        float3( 0.0,  0.5, 0.0),
        float3( 0.5, -0.5, 0.0),
        float3(-0.5, -0.5, 0.0)
    };
    
    const float3 colors[3] = {
        float3(1.0, 0.0, 0.0),
        float3(0.0, 1.0, 0.0),
        float3(0.0, 0.0, 1.0)
    };

    SetMeshOutputCounts(3, 1);

    verts[0].position = float4(positions[0], 1.0);
    verts[0].color = colors[0];
    
    verts[1].position = float4(positions[1], 1.0);
    verts[1].color = colors[1];
    
    verts[2].position = float4(positions[2], 1.0);
    verts[2].color = colors[2];

    tris[0] = uint3(0, 1, 2);
}

[shader("pixel")]
float4 pixel_main(VertexOutput input) : SV_Target
{
    return float4(input.color, 1.0);
}