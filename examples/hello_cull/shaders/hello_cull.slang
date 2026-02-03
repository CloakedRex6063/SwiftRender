#include "hello_cull_inc.slang"

struct TaskPayload
{
    uint meshlet_index[32];
};

groupshared TaskPayload s_Payload;

[NumThreads(32, 1, 1)]
[shader("amplification")]
void ampl_main(uint dtid: SV_DispatchThreadID, uint gtid: SV_GroupThreadID, uint gid: SV_GroupID)
{
    bool visible = false;

    if(bool(PushConstants.should_cull))
    {
        if(dtid < PushConstants.meshlet_count)
        {
            var transform_buffer = DescriptorHandle<StructuredBuffer<float4x4>>(GlobalConstants.transform_buffer_index);
            var transform = transform_buffer[PushConstants.transform_index];
            var frustum_buffer = DescriptorHandle<StructuredBuffer<Frustum>>(GlobalConstants.frustum_buffer_index);
            var bounding_buffer = DescriptorHandle<StructuredBuffer<BoundingSphere>>(GlobalConstants.bounding_buffer_index);
            visible = IsInFrustum(frustum_buffer[0], bounding_buffer[dtid + PushConstants.bounding_offset], transform);
        }
    }
    else
    {
        visible = true;
    }

    if (visible)
    {
        uint index = WavePrefixCountBits(visible);
        s_Payload.meshlet_index[index] = dtid;
    }

    uint visibleCount = WaveActiveCountBits(visible);
    DispatchMesh(visibleCount, 1, 1, s_Payload);
}

[outputtopology("triangle")]
[numthreads(128, 1, 1)]
[shader("mesh")]
void mesh_main(uint gtid: SV_GroupThreadID,
               uint gid: SV_GroupID,
               in payload TaskPayload payload,
               OutputVertices<OutVertex, 64> verts,
               OutputIndices<uint3, 124> triangles)
{
    var vertex_buffer = DescriptorHandle<StructuredBuffer<Vertex>>(PushConstants.vertex_buffer_index);
    var meshlet_buffer = DescriptorHandle<StructuredBuffer<Meshlet>>(PushConstants.mesh_buffer_index);
    var mesh_vertex_buffer = DescriptorHandle<StructuredBuffer<uint>>(PushConstants.mesh_vertex_buffer_index);
    var mesh_triangle_buffer = DescriptorHandle<StructuredBuffer<uint>>(PushConstants.mesh_triangle_buffer_index);
    var transform_buffer = DescriptorHandle<StructuredBuffer<float4x4>>(GlobalConstants.transform_buffer_index);

    uint meshlet_index = payload.meshlet_index[gid];
    Meshlet meshlet = meshlet_buffer[meshlet_index];
    SetMeshOutputCounts(meshlet.vertex_count, meshlet.triangle_count);

    if (gtid < meshlet.triangle_count)
    {
        uint packed = mesh_triangle_buffer[meshlet.triangle_offset + gtid];
        uint idx0 = (packed >> 0) & 0xFF;
        uint idx1 = (packed >> 8) & 0xFF;
        uint idx2 = (packed >> 16) & 0xFF;
        triangles[gtid] = uint3(idx0, idx1, idx2);
    }

    if (gtid < meshlet.vertex_count)
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
    if (material.albedo_index != -1)
    {
        var albedo_texture = DescriptorHandle<Texture2D>(uint(material.albedo_index));
        albedo *= albedo_texture.Sample(samp, input.uv).rgb;
    }

    float metallic = material.metallic;
    float roughness = material.roughness;
    if (material.metal_rough_index != -1)
    {
        var metal_rough_texture = DescriptorHandle<Texture2D>(uint(material.metal_rough_index));
        float3 mr = metal_rough_texture.Sample(samp, input.uv).rgb;
        metallic *= mr.g;
        roughness *= mr.b;
    }

    float3 emissive = material.emissive;
    if (material.emissive_index != -1)
    {
        var emissive_texture = DescriptorHandle<Texture2D>(uint(material.emissive_index));
        emissive *= emissive_texture.Sample(samp, input.uv).rgb;
    }

    float3 world_pos = input.world_pos;
    float3 n = input.normal;
    float3 v = normalize(GlobalConstants.cam_pos - world_pos);
    float3 f0 = float3(0.04, 0.04, 0.04);
    f0 = lerp(f0, albedo, metallic);

    float occlusion = 1.0;
    if (material.occlusion_index != -1)
    {
        var occlusion_texture = DescriptorHandle<Texture2D>(uint(material.occlusion_index));
        occlusion *= occlusion_texture.Sample(samp, input.uv).r;
    }

    float3 lo = float3(0.0, 0.0, 0.0);
    var point_light_buffer = DescriptorHandle<StructuredBuffer<PointLight>>(GlobalConstants.point_light_buffer_index);
    for (int i = 0; i < GlobalConstants.point_light_count; i++)
    {
        PointLight point_light = point_light_buffer[i];
        float3 l = normalize(point_light.position - world_pos);
        float3 h = normalize(v + l);
        float distance = length(point_light.position - world_pos);
        float attenuation = 1.0 / (distance * distance);
        float3 radiance = point_light.color * point_light.intensity * attenuation;
        lo += ApplyLight(n, h, v, l, f0, albedo, radiance, roughness, metallic);
    };

    var dir_light_buffer = DescriptorHandle<StructuredBuffer<DirectionalLight>>(GlobalConstants.dir_light_buffer_index);
    for (int i = 0; i < GlobalConstants.dir_light_count; i++)
    {
        DirectionalLight dir_light = dir_light_buffer[i];
        float3 l = normalize(dir_light.direction);
        float3 h = normalize(v + l);
        float3 radiance = dir_light.color * dir_light.intensity;
        lo += ApplyLight(n, h, v, l, f0, albedo, radiance, roughness, metallic);
    };

    float3 ambient = float3(0.03, 0.03, 0.03) * albedo * occlusion;
    float3 color = ambient + lo + emissive;
    color = color / (color + float3(1.0, 1.0, 1.0));
    return float4(color, 1.0);
}
