#include "pch.h"
#include "Model.h"
#include "Utility.h"
#include "PixEvents.h"
#include "Material.h"

Model::Model(const std::string& aPath)
{
	std::string directory = aPath.substr(0, aPath.find_last_of('/') + 1);
	mDirectory = std::wstring(directory.begin(), directory.end());
	LoadModel(aPath);
}

void Model::LoadModel(const std::string& aPath)
{
	Assimp::Importer importer;
	const aiScene* scene = importer.ReadFile(aPath.data(), aiProcess_Triangulate | aiProcess_ConvertToLeftHanded | aiProcess_GenSmoothNormals | aiProcess_CalcTangentSpace);

	ASSERT(scene != nullptr && (scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE) == 0 && scene->mRootNode != nullptr, "Model is not loaded.");

#ifdef _DEBUG
	mName = scene->mRootNode->mName.C_Str();
#endif // _DEBUG

	ProcessMaterials(scene);
	ProcessNode(scene->mRootNode, scene);
}

void Model::ProcessNode(const aiNode* aNode, const aiScene* aScene)
{
	for (unsigned int i = 0; i < aNode->mNumMeshes; ++i)
	{
		aiMesh* mesh = aScene->mMeshes[aNode->mMeshes[i]];
		mMeshes.push_back(ProcessMesh(mesh, aScene));
	}
	
	for (unsigned int i = 0; i < aNode->mNumChildren; i++)
	{
		ProcessNode(aNode->mChildren[i], aScene);
	}
}

Mesh Model::ProcessMesh(const aiMesh* aMesh, const aiScene* aScene)
{
	std::vector<Vertex> vertices(aMesh->mNumVertices);
	std::vector<unsigned int> indices;
	std::vector<Texture*> textures(MATERIAL_TEXTURES_COUNT);

	for (unsigned int i = 0; i < aMesh->mNumVertices; ++i)
	{
		Vertex& vertex = vertices[i];

		vertex.position.x = aMesh->mVertices[i].x;
		vertex.position.y = aMesh->mVertices[i].y;
		vertex.position.z = aMesh->mVertices[i].z;

		vertex.normal.x = aMesh->mNormals[i].x;
		vertex.normal.y = aMesh->mNormals[i].y;
		vertex.normal.z = aMesh->mNormals[i].z;

		vertex.tangent.x = aMesh->mTangents[i].x;
		vertex.tangent.y = aMesh->mTangents[i].y;
		vertex.tangent.z = aMesh->mTangents[i].z;

		vertex.bitangent.x = aMesh->mBitangents[i].x;
		vertex.bitangent.y = aMesh->mBitangents[i].y;
		vertex.bitangent.z = aMesh->mBitangents[i].z;

		if (aMesh->mTextureCoords[0])
		{
			vertex.texCoord.x = aMesh->mTextureCoords[0][i].x;
			vertex.texCoord.y = aMesh->mTextureCoords[0][i].y;
		}
		else
		{
			vertex.texCoord = DirectX::XMFLOAT2(0.0f, 0.0f);
		}
	}

	indices.reserve(aMesh->mNumFaces);
	for (unsigned int i = 0; i < aMesh->mNumFaces; ++i)
	{
		const aiFace& face = aMesh->mFaces[i];
		for (unsigned int j = 0; j < face.mNumIndices; ++j)
		{
			indices.push_back(face.mIndices[j]);
		}
	}

	return Mesh(Buffer(L"Vertices", vertices.size(), sizeof(Vertex), vertices.data(), D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER),
				Buffer(L"Indices", indices.size(), sizeof(unsigned int), indices.data(), D3D12_RESOURCE_STATE_INDEX_BUFFER),
				aMesh->mMaterialIndex, aMesh->mName.C_Str()
				);
}

