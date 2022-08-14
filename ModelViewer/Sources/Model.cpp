#include "pch.h"
#include "Model.h"
#include "Utility.h"

Model::Model(const std::string& aPath)
{
	std::string directory = aPath.substr(0, aPath.find_last_of('/') + 1);
	mDirectory = std::wstring(directory.begin(), directory.end());
	LoadModel(aPath);
}

void Model::LoadModel(const std::string& aPath)
{
	Assimp::Importer importer;
	//const aiScene* scene = importer.ReadFile(aPath.data(), aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_GenSmoothNormals);
	const aiScene* scene = importer.ReadFile(aPath.data(), aiProcess_Triangulate | aiProcess_ConvertToLeftHanded | aiProcess_GenSmoothNormals);

	ASSERT(scene != nullptr && (scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE) == 0 && scene->mRootNode != nullptr, "Model is not loaded.");

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
	std::vector<UINT16> indices;
	std::vector<Texture> textures;

	for (unsigned int i = 0; i < aMesh->mNumVertices; ++i)
	{
		Vertex& vertex = vertices[i];

		vertex.position.x = aMesh->mVertices[i].x;
		vertex.position.y = aMesh->mVertices[i].y;
		vertex.position.z = aMesh->mVertices[i].z;

		vertex.normal.x = aMesh->mNormals[i].x;
		vertex.normal.y = aMesh->mNormals[i].y;
		vertex.normal.z = aMesh->mNormals[i].z;

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

	if (aMesh->mMaterialIndex >= 0)
	{
		aiMaterial* material = aScene->mMaterials[aMesh->mMaterialIndex];

		std::vector<Texture> diffuseMaps = LoadMaterialTextures(material, aiTextureType_DIFFUSE, "texture_diffuse");
		textures.insert(textures.end(), diffuseMaps.begin(), diffuseMaps.end());

		std::vector<Texture> specularMaps = LoadMaterialTextures(material, aiTextureType_SPECULAR, "texture_specular");
		textures.insert(textures.end(), specularMaps.begin(), specularMaps.end());
	}

	return Mesh(Buffer(vertices.size(), sizeof(Vertex), vertices.data()), Buffer(indices.size(), sizeof(UINT16), indices.data()), std::move(textures));
}

std::vector<Texture> Model::LoadMaterialTextures(const aiMaterial* aMaterial, aiTextureType aTextureType, std::string aTypeName)
{
	std::vector<Texture> textures;
	for (unsigned int i = 0; i < aMaterial->GetTextureCount(aTextureType); ++i)
	{
		aiString string;
		aMaterial->GetTexture(aTextureType, i, &string);
		std::string fileName = string.C_Str();
		std::wstring texturePath = mDirectory + std::wstring(fileName.begin(), fileName.end());
		textures.emplace_back(Texture(texturePath));
	}
	return textures;
}

void Model::Render(Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> commandList, UINT rootParameterIndex) const
{
	for (const Mesh& mesh : mMeshes)
	{
		mesh.Render(commandList, rootParameterIndex);
	}
}
