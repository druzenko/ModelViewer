#pragma once

#include "GPUResource.h"

class Buffer : public GpuResource
{
	UINT64 mSizeInBytes = 0;
	UINT64 mElementsCount = 0;
	UINT64 mElementSizeInBytes = 0;

public:
	Buffer() {}
	Buffer(const wchar_t* name, UINT64 elementsCount, UINT64 elementSize, const void* data = nullptr, D3D12_RESOURCE_STATES stateAfter = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_NONE);
	UINT64 GetSizeInBytes() const { return mSizeInBytes; }
	UINT64 GetElementsCount() const { return mElementsCount; }
	UINT64 GetElementSizeInBytes() const { return mElementSizeInBytes; }

	void CreateSRV(Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> SRVDescriptorHeap, UINT offset) const override;
	void CreateUAV(Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> SRVDescriptorHeap, UINT offset) const override;
};

