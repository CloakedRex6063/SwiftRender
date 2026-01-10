#include "hello_grass_inc.slang"

groupshared Payload s_Payload;
groupshared uint s_VisibleCount;

[NumThreads(128, 1, 1)]
[shader("amplification")]
void ampl_main(uint gtid : SV_GroupThreadID, uint gid : SV_GroupID)
{
    if (gtid == 0) s_VisibleCount = 0;
    GroupMemoryBarrierWithGroupSync();

    uint base_patch = gid * 128;
    uint patch_id = base_patch + gtid;

    bool visible = false;

    if (patch_id < PushConstants.grass_count)
    {
        if (bool(PushConstants.cull))
        {
            var grass_buffer = DescriptorHandle<StructuredBuffer<GrassPatch>>(GlobalConstants.grass_buffer_index);
            GrassPatch grass_patch = grass_buffer[patch_id];

            var frustum_buffer = DescriptorHandle<StructuredBuffer<Frustum>>(GlobalConstants.frustum_buffer_index);
            visible = IsInFrustum(frustum_buffer[0], PushConstants.radius, grass_patch.position, bool(PushConstants.cull_z));
        }
        else
        {
            visible = true;
        }
    }

    if (visible)
    {
        uint index;
        InterlockedAdd(s_VisibleCount, 1, index);
        s_Payload.patch_id[index] = patch_id;

        const uint vertices_per_blade_edge = 6;
        const uint blades_per_patch = GROUP_SIZE / vertices_per_blade_edge;

        var grass_buffer = DescriptorHandle<StructuredBuffer<GrassPatch>>(GlobalConstants.grass_buffer_index);
        GrassPatch grass_patch = grass_buffer[patch_id];

        float distance_to_camera = distance(GlobalConstants.cam_pos, grass_patch.position);

        s_Payload.blade_count_f[index] = lerp(float(blades_per_patch), 2.0, saturate(distance_to_camera / PushConstants.lod_distance));
    }

    GroupMemoryBarrierWithGroupSync();

    DispatchMesh(s_VisibleCount, 1, 1, s_Payload);
}

groupshared float4 blade_randoms[21];

