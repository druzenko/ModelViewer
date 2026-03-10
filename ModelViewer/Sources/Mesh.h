#pragma once

#include <DirectXMath.h>
#include "Texture.h"
#include "Buffer.h"
#include "Material.h"

struct Vertex
{
	DirectX::XMFLOAT3 position;
	DirectX::XMFLOAT3 tangent;
	DirectX::XMFLOAT3 bitangent;
	DirectX::XMFLOAT3 normal;
	DirectX::XMFLOAT2 texCoord;
};

class Mesh
{
	Buffer mVertexBuffer;
	Buffer mIndexBuffer;

	D3D12_VERTEX_BUFFER_VIEW m_VertexBufferView;
	D3D12_INDEX_BUFFER_VIEW m_IndexBufferView;

	MaterialID mMaterialID;

#ifdef _DEBUG
	std::string mName;
#endif // DEBUG


private:
	inline void SetupMesh();

public:
	Mesh(const Buffer& aVertexBuffer, const Buffer& aIndexBuffer, MaterialID aMaterialID, const char* aName);
	void Render(Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> commandList, UINT aSRVRootParameterIndex, UINT aMaterialIDRootParameterIndex) const;
};

