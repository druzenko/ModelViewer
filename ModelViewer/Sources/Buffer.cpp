#include "pch.h"
#include "Buffer.h"
#include "Utility.h"

Buffer::Buffer(const wchar_t* name, UINT64 elementsCount, UINT64 elementSize, const void* data, D3D12_RESOURCE_STATES stateAfter, D3D12_RESOURCE_FLAGS flags)
    : mSizeInBytes(elementsCount* elementSize)
    , mElementsCount(elementsCount)
    , mElementSizeInBytes(elementSize)
{
    D3D12_RESOURCE_DESC bufferDesc = CD3DX12_RESOURCE_DESC::Buffer(mSizeInBytes, flags);
    D3D12_HEAP_PROPERTIES heapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
    if (SUCCEEDED(Graphics::g_Device->CreateCommittedResource(&heapProperties, D3D12_HEAP_FLAG_NONE, &bufferDesc, D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_PPV_ARGS(&m_pResource))))
    {
#ifdef _DEBUG
        m_pResource->SetName(name);
#endif
        m_GpuVirtualAddress = m_pResource->GetGPUVirtualAddress();
        mFormat = m_pResource->GetDesc().Format;

        if (data)
        {
            D3D12_SUBRESOURCE_DATA subresource;
            subresource.RowPitch = mSizeInBytes;
            subresource.SlicePitch = subresource.RowPitch;
            subresource.pData = data;

            Microsoft::WRL::ComPtr<ID3D12Resource> intermediateResource;
            heapProperties.Type = D3D12_HEAP_TYPE_UPLOAD;
            bufferDesc.Flags &= ~D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
            if (SUCCEEDED(Graphics::g_Device->CreateCommittedResource(&heapProperties, D3D12_HEAP_FLAG_NONE, &bufferDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&intermediateResource))))
            {
                Graphics::g_GraphicsCommandList->Reset(Graphics::g_GraphicsCommandAllocators[Graphics::g_CurrentBackBufferIndex].Get(), nullptr);

                UpdateSubresources(Graphics::g_GraphicsCommandList.Get(), m_pResource.Get(), intermediateResource.Get(), 0, 0, 1, &subresource);

                CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(m_pResource.Get(), D3D12_RESOURCE_STATE_COPY_DEST, stateAfter);
                Graphics::g_GraphicsCommandList->ResourceBarrier(1, &barrier);

                Graphics::g_GraphicsCommandList->Close();

                ID3D12CommandList* const commandLists[] = { Graphics::g_GraphicsCommandList.Get() };
                Graphics::g_GraphicsCommandQueue->ExecuteCommandLists(_countof(commandLists), commandLists);

                Graphics::Flush();

                Utility::Printf(L"Buffer resource created");
            }
            else
            {
                Utility::Printf(L"Failed to create intermediate buffer resource");
            }
        }
    }
    else
    {
        Utility::Printf(L"Failed to create buffer resource");
    }
}

void Buffer::CreateSRV(Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> SRVDescriptorHeap, UINT offset) const
{
    D3D12_SHADER_RESOURCE_VIEW_DESC shaderResourceViewDesc = {};
    shaderResourceViewDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
    shaderResourceViewDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    shaderResourceViewDesc.Format = DXGI_FORMAT_UNKNOWN;
    shaderResourceViewDesc.Buffer.FirstElement = 0;
    shaderResourceViewDesc.Buffer.NumElements = mElementsCount;
    shaderResourceViewDesc.Buffer.StructureByteStride = mElementSizeInBytes;

    CD3DX12_CPU_DESCRIPTOR_HANDLE srvHandle;
    srvHandle.InitOffsetted(SRVDescriptorHeap->GetCPUDescriptorHandleForHeapStart(), offset * Graphics::g_SRVDescriptorSize);

    Graphics::g_Device->CreateShaderResourceView(m_pResource.Get(), &shaderResourceViewDesc, srvHandle);
}

void Buffer::CreateUAV(Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> SRVDescriptorHeap, UINT offset) const
{
    D3D12_UNORDERED_ACCESS_VIEW_DESC unorderedAccessViewDesc = {};
    unorderedAccessViewDesc.Format = DXGI_FORMAT_UNKNOWN;
    unorderedAccessViewDesc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
    unorderedAccessViewDesc.Buffer.FirstElement = 0;
    unorderedAccessViewDesc.Buffer.NumElements = mElementsCount;
    unorderedAccessViewDesc.Buffer.StructureByteStride = mElementSizeInBytes;
    unorderedAccessViewDesc.Buffer.CounterOffsetInBytes = 0;
    unorderedAccessViewDesc.Buffer.Flags = D3D12_BUFFER_UAV_FLAG_NONE;

    CD3DX12_CPU_DESCRIPTOR_HANDLE srvHandle;
    srvHandle.InitOffsetted(SRVDescriptorHeap->GetCPUDescriptorHandleForHeapStart(), offset * Graphics::g_SRVDescriptorSize);

    Graphics::g_Device->CreateUnorderedAccessView(m_pResource.Get(), nullptr, &unorderedAccessViewDesc, srvHandle);
}