void Model::ProcessMaterials(const aiScene* aScene)
{
	for (unsigned int materialID = 0; materialID < aScene->mNumMaterials; ++materialID)
	{
		aiMaterial* material = aScene->mMaterials[materialID];

		std::vector<Texture*> textures(MATERIAL_TEXTURES_COUNT);

		for (unsigned int textureType = aiTextureType_NONE + 1; textureType <= MATERIAL_TEXTURES_COUNT; ++textureType)
		{
			ASSERT(material->GetTextureCount((aiTextureType)textureType) < 2, "Material has more than one texture of the same type.");
			if (material->GetTextureCount((aiTextureType)textureType))
			{
				aiString string;
				material->GetTexture((aiTextureType)textureType, 0, &string);
				std::string fileName = string.C_Str();
				std::wstring texturePath = mDirectory + std::wstring(fileName.begin(), fileName.end());
				textures[textureType - 1] = Texture::FindOrCreateTexture(texturePath);
			}
		}

		#define GET_MATERIAL_PARAM_COLOR_4D(result, paramKey) \
		{ \
			aiColor4D colorParam; \
			if (material->Get(paramKey, colorParam) == aiReturn_SUCCESS) \
				result = DirectX::XMFLOAT4(colorParam[0], colorParam[1], colorParam[2], colorParam[3]); \
		}

		#define GET_MATERIAL_PARAM_REAL(result, paramKey) \
		{ \
			ai_real realParam; \
			if (material->Get(paramKey, realParam) == aiReturn_SUCCESS) \
				result = realParam; \
		}

		MaterialParams params;
		GET_MATERIAL_PARAM_COLOR_4D(params.DiffuseColor, AI_MATKEY_COLOR_DIFFUSE);
		GET_MATERIAL_PARAM_COLOR_4D(params.AmbientColor, AI_MATKEY_COLOR_AMBIENT);
		GET_MATERIAL_PARAM_COLOR_4D(params.SpecularColor, AI_MATKEY_COLOR_SPECULAR);
		GET_MATERIAL_PARAM_COLOR_4D(params.EmissiveColor, AI_MATKEY_COLOR_EMISSIVE);

		GET_MATERIAL_PARAM_REAL(params.IndexOfRefraction, AI_MATKEY_REFRACTI);
		GET_MATERIAL_PARAM_REAL(params.Opacity, AI_MATKEY_OPACITY);
		GET_MATERIAL_PARAM_REAL(params.SpecularPower, AI_MATKEY_SHININESS);
		GET_MATERIAL_PARAM_REAL(params.SpecularScale, AI_MATKEY_SHININESS_STRENGTH);
		GET_MATERIAL_PARAM_REAL(params.BumpIntensity, AI_MATKEY_BUMPSCALING);

		params.HasAmbientTexture = textures[aiTextureType_AMBIENT - 1] != nullptr;
		params.HasEmissiveTexture = textures[aiTextureType_EMISSIVE - 1] != nullptr;
		params.HasDiffuseTexture = textures[aiTextureType_DIFFUSE - 1] != nullptr;
		params.HasSpecularTexture = textures[aiTextureType_SPECULAR - 1] != nullptr;
		params.HasSpecularPowerTexture = textures[aiTextureType_SHININESS - 1] != nullptr;
		params.HasNormalTexture = textures[aiTextureType_NORMALS - 1] != nullptr;
		params.HasBumpTexture = textures[aiTextureType_HEIGHT - 1] != nullptr;
		params.HasOpacityTexture = textures[aiTextureType_OPACITY - 1] != nullptr;

		Materials::AddMaterial(std::move(params), std::move(textures), material->GetName().C_Str());
	}
}

void Model::LoadMaterialTextures(const aiMaterial* aMaterial, aiTextureType aTextureType, std::vector<Texture*>& aTextures)
{
	ASSERT(aMaterial->GetTextureCount(aTextureType) < 2, "Material has more than one texture of the same type.");
	if (aMaterial->GetTextureCount(aTextureType))
	{
		aiString string;
		aMaterial->GetTexture(aTextureType, 0, &string);
		std::string fileName = string.C_Str();
		std::wstring texturePath = mDirectory + std::wstring(fileName.begin(), fileName.end());
		aTextures[aTextureType - 1] = Texture::FindOrCreateTexture(texturePath);
	}
}

void Model::Render(Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> commandList, UINT aSRVRootParameterIndex, UINT aMaterialIDRootParameterIndex) const
{
	PIX_SCOPED_EVENT(void, commandList.Get(), 0x0000FF, "Draw model: %s", mName.c_str());
	for (const Mesh& mesh : mMeshes)
	{
		mesh.Render(commandList, aSRVRootParameterIndex, aMaterialIDRootParameterIndex);
	}
}
