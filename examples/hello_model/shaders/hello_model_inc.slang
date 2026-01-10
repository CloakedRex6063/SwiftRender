struct Vertex
{
    float3 position;
    float uv_x;
    float3 normal;
    float uv_y;
    float4 tangent;
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
    float3 padding;
};
ConstantBuffer<GlobalConstant> GlobalConstants : register(b1);

static const float PI = 3.14159265359;