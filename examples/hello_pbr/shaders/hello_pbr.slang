#include "hello_pbr_inc.slang"

[outputtopology("triangle")]
[numthreads(128, 1, 1)]
[shader("mesh")]
void mesh_main(
    uint gtid : SV_GroupThreadID,
    uint gid  : SV_GroupID,
    OutputVertices<OutVertex, 64> verts,
    OutputIndices<uint3, 124> triangles)
{
    var vertex_buffer = DescriptorHandle<StructuredBuffer<Vertex>>(PushConstants.vertex_buffer_index);
    var meshlet_buffer = DescriptorHandle<StructuredBuffer<Meshlet>>(PushConstants.mesh_buffer_index);
    var mesh_vertex_buffer = DescriptorHandle<StructuredBuffer<uint>>(PushConstants.mesh_vertex_buffer_index);
    var mesh_triangle_buffer = DescriptorHandle<StructuredBuffer<uint>>(PushConstants.mesh_triangle_buffer_index);
    var transform_buffer = DescriptorHandle<StructuredBuffer<float4x4>>(GlobalConstants.transform_buffer_index);
    Meshlet meshlet = meshlet_buffer[gid];
    SetMeshOutputCounts(meshlet.vertex_count, meshlet.triangle_count);

    if(gtid < meshlet.triangle_count)
    {
        uint packed = mesh_triangle_buffer[meshlet.triangle_offset + gtid];
        uint idx0 = (packed >> 0) & 0xFF;
        uint idx1 = (packed >> 8) & 0xFF;
        uint idx2 = (packed >> 16) & 0xFF;
        triangles[gtid] = uint3(idx0, idx1, idx2);
    }

    if(gtid < meshlet.vertex_count)
    {
        uint vertex_index = meshlet.vertex_offset + gtid;
        vertex_index = mesh_vertex_buffer[vertex_index];
        Vertex vertex = vertex_buffer[vertex_index];
        float4 world_pos = mul(transform_buffer[PushConstants.transform_index], float4(vertex.position, 1.0));
        verts[gtid].position = mul(GlobalConstants.view_proj, world_pos);
        verts[gtid].world_pos = world_pos.xyz;
        verts[gtid].normal = vertex.normal;
        verts[gtid].uv = float2(vertex.uv_x, vertex.uv_y);
        verts[gtid].tangent = vertex.tangent;
    }
}

SamplerState samp : register(s0);

[shader("pixel")]
float4 pixel_main(OutVertex input) : SV_TARGET
{
    var material_buffer = DescriptorHandle<StructuredBuffer<Material>>(GlobalConstants.material_buffer_index);
    Material material = material_buffer[PushConstants.material_index];

    float3 albedo = material.albedo.rgb;
    if(material.albedo_index != -1)
    {
        var albedo_texture = DescriptorHandle<Texture2D>(uint(material.albedo_index));
        albedo *= albedo_texture.Sample(samp, input.uv).rgb;
    }

    float3 emissive = material.emissive;
    if(material.emissive_index != -1)
    {
        var emissive_texture = DescriptorHandle<Texture2D>(uint(material.emissive_index));
        emissive *= emissive_texture.Sample(samp, input.uv).rgb;
    }

    float3 color = albedo + emissive;
    return float4(color, 1.0);
}