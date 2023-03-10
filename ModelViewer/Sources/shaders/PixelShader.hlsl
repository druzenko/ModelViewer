struct PixelShaderInput
{
    float4 Position     : SV_Position;
    float3 TangentVS    : TANGENT;
    float3 BitangentVS  : BITANGENT;
    float3 NormalVS     : NORMAL;
    float3 PositionVS   : POSITION;
    float2 TexCoord     : TEXCOORD;
};

struct PSRootConstants
{
    uint LightsCount;
};

struct MaterialParams
{
    //float4  GlobalAmbient;
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
    float2  Padding;
    //--------------------------- ( 16 bytes )
};  //--------------------------- ( 16 * 10 = 160 bytes )

#define MAX_CB_SIZE 65536
#define MATERIAL_SIZE 144
#define MAX_MATERIALS_COUNT_IN_CB 455

struct Materials
{
    MaterialParams materialParams[MAX_MATERIALS_COUNT_IN_CB];
};

struct Light
{
    float3 PositionWS;
    //--------------------------------------------------------------( 12 bytes )
    float3 DirectionWS;
    //--------------------------------------------------------------( 12 bytes )
    float3 PositionVS;
    //--------------------------------------------------------------( 12 bytes )
    float3 DirectionVS;
    //--------------------------------------------------------------( 12 bytes )
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

struct LightingResult
{
    float4 Diffuse;
    float4 Specular;
};

//ConstantBuffer<Material> MaterialCB : register(b0);
ConstantBuffer<Materials> MateriaslCB : register(b1);

struct MaterialID
{
    uint ID;
};

ConstantBuffer<MaterialID> MaterialIDCB : register(b2);

ConstantBuffer<PSRootConstants> PSRootConstantsCB : register(b3);

StructuredBuffer<Light> Lights : register(t21);

#define POINT_LIGHT 0
#define SPOT_LIGHT 1
#define DIRECTIONAL_LIGHT 2
 
//Texture2D<float4> AmbientTexture        : register(t0);
//Texture2D<float4> EmissiveTexture       : register(t1);
//Texture2D<float4> DiffuseTexture        : register(t2);
//Texture2D<float4> SpecularTexture       : register(t3);
//Texture2D<float4> SpecularPowerTexture  : register(t4);
//Texture2D<float4> NormalTexture         : register(t5);
//Texture2D<float4> BumpTexture           : register(t6);
//Texture2D<float4> OpacityTexture        : register(t7);

//Texture2D<float4> Texture1[18]        : register(t0);
Texture2D<float4> DiffuseTexture        : register(t10);
Texture2D<float4> SpecularTexture       : register(t11);
Texture2D<float4> AmbientTexture        : register(t12);
Texture2D<float4> EmissiveTexture       : register(t13);
Texture2D<float4> BumpTexture           : register(t14);
Texture2D<float4> NormalTexture         : register(t15);
Texture2D<float4> SpecularPowerTexture  : register(t16);
Texture2D<float4> OpacityTexture        : register(t17);
Texture2D<float4> Texture8              : register(t18);
Texture2D<float4> Texture9              : register(t19);
Texture2D<float4> Texture10             : register(t20);

SamplerState textureSampler : register(s0);

float4 DoDiffuse(Light light, float4 L, float4 N)
{
    float NdotL = max(dot(N, L), 0);
    return light.Color * NdotL;
}

float4 DoSpecular(Light light, MaterialParams material, float4 V, float4 L, float4 N)
{
    float4 R = normalize(reflect(-L, N));
    float RdotV = max(dot(R, V), 0);

    return light.Color * pow(RdotV, material.SpecularPower);
}

float DoAttenuation(Light light, float distance)
{
    return 1.0 - smoothstep(0.75 * light.Range, light.Range, distance);
}

LightingResult DoDirectionalLight(Light light, MaterialParams material, float4 V, float4 P, float4 N)
{
    LightingResult result;

    float4 L = float4(-light.DirectionVS, 0.0);

    result.Diffuse = DoDiffuse(light, L, N) * light.Intensity;
    result.Specular = DoSpecular(light, material, V, L, N) * light.Intensity;

    return result;
}

LightingResult DoPointLight(Light light, MaterialParams material, float4 V, float4 P, float4 N)
{
    LightingResult result;
    
    float4 L = float4(light.PositionVS, 1.0) - P;
    float distance = length(L);
    L /= distance;
    
    float attenuation = DoAttenuation(light, distance);
    
    result.Diffuse = DoDiffuse(light, L, N) * light.Intensity * attenuation;
    result.Specular = DoSpecular(light, material, V, L, N) * light.Intensity * attenuation;
    
    return result;
}

LightingResult DoLighting(StructuredBuffer<Light> lights, MaterialParams material, float4 eyePos, float4 P, float4 N)
{
    float4 V = normalize(eyePos - P);

    LightingResult totalResult = (LightingResult)0;

    for (uint i = 0; i < PSRootConstantsCB.LightsCount; ++i)
    {
        LightingResult result = (LightingResult)0;

        if (!lights[i].Enabled)
        {
            continue;
        }
        
        if (lights[i].Type != DIRECTIONAL_LIGHT && length(lights[i].PositionVS - P) > lights[i].Range)
        {
            continue;
        }

        switch (lights[i].Type)
        {
        case DIRECTIONAL_LIGHT:
        {
            result = DoDirectionalLight(lights[i], material, V, P, N);
        }
        break;
        case POINT_LIGHT:
        {
            result = DoPointLight(lights[i], material, V, P, N);
        }
        break;
        case SPOT_LIGHT:
        {
            //result = DoSpotLight(lights[i], material, V, P, N);
        }
        break;
        }

        totalResult.Diffuse += result.Diffuse;
        totalResult.Specular += result.Specular;
    }

    return totalResult;
}

float3 ExpandNormal(float3 n)
{
    return n * 2.0 - 1.0;
}

float4 DoNormalMapping(float3x3 TBN, Texture2D tex, sampler s, float2 uv)
{
    float3 normal = tex.Sample(s, uv).xyz;
    normal = ExpandNormal(normal);
    normal = mul(normal, TBN);
    return normalize(float4(normal, 0.0));
}

float4 DoBumpMapping(float3x3 TBN, Texture2D tex, sampler s, float2 uv, float bumpScale)
{
    // Sample the heightmap at the current texture coordinate.
    float height = tex.Sample(s, uv).r * bumpScale;
    // Sample the heightmap in the U texture coordinate direction.
    float heightU = tex.Sample(s, uv, int2(1, 0)).r * bumpScale;
    // Sample the heightmap in the V texture coordinate direction.
    float heightV = tex.Sample(s, uv, int2(0, 1)).r * bumpScale;

    float3 p = { 0, 0, height };
    float3 pU = { 1, 0, heightU };
    float3 pV = { 0, 1, heightV };

    // normal = tangent x bitangent
    float3 normal = cross(normalize(pU - p), normalize(pV - p));

    // Transform normal from tangent space to view space.
    normal = mul(normal, TBN);

    return float4(normal, 0);
}

[earlydepthstencil]
float4 main(PixelShaderInput IN) : SV_Target
{
    float4 cameraPos = { 0, 0, 0, 1 };
    MaterialParams material = MateriaslCB.materialParams[MaterialIDCB.ID];

    float4 diffuse = material.DiffuseColor;
    if (material.HasDiffuseTexture)
    {
        float4 diffuseTex = DiffuseTexture.Sample(textureSampler, IN.TexCoord);
        if (any(diffuse.rgb))
        {
            diffuse *= diffuseTex;
        }
        else
        {
            diffuse = diffuseTex;
        }
    }

    float alpha = diffuse.a;
    if (material.HasOpacityTexture)
    {
        alpha = OpacityTexture.Sample(textureSampler, IN.TexCoord).r;
    }

    float4 ambient = material.AmbientColor;
    if (material.HasAmbientTexture)
    {
        float4 ambientTex = AmbientTexture.Sample(textureSampler, IN.TexCoord);
        if (any(ambient.rgb))
        {
            ambient *= ambientTex;
        }
        else
        {
            ambient = ambientTex;
        }
    }

    float4 emissive = material.EmissiveColor;
    if (material.HasEmissiveTexture)
    {
        float4 emissiveTex = EmissiveTexture.Sample(textureSampler, IN.TexCoord);
        if (any(emissive.rgb))
        {
            emissive *= emissiveTex;
        }
        else
        {
            emissive = emissiveTex;
        }
    }

    float specularPower = material.SpecularPower;
    if (material.HasSpecularPowerTexture)
    {
        specularPower = SpecularPowerTexture.Sample(textureSampler, IN.TexCoord).r * material.SpecularScale;
    }

    float4 normal;
    if (material.HasNormalTexture)
    {
        float3x3 TBN = float3x3(normalize(IN.TangentVS),
                                normalize(IN.BitangentVS),
                                normalize(IN.NormalVS));

        normal = DoNormalMapping(TBN, NormalTexture, textureSampler, IN.TexCoord);
    }
    else if (material.HasBumpTexture)
    {
        float3x3 TBN = float3x3(normalize(IN.TangentVS),
                                normalize(-IN.BitangentVS),
                                normalize(IN.NormalVS));

        normal = DoBumpMapping(TBN, BumpTexture, textureSampler, IN.TexCoord, material.BumpIntensity);
    }
    else
    {
        normal = normalize(float4(IN.NormalVS, 0.0));
    }

    float4 P = float4(IN.PositionVS, 1);

    LightingResult result = DoLighting(Lights, material, cameraPos, P, normal);

    diffuse *= float4(result.Diffuse.rgb, 1.0f); // Discard the alpha value from the lighting calculations.

    float4 specular = 0;
    if (specularPower > 1.0f) // If specular power is too low, don't use it.
    {
        specular = material.SpecularColor;
        if (material.HasSpecularTexture)
        {
            float4 specularTex = SpecularTexture.Sample(textureSampler, IN.TexCoord);
            if (any(specular.rgb))
            {
                specular *= specularTex;
            }
            else
            {
                specular = specularTex;
            }
        }
        specular *= result.Specular;
    }

    return float4((ambient + emissive + diffuse + specular).rgb, alpha * material.Opacity);
}