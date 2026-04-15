#define NUM_THREADS 64

struct ComputeShaderInput
{
    uint3 GroupID : SV_GroupID; // 3D index of the thread group in the dispatch.
    uint3 GroupThreadID : SV_GroupThreadID; // 3D index of local thread ID in a thread group.
    uint3 DispatchThreadID : SV_DispatchThreadID; // 3D index of global thread ID in the dispatch.
    uint GroupIndex : SV_GroupIndex; // Flattened local index of the thread within a thread group.
};

struct CSLightRootConstants
{
    matrix MV;
    uint LightsCount;
};

struct Light
{
    float3 PositionWS;
    //--------------------------------------------------------------( 16 bytes )
    float3 DirectionWS;
    //--------------------------------------------------------------( 16 bytes )
    float3 PositionVS;
    //--------------------------------------------------------------( 16 bytes )
    float3 DirectionVS;
    //--------------------------------------------------------------( 16 bytes )
    float4 Color;
    //--------------------------------------------------------------( 16 bytes )
    float SpotlightAngle;
    float Range;
    float Intensity;
    bool Enabled;
    //--------------------------------------------------------------( 16 bytes )
    bool Selected;
    uint Type;
    //float2 Padding;
    //--------------------------------------------------------------( 16 bytes )
    //--------------------------------------------------------------( 16 * 7 = 112 bytes )
};

ConstantBuffer<CSLightRootConstants> CSLightRootConstantsCB : register(b0);

RWStructuredBuffer<Light> Lights : register(u0);

[numthreads(NUM_THREADS, 1, 1)]
void main(ComputeShaderInput IN)
{
    uint lightID = IN.GroupID.x * NUM_THREADS + IN.GroupIndex;
    if (lightID < CSLightRootConstantsCB.LightsCount)
    {
        Lights[lightID].PositionVS = mul(CSLightRootConstantsCB.MV, float4(Lights[lightID].PositionWS, 1.0)).xyz;
        Lights[lightID].DirectionVS = mul((float3x3)CSLightRootConstantsCB.MV, Lights[lightID].DirectionWS);
        Lights[lightID].DirectionVS = normalize(Lights[lightID].DirectionVS);
    }
}