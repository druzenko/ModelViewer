#pragma once

#include <DirectXMath.h>

struct Material
{
    DirectX::XMFLOAT4  GlobalAmbient;
    DirectX::XMFLOAT4  AmbientColor;
    DirectX::XMFLOAT4  EmissiveColor;
    DirectX::XMFLOAT4  DiffuseColor;
    DirectX::XMFLOAT4  SpecularColor;
    DirectX::XMFLOAT4  Reflectance;
    float   Opacity;
    float   SpecularPower;
    float   IndexOfRefraction;
    UINT32    HasAmbientTexture;
    UINT32    HasEmissiveTexture;
    UINT32    HasDiffuseTexture;
    UINT32    HasSpecularTexture;
    UINT32    HasSpecularPowerTexture;
    UINT32    HasNormalTexture;
    UINT32    HasBumpTexture;
    UINT32    HasOpacityTexture;
    float   BumpIntensity;
    float   SpecularScale;
    float   AlphaThreshold;
};
