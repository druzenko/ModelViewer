struct PixelShaderInput
{
    float4 Position     : SV_Position;
    float3 TangentVS    : TANGENT;
    float3 BinormalVS   : BINORMAL;
    float3 NormalVS     : NORMAL;
    float3 PositionVS   : POSITION;
    float2 TexCoord     : TEXCOORD;
};

struct Material
{
    float4  GlobalAmbient;
    //-------------------------- ( 16 bytes )
    float4  AmbientColor;
    //-------------------------- ( 16 bytes )
    float4  EmissiveColor;
    //-------------------------- ( 16 bytes )
    float4  DiffuseColor;
    //-------------------------- ( 16 bytes )
    float4  SpecularColor;
    //-------------------------- ( 16 bytes )
    // Reflective value.
    float4  Reflectance;
    //-------------------------- ( 16 bytes )
    float   Opacity;
    float   SpecularPower;
    // For transparent materials, IOR > 0.
    float   IndexOfRefraction;
    bool    HasAmbientTexture;
    //-------------------------- ( 16 bytes )
    bool    HasEmissiveTexture;
    bool    HasDiffuseTexture;
    bool    HasSpecularTexture;
    bool    HasSpecularPowerTexture;
    //-------------------------- ( 16 bytes )
    bool    HasNormalTexture;
    bool    HasBumpTexture;
    bool    HasOpacityTexture;
    float   BumpIntensity;
    //-------------------------- ( 16 bytes )
    float   SpecularScale;
    float   AlphaThreshold;
    //float2  Padding;
    //--------------------------- ( 16 bytes )
};  //--------------------------- ( 16 * 10 = 160 bytes )

struct Light
{
    float4 PositionWS;
    //--------------------------------------------------------------( 16 bytes )
    float4 DirectionWS;
    //--------------------------------------------------------------( 16 bytes )
    float4 PositionVS;
    //--------------------------------------------------------------( 16 bytes )
    float4 DirectionVS;
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

ConstantBuffer<Material> MaterialCB : register(b0);

StructuredBuffer<Light> Lights;
 
//Texture2D<float4> AmbientTexture        : register(t0);
//Texture2D<float4> EmissiveTexture       : register(t1);
//Texture2D<float4> DiffuseTexture        : register(t2);
//Texture2D<float4> SpecularTexture       : register(t3);
//Texture2D<float4> SpecularPowerTexture  : register(t4);
//Texture2D<float4> NormalTexture         : register(t5);
//Texture2D<float4> BumpTexture           : register(t6);
//Texture2D<float4> OpacityTexture        : register(t7);

//Texture2D<float4> Texture1[18]        : register(t0);
Texture2D<float4> Texture1        : register(t0);
Texture2D<float4> Texture2        : register(t1);
Texture2D<float4> Texture3        : register(t2);
Texture2D<float4> AmbientTexture    : register(t3);
Texture2D<float4> Texture5        : register(t4);
Texture2D<float4> Texture6        : register(t5);
Texture2D<float4> Texture7        : register(t6);
Texture2D<float4> Texture8        : register(t7);
Texture2D<float4> Texture9        : register(t8);
Texture2D<float4> Texture10        : register(t9);
Texture2D<float4> Texture11        : register(t10);
Texture2D<float4> Texture12        : register(t11);
Texture2D<float4> Texture13        : register(t12);
Texture2D<float4> Texture14        : register(t13);
Texture2D<float4> Texture15        : register(t14);
Texture2D<float4> Texture16        : register(t15);
Texture2D<float4> Texture17        : register(t16);
Texture2D<float4> Texture18        : register(t17);

SamplerState texureSampler : register(s0);

float4 main(PixelShaderInput IN) : SV_Target
{
    return AmbientTexture.Sample(texureSampler, IN.TexCoord);
}