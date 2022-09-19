struct PixelShaderInput
{
    float4 Position     : SV_Position;
    float3 TangentVS    : TANGENT;
    float3 BinormalVS   : BINORMAL;
    float3 NormalVS     : NORMAL;
    float3 PositionVS   : POSITION;
    float2 TexCoord     : TEXCOORD;
};
 
Texture2D<float4> Texture : register(t0);

SamplerState texureSampler : register(s0);

float4 main(PixelShaderInput IN) : SV_Target
{
    return Texture.Sample(texureSampler, IN.TexCoord);
}