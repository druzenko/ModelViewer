#include "pch.h"
#include "Texture.h"
#include "DirectXTex.h"
#include "Utility.h"

std::map<std::wstring, Texture> Texture::sTextureRegister;

Texture* Texture::FindOrCreateTexture(const std::wstring& aPath)
{
    auto findIt = sTextureRegister.find(aPath);
    if (findIt != sTextureRegister.end())
    {
        return &findIt->second;
    }
    else
    {
        return &(sTextureRegister.emplace(std::make_pair(aPath, Texture(aPath))).first->second);
    }
}

Texture::Texture(const std::wstring& aPath)
{
#ifdef _DEBUG
    mName = aPath.substr(aPath.find_last_of('\\') + 1);
#endif

	auto image = std::make_unique<DirectX::ScratchImage>();
	if (SUCCEEDED(LoadFromWICFile(aPath.c_str(), DirectX::WIC_FLAGS_NONE, nullptr, *image)))
	{
		const DirectX::TexMetadata metaData = image->GetMetadata();
		m_Width = metaData.width;
		m_Height = metaData.height;
		m_Depth = metaData.depth;
        m_Format = metaData.format;

        D3D12_RESOURCE_DESC texDesc = {};
        switch (metaData.dimension)
        {
            case DirectX::TEX_DIMENSION_TEXTURE1D:
                texDesc = CD3DX12_RESOURCE_DESC::Tex1D(m_Format, m_Width, metaData.arraySize);
                break;
            case DirectX::TEX_DIMENSION_TEXTURE2D:
                texDesc = CD3DX12_RESOURCE_DESC::Tex2D(m_Format, m_Width, m_Height, metaData.arraySize);
                break;
            case DirectX::TEX_DIMENSION_TEXTURE3D:
                texDesc = CD3DX12_RESOURCE_DESC::Tex3D(m_Format, m_Width, m_Height, m_Depth, metaData.arraySize);
                break;
            default:
                ASSERT(false, "Wrong texture dimension.");
                break;
        }

        D3D12_HEAP_PROPERTIES heapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
        if (SUCCEEDED(Graphics::g_Device->CreateCommittedResource(&heapProperties, D3D12_HEAP_FLAG_NONE, &texDesc, D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_PPV_ARGS(&m_pResource))))
        {
#ifdef _DEBUG
            m_pResource->SetName(mName.c_str());
#endif
            mFormat = m_pResource->GetDesc().Format;

            UINT64 subresourcesCount = image->GetImageCount();
            std::vector<D3D12_SUBRESOURCE_DATA> subresources(subresourcesCount);
            const DirectX::Image* pImages = image->GetImages();

            for (UINT64 i = 0; i < subresourcesCount; ++i)
            {
                D3D12_SUBRESOURCE_DATA& subresource = subresources[i];
                const DirectX::Image& image = pImages[i];
                subresource.RowPitch = image.rowPitch;
                subresource.SlicePitch = image.slicePitch;
                subresource.pData = image.pixels;
            }

            UINT64 requiredSize = GetRequiredIntermediateSize(m_pResource.Get(), 0, subresourcesCount);
            Microsoft::WRL::ComPtr<ID3D12Resource> intermediateResource;
            heapProperties.Type = D3D12_HEAP_TYPE_UPLOAD;
            D3D12_RESOURCE_DESC resourceDesc = CD3DX12_RESOURCE_DESC::Buffer(requiredSize);
            if (SUCCEEDED(Graphics::g_Device->CreateCommittedResource(&heapProperties, D3D12_HEAP_FLAG_NONE, &resourceDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&intermediateResource))))
            {
                Graphics::g_GraphicsCommandList->Reset(Graphics::g_GraphicsCommandAllocators[Graphics::g_CurrentBackBufferIndex].Get(), nullptr);

                UpdateSubresources(Graphics::g_GraphicsCommandList.Get(), m_pResource.Get(), intermediateResource.Get(), 0, 0, subresourcesCount, subresources.data());

                CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(m_pResource.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
                Graphics::g_GraphicsCommandList->ResourceBarrier(1, &barrier);

                Graphics::g_GraphicsCommandList->Close();

                ID3D12CommandList* const commandLists[] = { Graphics::g_GraphicsCommandList.Get() };
                Graphics::g_GraphicsCommandQueue->ExecuteCommandLists(_countof(commandLists), commandLists);

                Graphics::Flush();

                static unsigned texturesCount = 0;
                ++texturesCount;
                Utility::Printf(L"Texture resource created: %s. Textures count = %lu", aPath.c_str(), texturesCount);
            }
            else
            {
                Utility::Printf(L"Failed to create intermediate resource for texture: %s", aPath.c_str());
            }
        }
        else
        {
            Utility::Printf(L"Failed to create resource for texture: %s", aPath.c_str());
        }
	}
}

void Texture::CreateSRV(Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> SRVDescriptorHeap, UINT offset) const
{
    D3D12_SHADER_RESOURCE_VIEW_DESC shaderResourceViewDesc = {};
    shaderResourceViewDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    shaderResourceViewDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    shaderResourceViewDesc.Format = m_Format;
    shaderResourceViewDesc.Texture2D.MipLevels = 1;
    shaderResourceViewDesc.Texture2D.MostDetailedMip = 0;
    shaderResourceViewDesc.Texture2D.ResourceMinLODClamp = 0.0f;

    CD3DX12_CPU_DESCRIPTOR_HANDLE srvHandle;
    srvHandle.InitOffsetted(SRVDescriptorHeap->GetCPUDescriptorHandleForHeapStart(), offset * Graphics::g_SRVDescriptorSize);

    Graphics::g_Device->CreateShaderResourceView(m_pResource.Get(), &shaderResourceViewDesc, srvHandle);
}

void Texture::CreateEmptySRV(Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> SRVDescriptorHeap, UINT offset)
{
    D3D12_SHADER_RESOURCE_VIEW_DESC shaderResourceViewDesc = {};
    shaderResourceViewDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    shaderResourceViewDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    shaderResourceViewDesc.Format = DXGI_FORMAT_R32G32B32A32_UINT;
    shaderResourceViewDesc.Texture2D.MipLevels = 1;
    shaderResourceViewDesc.Texture2D.MostDetailedMip = 0;
    shaderResourceViewDesc.Texture2D.ResourceMinLODClamp = 0.0f;

    CD3DX12_CPU_DESCRIPTOR_HANDLE srvHandle;
    srvHandle.InitOffsetted(SRVDescriptorHeap->GetCPUDescriptorHandleForHeapStart(), offset * Graphics::g_SRVDescriptorSize);

    Graphics::g_Device->CreateShaderResourceView(nullptr, &shaderResourceViewDesc, srvHandle);
}