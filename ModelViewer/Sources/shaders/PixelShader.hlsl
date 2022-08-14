struct PixelShaderInput
{
    float4 Position : SV_Position;
    float3 Normal : NORMAL;
    float2 TexCoord : TEXCOORD;
};
 
Texture2D<float4> Texture : register(t0);

SamplerState texureSampler : register(s0);

float4 main(PixelShaderInput IN) : SV_Target
{
    return Texture.Sample(texureSampler, IN.TexCoord);
}