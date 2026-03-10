#include "pch.h"
#include "Mesh.h"
#include "Utility.h"
#include "PixEvents.h"

Mesh::Mesh(const Buffer& aVertexBuffer, const Buffer& aIndexBuffer, MaterialID aMaterialID, const char* aName)
	: mVertexBuffer(aVertexBuffer)
	, mIndexBuffer(aIndexBuffer)
	, mMaterialID(aMaterialID)
#ifdef _DEBUG
	, mName(aName)
#endif // _DEBUG

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
}

void Mesh::Render(Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> commandList, UINT aSRVRootParameterIndex, UINT aMaterialIDRootParameterIndex) const
{
	commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	commandList->IASetVertexBuffers(0, 1, &m_VertexBufferView);
	commandList->IASetIndexBuffer(&m_IndexBufferView);

	commandList->SetGraphicsRoot32BitConstant(aMaterialIDRootParameterIndex, mMaterialID, 0);

	CD3DX12_GPU_DESCRIPTOR_HANDLE srvHandle;
	srvHandle.InitOffsetted(Graphics::g_SRVDescriptorHeap->GetGPUDescriptorHandleForHeapStart(), mMaterialID * MATERIAL_TEXTURES_COUNT * Graphics::g_SRVDescriptorSize);
	Graphics::g_GraphicsCommandList->SetGraphicsRootDescriptorTable(aSRVRootParameterIndex, srvHandle);

	PIX_SCOPED_EVENT(commandList->DrawIndexedInstanced(m_IndexBufferView.SizeInBytes / sizeof(unsigned int), 1, 0, 0, 0), commandList.Get(), 0x0000FF, "Draw mesh: %s, material %s", mName.c_str(), Materials::GetMaterialName(mMaterialID));
}
