#pragma once

#include "GPUResource.h"

class Buffer : public GpuResource
{
	UINT64 mSizeInBytes = 0;
	UINT64 mElementsCount = 0;
	UINT64 mElementSizeInBytes = 0;

public:
	Buffer() {}
	Buffer(UINT64 elementsCount, UINT64 elementSize, const void* data = nullptr, D3D12_RESOURCE_STATES state = D3D12_RESOURCE_STATE_COPY_DEST);
	UINT64 GetSizeInBytes() const { return mSizeInBytes; }
	UINT64 GetElementsCount() const { return mElementsCount; }
	UINT64 GetElementSizeInBytes() const { return mElementSizeInBytes; }
};

