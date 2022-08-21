#pragma once

#include <DirectXMath.h>
#include "Texture.h"
#include "Buffer.h"

struct Vertex
{
	DirectX::XMFLOAT3 position;
	DirectX::XMFLOAT3 normal;
	DirectX::XMFLOAT2 texCoord;
};

class Mesh
{
	Buffer mVertexBuffer;
	Buffer mIndexBuffer;
	std::vector<Texture*> mTextures;

	D3D12_VERTEX_BUFFER_VIEW m_VertexBufferView;
	D3D12_INDEX_BUFFER_VIEW m_IndexBufferView;

	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> mSRVDescriptorHeap;

private:
	inline void SetupMesh();

public:
	Mesh(const Buffer& aVertexBuffer, const Buffer& aIndexBuffer, std::vector<Texture*>&& aTextures);
	void Render(Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> commandList, UINT rootParameterIndex) const;
};