[outputtopology("triangle")]
[numthreads(GROUP_SIZE, 1, 1)]
[shader("mesh")]
void mesh_main(
    uint gtid : SV_GroupThreadID,
    uint gid : SV_GroupID,
    in payload Payload payload,
    OutputVertices<Vertex, 252> verts,
    OutputIndices<uint3, 210> triangles)
{
    uint patch_id = payload.patch_id[gid];

    const uint vertices_per_blade_edge = 6;
    const uint blades_per_patch = GROUP_SIZE / vertices_per_blade_edge;
    const uint triangles_per_blade = 10;
    const float grass_leaning = 0.7;
    const float screen_space_thickness = 0.002;

    uint grass_id = gtid / vertices_per_blade_edge;
    uint local_id = gtid % vertices_per_blade_edge;

    var grass_buffer = DescriptorHandle<StructuredBuffer<GrassPatch>>(GlobalConstants.grass_buffer_index);
    GrassPatch grass_patch = grass_buffer[patch_id];

    float blade_count_f = payload.blade_count_f[gid];
    uint blade_count = uint(ceil(blade_count_f));

    uint actual_verts = blade_count * vertices_per_blade_edge * 2;
    uint actual_tris = blade_count * triangles_per_blade;

    SetMeshOutputCounts(actual_verts, actual_tris);

    if(grass_id >= blade_count) return;

    if(local_id == 0)
    {
        uint seed_height = grass_id * 73856093u ^ patch_id * 19349663u;
        uint seed_position = grass_id * 83492791u ^ patch_id * 50331653u;
        uint seed_rotation = grass_id * 19349669u ^ patch_id * 73856093u;

        float2 rand_position = RandomPointInCircle(seed_position, PushConstants.radius);
        float rand_height = Hash(seed_height);
        float rand_rotation = Hash(seed_rotation) * 2.0 * PI;

        blade_randoms[grass_id] = float4(rand_position, rand_height, rand_rotation);
    }

    GroupMemoryBarrierWithGroupSync();

    float4 randoms = blade_randoms[grass_id];
    float2 rand_position = randoms.xy;
    float rand_height = randoms.z;
    float rotation = randoms.w;

    float height = grass_patch.height * (1.0 + 0.5 * rand_height);
    float3 position = grass_patch.position + float3(rand_position.x, 0, rand_position.y);

    float3 up = float3(0, 1, 0);
    float3 base_right = float3(1, 0, 0);
    float3 blade_direction = float3(cos(rotation), 0, sin(rotation));

    float3 wind_direction = ApplyWind(position);

    float3 p0 = position;
    float3 p1 = p0 + float3(0, height * 0.5, 0);
    float base_lean = 0.3 + 0.4 * rand_height;
    float3 p2 = p0 + float3(0, height, 0) + blade_direction * height * base_lean + wind_direction;
    MakePersistentLength(p0, p1, p2, height);

    float t = local_id / float(vertices_per_blade_edge - 1);

    float3 tangent  = normalize(BezierDerivative(p0, p1, p2, t));

    float3 vertex_position = Bezier(p0, p1, p2, t);

    float3 side_vec = normalize(float3(blade_direction.y, 0, -blade_direction.x));
    float3 normal = cross(side_vec, tangent);

    float width = PushConstants.width * (1.0 - t);

    width *= blades_per_patch / blade_count_f;

    if (grass_id == (blade_count - 1)) {
        width *= frac(blade_count_f);
    }

    float3 left_vector = vertex_position - blade_direction * width * 0.5;
    float3 right_vector = vertex_position + blade_direction * width * 0.5;

    float3 view_dir = normalize(GlobalConstants.cam_pos - vertex_position);
    float view_dot_normal = saturate(dot(normal.xz, view_dir.xz));
    float view_space_thicken_factor = EaseOut(1.0 - view_dot_normal, 4.0);
    float3 adjustment_direction = normalize(cross(view_dir, normal));

    if(bool(PushConstants.apply_view_space_thicken))
    {
        left_vector += view_space_thicken_factor * adjustment_direction * width;
        right_vector += view_space_thicken_factor * adjustment_direction * width;
    }

    uint base_vertex_index = grass_id * vertices_per_blade_edge * 2;

    float density = float(blade_count) / float(blades_per_patch);
    verts[base_vertex_index + local_id * 2].position = mul(GlobalConstants.view_proj, float4(left_vector, 1));
    verts[base_vertex_index + local_id * 2].normal = normal;
    verts[base_vertex_index + local_id * 2].uv = float2(0.0, t);
    verts[base_vertex_index + local_id * 2].world_pos = left_vector;
    verts[base_vertex_index + local_id * 2].density = density;

    verts[base_vertex_index + local_id * 2 + 1].position = mul(GlobalConstants.view_proj, float4(right_vector, 1));
    verts[base_vertex_index + local_id * 2 + 1].normal = normal;
    verts[base_vertex_index + local_id * 2 + 1].uv = float2(1.0, t);
    verts[base_vertex_index + local_id * 2 + 1].world_pos = right_vector;
    verts[base_vertex_index + local_id * 2 + 1].density = density;

    if(local_id < (vertices_per_blade_edge - 1))
    {
        uint base_tri_index = grass_id * triangles_per_blade;
        uint v0 = base_vertex_index + local_id * 2;

        triangles[base_tri_index + local_id * 2] = uint3(v0, v0 + 1, v0 + 2);
        triangles[base_tri_index + local_id * 2 + 1] = uint3(v0 + 1, v0 + 3, v0 + 2);
    }
}

[shader("pixel")]
float4 pixel_main(Vertex input) : SV_TARGET
{
    float3 normal = input.normal;

    float3 base_color = float3(0.05, 0.2, 0.01);
    float3 tip_color = float3(0.5, 0.5, 0.1);
    float3 grass_color = lerp(base_color, tip_color, EaseIn(input.uv.y, 4.0));

    float selfshadow = pow(input.uv.y, 1.5);

    float ao_for_density = lerp(1.0, 0.25, input.density);
    float ao = lerp(ao_for_density, 1.0, EaseIn(input.uv.y, 2.0));

    float occlusion = ao * selfshadow;

    float3 lo = grass_color + 0.3 * occlusion;

    return float4(lo, 1.0);
}