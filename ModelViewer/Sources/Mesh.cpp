#include "pch.h"
#include "Mesh.h"
#include "Utility.h"

Mesh::Mesh(const Buffer& aVertexBuffer, const Buffer& aIndexBuffer, std::vector<Texture*>&& aTextures)
	: mVertexBuffer(aVertexBuffer)
	, mIndexBuffer(aIndexBuffer)
	, mTextures(aTextures)
{
	SetupMesh();
}

void Mesh::SetupMesh()
{
	m_VertexBufferView.BufferLocation = mVertexBuffer.GetGpuVirtualAddress();
	m_VertexBufferView.SizeInBytes = mVertexBuffer.GetSizeInBytes();
	m_VertexBufferView.StrideInBytes = mVertexBuffer.GetElementSizeInBytes();

	m_IndexBufferView.BufferLocation = mIndexBuffer.GetGpuVirtualAddress();
	m_IndexBufferView.Format = DXGI_FORMAT_R32_UINT;
	m_IndexBufferView.SizeInBytes = mIndexBuffer.GetSizeInBytes();

	unsigned nullTexturesCount = std::count(mTextures.begin(), mTextures.end(), nullptr) == mTextures.size();
	if (nullTexturesCount == mTextures.size())
	{
		bool stop = true;
	}


	D3D12_DESCRIPTOR_HEAP_DESC descriptorHeapDesc = {};
	descriptorHeapDesc.NumDescriptors = mTextures.size();// -nullTexturesCount;
	descriptorHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	descriptorHeapDesc.NodeMask = 0;
	descriptorHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;

	ASSERT_HRESULT(Graphics::g_Device->CreateDescriptorHeap(&descriptorHeapDesc, IID_PPV_ARGS(&mSRVDescriptorHeap)), "Failed to create SRV descriptor heap");

	unsigned counter = 0;
	for (UINT i = 0; i < mTextures.size(); ++i)
	{
		if (mTextures[i])
		{
			mTextures[i]->CreateSRV(mSRVDescriptorHeap, i);
			++counter;
		}
	}
}

void Mesh::Render(Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> commandList, UINT rootParameterIndex) const
{
	commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	commandList->IASetVertexBuffers(0, 1, &m_VertexBufferView);
	commandList->IASetIndexBuffer(&m_IndexBufferView);
	
	ID3D12DescriptorHeap* heaps[] = { mSRVDescriptorHeap.Get() };
	Graphics::g_CommandList->SetDescriptorHeaps(1, heaps);
	Graphics::g_CommandList->SetGraphicsRootDescriptorTable(rootParameterIndex, mSRVDescriptorHeap->GetGPUDescriptorHandleForHeapStart());

	commandList->DrawIndexedInstanced(m_IndexBufferView.SizeInBytes / sizeof(unsigned int), 1, 0, 0, 0);
}
