#pragma once

#include "pch.h"
#include "GPUResource.h"

class Texture : public GpuResource
{
public:
    //Texture() { m_hCpuDescriptorHandle.ptr = D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN; }
    //Texture(D3D12_CPU_DESCRIPTOR_HANDLE Handle) : m_hCpuDescriptorHandle(Handle) {}
    static Texture* FindOrCreateTexture(const std::wstring& aPath);

    void CreateSRV(Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> SRVDescriptorHeap, UINT offset) const;

    virtual void Destroy() override
    {
        GpuResource::Destroy();
        m_hCpuDescriptorHandle.ptr = D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN;
    }

    uint32_t GetWidth() const { return m_Width; }
    uint32_t GetHeight() const { return m_Height; }
    uint32_t GetDepth() const { return m_Depth; }
    DXGI_FORMAT GetFormat() const { return m_Format; }
    const D3D12_CPU_DESCRIPTOR_HANDLE& GetSRV() const { return m_hCpuDescriptorHandle; }

private:
    Texture(const std::wstring& aPath);

    static std::map<std::wstring, Texture> sTextureRegister;

    uint32_t m_Width;
    uint32_t m_Height;
    uint32_t m_Depth;
    DXGI_FORMAT m_Format;
    D3D12_CPU_DESCRIPTOR_HANDLE m_hCpuDescriptorHandle;
};

