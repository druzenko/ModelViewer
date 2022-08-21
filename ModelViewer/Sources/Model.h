#pragma once

#include "Mesh.h"
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

class Model
{
	std::vector<Mesh> mMeshes;
	std::wstring mDirectory;

private:
	void LoadModel(const std::string& aPath);
	void ProcessNode(const aiNode* aNode, const aiScene* aScene);
	Mesh ProcessMesh(const aiMesh* aMesh, const aiScene* aScene);
	std::vector<Texture*> LoadMaterialTextures(const aiMaterial* aMaterial, aiTextureType aTextureType);

public:
	Model() {}
	Model(const std::string& aPath);
	void Render(Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> commandList, UINT rootParameterIndex) const;
};

