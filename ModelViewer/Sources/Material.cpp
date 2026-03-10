#include "pch.h"
#include "Material.h"
#include "Texture.h"

namespace Materials
{
    static std::vector<Material> sMaterialRegister;
    static std::vector<MaterialParams> sMaterialParamsRegister;

    void AddMaterial(MaterialParams&& aParams, std::vector<Texture*>&& aTextures, const char* aName)
    {
        sMaterialParamsRegister.push_back(aParams);
        sMaterialRegister.push_back({ aTextures });
#ifdef _DEBUG
        sMaterialRegister.rbegin()->mName = aName;
#endif // _DEBUG

    }

    unsigned int GetMaterialCount()
    {
        return sMaterialRegister.size();
    }

    const char* GetMaterialName(MaterialID materialID)
    {
#ifdef _DEBUG
        return sMaterialRegister[materialID].mName.c_str();
#else
        return nullptr;
#endif // _DEBUG
    }

    void CreateMaterialTexturesSRV()
    {
        for (unsigned int materialID = 0; materialID < sMaterialRegister.size(); ++materialID)
        {
            Material& materialResources = sMaterialRegister[materialID];
            for (unsigned int i = 0; i < materialResources.mTextures.size(); ++i)
            {
                if (materialResources.mTextures[i])
                {
                    materialResources.mTextures[i]->CreateSRV(Graphics::g_SRVDescriptorHeap, materialID * MATERIAL_TEXTURES_COUNT + i);
                }
                else
                {
                    Texture::CreateEmptySRV(Graphics::g_SRVDescriptorHeap, materialID * MATERIAL_TEXTURES_COUNT + i);
                }
            }
        }
    }

    const std::vector<MaterialParams>& GetMaterialParams()
    {
        return sMaterialParamsRegister;
    }
}