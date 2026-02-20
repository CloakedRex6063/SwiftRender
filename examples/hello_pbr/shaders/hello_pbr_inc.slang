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

struct PushConstant
{
    uint sampler_index;
    uint vertex_buffer_index;
    uint mesh_buffer_index;
    uint mesh_vertex_buffer_index;
    uint mesh_triangle_buffer_index;
    uint material_index;
    uint transform_index;
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
    float3 padding;
};
ConstantBuffer<GlobalConstant> GlobalConstants : register(b1);

static const float PI = 3.14159265359;

float DistributionGGX(float3 N, float3 H, float roughness)
{
    float a = roughness*roughness;
    float a2 = a*a;
    float n_dot_h = max(dot(N, H), 0.0);
    float n_dot_h_2 = n_dot_h*n_dot_h;

    float nom   = a2;
    float denom = (n_dot_h_2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;

    return nom / denom;
}
// ----------------------------------------------------------------------------
float GeometrySchlickGGX(float n_dot_v, float roughness)
{
    float r = (roughness + 1.0);
    float k = (r*r) / 8.0;

    float nom   = n_dot_v;
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

float3 ApplyLight(
       float3 n,
       float3 h,
       float3 v,
       float3 l,
       float3 f0,
       float3 albedo,
       float3 radiance,
       float roughness,
       float metallic)
{
    float ndf = DistributionGGX(n, h, roughness);
    float g = GeometrySmith(n, v, l, roughness);
    float3 f = FresnelSchlick(max(dot(h, v), 0.0), f0);

    float3 num = ndf * g * f;
    float denom = 4.0 * max(dot(n, v), 0.0) * max(dot(n, l), 0.0) + 0.0001;
    float3 specular = num / denom;

    float3 ks = f;
    float3 kd = float3(1,1,1) - ks;
    kd *= 1.0 - metallic;

    float n_dot_l = max(dot(n, l), 0.0);
    return (kd * albedo / PI + specular) * radiance * n_dot_l;
}