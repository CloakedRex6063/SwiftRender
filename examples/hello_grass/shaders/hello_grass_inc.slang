struct Vertex
{
    float4 position : SV_Position;
    float3 normal : NORMAL;
    float2 uv : TEXCOORD;
    float3 world_pos : POSITION;
    float density : DENSITY;
};

struct GrassPatch
{
    float3 position;
    float height;
};

struct GlobalConstant
{
    float4x4 view_proj;

    float3 cam_pos;
    uint frustum_buffer_index;

    uint grass_buffer_index;
    uint grass_patch_count;
    float2 padding;
};
ConstantBuffer<GlobalConstant> GlobalConstants : register(b1);

struct PushConstant
{
    float wind_speed;
    float wind_strength;
    uint apply_view_space_thicken;
    float lod_distance;

    uint grass_count;
    float radius;
    uint cull;
    uint cull_z;

    float width;
    float time;
    float2 padding;
};

ConstantBuffer<PushConstant> PushConstants : register(b0);

static const uint GROUP_SIZE = 128;
static const int GRASS_VERT_COUNT = 256;
static const int GRASS_PRIM_COUNT = 192;
static const float PI = 3.14159265359;

float3 Bezier(float3 p0, float3 p1, float3 p2, float t)
{
    float3 a = lerp(p0, p1, t);
    float3 b = lerp(p1, p2, t);
    return lerp(a, b, t);
}

float3 BezierDerivative(float3 p0, float3 p1, float3 p2, float t)
{
    return 2. * (1. - t) * (p1 - p0) + 2. * t * (p2 - p1);
}

float Hash(uint seed)
{
    seed = (seed ^ 61u) ^ (seed >> 16u);
    seed *= 9u;
    seed = seed ^ (seed >> 4u);
    seed *= 0x27d4eb2du;
    seed = seed ^ (seed >> 15u);
    return float(seed) / 4294967296.0;
}

float3 Hash3(uint seed)
{
    uint h1 = seed * 73856093u;
    uint h2 = seed * 19349663u;
    uint h3 = seed * 83492791u;

    h1 = (h1 ^ 61u) ^ (h1 >> 16u);
    h2 = (h2 ^ 61u) ^ (h2 >> 16u);
    h3 = (h3 ^ 61u) ^ (h3 >> 16u);

    h1 *= 9u;
    h2 *= 9u;
    h3 *= 9u;

    return float3(h1, h2, h3) * (1.0 / 4294967296.0);
}

float Hash(float3 p)
{
    uint3 q = uint3(int3(p * 1000.0));
    uint n = q.x * 374761393u + q.y * 668265263u + q.z * 1274126177u;
    return Hash(n);
}

float2 RandomPointInCircle(uint seed, float radius)
{
    float random1 = Hash(seed);
    float random2 = Hash(seed + 1u);

    float r = sqrt(random1) * radius;
    float theta = random2 * 2.0 * PI;

    return float2(r * cos(theta), r * sin(theta));
}

float2 RandomPointInCircle(float3 position, float radius)
{
    uint seed = uint(Hash(position));
    return RandomPointInCircle(seed, radius);
}

float3 GetPatchAmbientMotion(float3 center, float time)
{
    float3 offset = float3(0, 0, 0);
    offset.x = (2 * sin(center.x + center.y + center.z + time)) + 1;
    offset.z = sin(2 * (center.x + center.y + center.z + time)) + 0.5;
    return offset;
}

float EaseIn(float t, int power) {
    return pow(t, float(power));
}

float EaseOut(float x, float power)
{
    return 1.0 - pow(1.0 - x, power);
}

float Noise2D(float2 p)
{
    float2 i = floor(p);
    float2 f = frac(p);

    f = f * f * (3.0 - 2.0 * f);

    float a = Hash(uint(i.x) + uint(i.y) * 57u);
    float b = Hash(uint(i.x + 1.0) + uint(i.y) * 57u);
    float c = Hash(uint(i.x) + uint(i.y + 1.0) * 57u);
    float d = Hash(uint(i.x + 1.0) + uint(i.y + 1.0) * 57u);

    return lerp(lerp(a, b, f.x), lerp(c, d, f.x), f.y);
}

float Remap(float value, float from_min, float from_max, float to_min, float to_max)
{
    return to_min + (value - from_min) * (to_max - to_min) / (from_max - from_min);
}

float3 ApplyWind(float3 position)
{
    float wind_dir = Noise2D(position.xz * 0.05 + 0.05 * PushConstants.time);
    wind_dir = Remap(wind_dir, 0.0, 1.0, 0.0, 2.0 * PI);
    float wind_noise_sample = Noise2D(position.xz * 0.25 + PushConstants.time * PushConstants.wind_speed);
    float wind_lean_angle = EaseIn(wind_noise_sample, 2) * 1.0;
    float wind_lean_amount = EaseIn(sin(wind_lean_angle), 2) * 1.25 * PushConstants.wind_strength;
    float3 wind_direction = float3(cos(wind_dir), 0, sin(wind_dir));
    return wind_direction * wind_lean_amount;
}

void MakePersistentLength(in float3 v0, inout float3 v1, inout float3 v2, in float height)
{
    float3 v01 = v1 - v0;
    float3 v12 = v2 - v1;
    float lv01 = length(v01);
    float lv12 = length(v12);

    float L1 = lv01 + lv12;
    float L0 = length(v2-v0);
    float L = (2.0f * L0 + L1) / 3.0f;

    float ldiff = height / L;
    v01 = v01 * ldiff;
    v12 = v12 * ldiff;
    v1 = v0 + v01;
    v2 = v1 + v12;
}

struct Payload
{
    uint patch_id[128];
    float blade_count_f[128];
};

struct Plane
{
    float3 normal;
    float distance;
};

struct Frustum
{
    Plane topFace;
    Plane bottomFace;
    Plane leftFace;
    Plane rightFace;
    Plane nearFace;
    Plane farFace;
};

struct BoundingSphere
{
    float3 center;
    float radius;
};

bool IsInsidePlane(Plane plane, BoundingSphere sphere)
{
    return (dot(plane.normal, sphere.center) - plane.distance) >= -sphere.radius;
}

bool IsInFrustum(Frustum frustum, float radius, float3 position, bool cull_z)
{
    const BoundingSphere sphere = {position, radius};

    if (!IsInsidePlane(frustum.leftFace, sphere)) return false;
    if (!IsInsidePlane(frustum.rightFace, sphere)) return false;
    if (!IsInsidePlane(frustum.topFace, sphere)) return false;
    if (!IsInsidePlane(frustum.bottomFace, sphere)) return false;

    if(cull_z)
    {
        if (!IsInsidePlane(frustum.nearFace, sphere)) return false;
        if (!IsInsidePlane(frustum.farFace, sphere)) return false;
    }

    return true;
}