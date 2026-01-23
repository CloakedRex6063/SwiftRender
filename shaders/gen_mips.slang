struct PushConstant
{
    uint mip1_index;
    uint src_index;
    float2 texel_size;
};
ConstantBuffer<PushConstant> PushConstants : register(b0);

SamplerState BilinearClamp : register(s0);

[numthreads( 8, 8, 1 )]
[shader("compute")]
void main(uint3 dtid : SV_DispatchThreadID)
{
    var dest_mip = DescriptorHandle<RWTexture2D<float4>>(PushConstants.mip1_index);
    var src_mip = DescriptorHandle<Texture2D<float4>>(PushConstants.src_index);

    float2 tex_coords = PushConstants.texel_size * (dtid.xy + 0.5);

    float4 color = src_mip.SampleLevel(BilinearClamp, tex_coords, 0);
    dest_mip[dtid.xy] = color;
}