#pragma once

#define NOMINMAX

//#define WIN32_LEAN_AND_MEAN

#include <wrl.h>
#include <Windows.h>
#include <cstdarg>
#include <cstdio>
#include <type_traits>
#include <shlwapi.h>

#include <d3d12.h>
#include <dxgi1_6.h>
#include "d3dx12.h"
#include <d3dcompiler.h>
#ifdef _DEBUG
#include <dxgidebug.h>
#endif

#define D3D12_GPU_VIRTUAL_ADDRESS_NULL      ((D3D12_GPU_VIRTUAL_ADDRESS)0)
#define D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN   ((D3D12_GPU_VIRTUAL_ADDRESS)-1)

#include "Graphics.h"

#include <string>
#include <vector>
#include <map>
