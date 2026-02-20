struct PushConstant
{
    uint sampler_index;
    uint mip1_index;
    uint src_index;
    float2 texel_size;
};
ConstantBuffer<PushConstant> PushConstants : register(b0);

[numthreads( 8, 8, 1 )]
[shader("compute")]
void main(uint3 dtid : SV_DispatchThreadID)
{
    var dest_mip = DescriptorHandle<RWTexture2D<float4>>(PushConstants.mip1_index);
    var src_mip = DescriptorHandle<Sampler2D>(uint2(PushConstants.src_index, PushConstants.sampler_index));

    float2 tex_coords = PushConstants.texel_size * (dtid.xy + 0.5);

    float4 color = src_mip.SampleLevel(tex_coords, 0);
    dest_mip[dtid.xy] = color;
}