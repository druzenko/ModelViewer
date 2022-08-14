#pragma once

#include "GPUResource.h"

class Buffer : public GpuResource
{
	UINT64 mSizeInBytes;
	UINT64 mElementSizeInBytes;

public:
	Buffer(UINT64 elementsCount, UINT64 elementSize, const void* data);
	UINT64 GetSizeInBytes() const { return mSizeInBytes; }
	UINT64 GetElementSizeInBytes() const { return mElementSizeInBytes; }
};

