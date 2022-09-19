struct Transform
{
    matrix MVP;
    matrix MV;
};
 
ConstantBuffer<Transform> TransformCB : register(b0);
 
struct VertexInput
{
    float3 Position : POSITION;
    float3 Tangent  : TANGENT;
    float3 Binormal : BINORMAL;
    float3 Normal   : NORMAL;
    float2 TexCoord : TEXCOORD;
};
 
struct VertexShaderOutput
{
    float4 Position     : SV_Position;
    float3 TangentVS    : TANGENT;
    float3 BinormalVS   : BINORMAL;
    float3 NormalVS     : NORMAL;
    float3 PositionVS   : POSITION;
    float2 TexCoord     : TEXCOORD;
};
 
VertexShaderOutput main(VertexInput IN)
{
    VertexShaderOutput OUT;
 
    OUT.Position = mul(TransformCB.MVP, float4(IN.Position, 1.0f));
    OUT.PositionVS = mul(TransformCB.MV, float4(IN.Position, 1.0f)).xyz;
    OUT.TangentVS = mul((float3x3)TransformCB.MV, IN.Tangent);
    OUT.BinormalVS = mul((float3x3)TransformCB.MV, IN.Binormal);
    OUT.NormalVS = mul((float3x3)TransformCB.MV, IN.Normal);
    OUT.TexCoord = IN.TexCoord;
 
    return OUT;
}