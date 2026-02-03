struct Vertex
{
    float3 position;
    float uv_x;
    float3 normal;
    float uv_y;
    float4 tangent;
};

struct PointLight
{
    float3 position;
    float intensity;
    float3 color;
    uint range;
};

struct DirectionalLight
{
    float3 direction;
    float intensity;
    float3 color;
    uint cast_shadows;
};

struct OutVertex
{
    float4 position : SV_POSITION;
    float3 world_pos : POSITION;
    float2 uv : TEXCOORD0;
    float3 normal : NORMAL;
    float4 tangent : TANGENT;
};

struct Meshlet
{
    uint vertex_offset;
    uint triangle_offset;
    uint vertex_count;
    uint triangle_count;
};

struct Material
{
    float4 albedo;

    float3 emissive;
    int albedo_index;

    int emissive_index;
    int metal_rough_index;
    float metallic;
    float roughness;

    int normal_index;
    int occlusion_index;
    float alpha_cutoff;
    uint alpha_mode;
};

struct Plane
{
    float3 normal;
    float distance;
};

struct Frustum
{
    float4 planes[6];
};

struct CullData
{
    float3 center;
    float radius;
    float3 cone_apex;
    uint32_t cone_packed;
};

struct PushConstant
{
    uint vertex_buffer_index;
    uint mesh_buffer_index;
    uint mesh_vertex_buffer_index;
    uint mesh_triangle_buffer_index;
    uint material_index;
    uint transform_index;
    uint meshlet_count;
    uint bounding_offset;
    uint should_cull;
};

ConstantBuffer<PushConstant> PushConstants : register(b0);

struct GlobalConstant
{
    float4x4 view_proj;

    float3 cam_pos;
    uint material_buffer_index;

    uint transform_buffer_index;
    uint point_light_buffer_index;
    uint dir_light_buffer_index;
    uint point_light_count;

    uint dir_light_count;
    uint frustum_buffer_index;
    uint bounding_buffer_index;
    float padding;
};
ConstantBuffer<GlobalConstant> GlobalConstants : register(b1);

static const float PI = 3.14159265359;

float DistributionGGX(float3 N, float3 H, float roughness)
{
    float a = roughness * roughness;
    float a2 = a * a;
    float n_dot_h = max(dot(N, H), 0.0);
    float n_dot_h_2 = n_dot_h * n_dot_h;

    float nom = a2;
    float denom = (n_dot_h_2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;

    return nom / denom;
}
// ----------------------------------------------------------------------------
float GeometrySchlickGGX(float n_dot_v, float roughness)
{
    float r = (roughness + 1.0);
    float k = (r * r) / 8.0;

    float nom = n_dot_v;
    float denom = n_dot_v * (1.0 - k) + k;

    return nom / denom;
}
// ----------------------------------------------------------------------------
float GeometrySmith(float3 n, float3 v, float3 l, float roughness)
{
    float n_dot_v = max(dot(n, v), 0.0);
    float n_dot_l = max(dot(n, l), 0.0);
    float ggx2 = GeometrySchlickGGX(n_dot_v, roughness);
    float ggx1 = GeometrySchlickGGX(n_dot_l, roughness);

    return ggx1 * ggx2;
}

// ----------------------------------------------------------------------------
float3 FresnelSchlick(float cos_theta, float3 f0)
{
    return f0 + (1.0 - f0) * pow(clamp(1.0 - cos_theta, 0.0, 1.0), 5.0);
}

// ----------------------------------------------------------------------------

float3
ApplyLight(float3 n, float3 h, float3 v, float3 l, float3 f0, float3 albedo, float3 radiance, float roughness, float metallic)
{
    float ndf = DistributionGGX(n, h, roughness);
    float g = GeometrySmith(n, v, l, roughness);
    float3 f = FresnelSchlick(max(dot(h, v), 0.0), f0);

    float3 num = ndf * g * f;
    float denom = 4.0 * max(dot(n, v), 0.0) * max(dot(n, l), 0.0) + 0.0001;
    float3 specular = num / denom;

    float3 ks = f;
    float3 kd = float3(1, 1, 1) - ks;
    kd *= 1.0 - metallic;

    float n_dot_l = max(dot(n, l), 0.0);
    return (kd * albedo / PI + specular) * radiance * n_dot_l;
}

bool IsVisible(Frustum frustum, CullData cull_data, float4x4 transform, float3 camera_position)
{
    float4 center = mul(transform, float4(cull_data.center, 1));
    var scale = GetScale(transform);
    var max_scale = max(scale.x, max(scale.y, scale.z));
    var radius = cull_data.radius * max_scale;
    for (int i = 0; i < 6; ++i)
    {
        if (dot(center, frustum.planes[i]) < -radius)
        {
            return false;
        }
    }

    int3 axis_s8 = int3(
        int(cull_data.cone_packed << 24) >> 24,
        int(cull_data.cone_packed << 16) >> 24,
        int(cull_data.cone_packed <<  8) >> 24
    );
    int cutoff_s8 = int(cull_data.cone_packed) >> 24;

    float3 cone_axis = float3(axis_s8) / 127.0;
    float cone_cutoff = float(cutoff_s8) / 127.0;

    if (cone_cutoff > 1.0)
        return true;

    float3 apex_world = mul(transform, float4(cull_data.cone_apex, 1.0)).xyz;
    float3 axis_world = normalize(mul(transform, float4(cone_axis, 0.0))).xyz;
    float3 view = normalize(camera_position - apex_world);

    if (dot(view, -axis_world) > cone_cutoff)
        return false;

    return true;
}

float3 GetScale(float4x4 m)
{
    return float3(length(m[0].xyz), length(m[1].xyz), length(m[2].xyz));
}
