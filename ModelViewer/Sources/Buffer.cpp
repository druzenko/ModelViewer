#include "pch.h"
#include "Buffer.h"
#include "Utility.h"

Buffer::Buffer(UINT64 elementsCount, UINT64 elementSize, const void* data)
    : mSizeInBytes(elementsCount* elementSize)
    , mElementSizeInBytes(elementSize)
{
    D3D12_RESOURCE_DESC bufferDesc = CD3DX12_RESOURCE_DESC::Buffer(mSizeInBytes);
    D3D12_HEAP_PROPERTIES heapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
    if (SUCCEEDED(Graphics::g_Device->CreateCommittedResource(&heapProperties, D3D12_HEAP_FLAG_NONE, &bufferDesc, D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_PPV_ARGS(&m_pResource))))
    {
        m_GpuVirtualAddress = m_pResource->GetGPUVirtualAddress();

        D3D12_SUBRESOURCE_DATA subresource;
        subresource.RowPitch = mSizeInBytes;
        subresource.SlicePitch = subresource.RowPitch;
        subresource.pData = data;

        Microsoft::WRL::ComPtr<ID3D12Resource> intermediateResource;
        heapProperties.Type = D3D12_HEAP_TYPE_UPLOAD;
        if (SUCCEEDED(Graphics::g_Device->CreateCommittedResource(&heapProperties, D3D12_HEAP_FLAG_NONE, &bufferDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&intermediateResource))))
        {
            Graphics::g_CommandList->Reset(Graphics::g_CommandAllocators[Graphics::g_CurrentBackBufferIndex].Get(), nullptr);

            UpdateSubresources(Graphics::g_CommandList.Get(), m_pResource.Get(), intermediateResource.Get(), 0, 0, 1, &subresource);

            Graphics::g_CommandList->Close();

            ID3D12CommandList* const commandLists[] = { Graphics::g_CommandList.Get() };
            Graphics::g_CommandQueue->ExecuteCommandLists(_countof(commandLists), commandLists);

            Graphics::Flush();

            Utility::Printf(L"Buffer resource created");
        }
        else
        {
            Utility::Printf(L"Failed to create intermediate buffer resource");
        }
    }
    else
    {
        Utility::Printf(L"Failed to create buffer resource");
    }
}
