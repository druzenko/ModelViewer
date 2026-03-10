#pragma once

#include <DirectXMath.h>
#include <assimp/material.h>

class Texture;

#define MATERIAL_TEXTURES_COUNT aiTextureType_REFLECTION

using MaterialID = UINT32;

struct MaterialParams
{
    //DirectX::XMFLOAT4  GlobalAmbient;
    DirectX::XMFLOAT4  AmbientColor = DirectX::XMFLOAT4();
    DirectX::XMFLOAT4  EmissiveColor = DirectX::XMFLOAT4();
    DirectX::XMFLOAT4  DiffuseColor = DirectX::XMFLOAT4();
    DirectX::XMFLOAT4  SpecularColor = DirectX::XMFLOAT4();
    DirectX::XMFLOAT4  Reflectance = DirectX::XMFLOAT4();
    float   Opacity = 0.0f;
    float   SpecularPower = 0.0f;
    float   IndexOfRefraction = 0.0f;
    UINT32    HasAmbientTexture = 0;
    UINT32    HasEmissiveTexture = 0;
    UINT32    HasDiffuseTexture = 0;
    UINT32    HasSpecularTexture = 0;
    UINT32    HasSpecularPowerTexture = 0;
    UINT32    HasNormalTexture = 0;
    UINT32    HasBumpTexture = 0;
    UINT32    HasOpacityTexture = 0;
    float   BumpIntensity = 0.0f;
    float   SpecularScale = 0.0f;
    float   AlphaThreshold = 0.0f;
    DirectX::XMFLOAT2 padding;
};

struct Material
{
    std::vector<Texture*> mTextures;
#ifdef _DEBUG
    std::string mName;
#endif // _DEBUG
};

namespace Materials
{
    void AddMaterial(MaterialParams&& aParams, std::vector<Texture*>&& aTextures, const char* aName);
    unsigned int GetMaterialCount();
    const char* GetMaterialName(MaterialID materialID);
    void CreateMaterialTexturesSRV();
    const std::vector<MaterialParams>& GetMaterialParams();
}