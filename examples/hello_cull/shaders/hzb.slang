#define FLT_MAX 3.402823466e+38F

struct PushConstant
{
    uint src_mip_level;
    uint src_dimension;
    float2 texel_size;
    uint src_index;
    uint dst_index;
}
ConstantBuffer<PushConstant> PushConstants : register(b0);

SamplerState pointSampler : register(s0);

[shader("compute")]
void compute_main(uint3 dtid: SV_DispatchThreadID, uint3 gtid: SV_GroupThreadID, uint gid: SV_GroupID)
{
    float src = -FLT_MAX;

    float2 uv = PushConstants.texel_size * (dtid.xy + float2(0.25, 0.25));
    float2 off = PushConstants.texel_size * 0.5;

    float2 offset1 = float2(0.0, 0.0);
    float2 offset2 = float2(off.x, 0.0);
    float2 offset3 = float2(0.0, off.y);
    float2 offset4 = float2(off.x, off.y);

    var src_mip = DescriptorHandle<Texture2D<float>>(PushConstants.src_index);
    var dst_mip = DescriptorHandle<RWTexture2D<float>>(PushConstants.dst_index);

    src = max(max(max(src_mip.SampleLevel(pointSampler, uv + offset1, PushConstants.src_mip_level),
                      max(src_mip.SampleLevel(pointSampler, uv + offset2, PushConstants.src_mip_level),
                          max(src_mip.SampleLevel(pointSampler, uv + offset3, PushConstants.src_mip_level),
                              src_mip.SampleLevel(pointSampler, uv + offset4, PushConstants.src_mip_level)))),
                  max(src_mip.SampleLevel(pointSampler, uv + offset1 + float2(off.x, 0.0), PushConstants.src_mip_level),
                      max(src_mip.SampleLevel(pointSampler, uv + offset2 + float2(off.x, 0.0), PushConstants.src_mip_level),
                          max(src_mip.SampleLevel(pointSampler, uv + offset3 + float2(off.x, 0.0), PushConstants.src_mip_level),
                              src_mip.SampleLevel(pointSampler, uv + offset4 + float2(off.x, 0.0), PushConstants.src_mip_level))))),

              max(max(src_mip.SampleLevel(pointSampler, uv + offset1 + float2(0.0, off.y), PushConstants.src_mip_level),
                      max(src_mip.SampleLevel(pointSampler, uv + offset2 + float2(0.0, off.y), PushConstants.src_mip_level),
                          max(src_mip.SampleLevel(pointSampler, uv + offset3 + float2(0.0, off.y), PushConstants.src_mip_level),
                              src_mip.SampleLevel(pointSampler, uv + offset4 + float2(0.0, off.y), PushConstants.src_mip_level)))),
                  max(src_mip.SampleLevel(pointSampler, uv + offset1 + float2(off.x, off.y), PushConstants.src_mip_level),
                      max(src_mip.SampleLevel(pointSampler, uv + offset2 + float2(off.x, off.y), PushConstants.src_mip_level),
                          max(src_mip.SampleLevel(pointSampler, uv + offset3 + float2(off.x, off.y), PushConstants.src_mip_level),
                              src_mip.SampleLevel(pointSampler, uv + offset4 + float2(off.x, off.y), PushConstants.src_mip_level))))));

    dst_mip[dtid.xy] = src;
}
