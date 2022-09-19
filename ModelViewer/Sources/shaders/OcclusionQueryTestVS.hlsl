struct Transform
{
    matrix MVP;
};
 
ConstantBuffer<Transform> TransformCB : register(b0);
 
struct VertexInput
{
    float3 Position : POSITION;
    float4 Color : COLOR;
};
 
struct VertexShaderOutput
{
    float4 Position     : SV_Position;
    float4 Color : COLOR;
};
 
VertexShaderOutput main(VertexInput IN)
{
    VertexShaderOutput OUT;
    OUT.Position = mul(TransformCB.MVP, float4(IN.Position, 1.0f));
    OUT.Color = IN.Color;
    return OUT;
}