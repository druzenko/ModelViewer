#include "pch.h"
#include "Graphics.h"
#include "Application.h"
#include "Utility.h"
#include "dxgidebug.h"
#include <chrono>

#if _DEBUG
#include <filesystem>
#include <shlobj.h>
#include "pix3.h"
#endif

namespace Graphics
{
    constexpr uint32_t vendorID_Nvidia = 4318;
    constexpr uint32_t vendorID_AMD = 4098;
    constexpr uint32_t vendorID_Intel = 8086;

    bool g_VSync = false;
    bool g_TearingSupported = false;
    constexpr DXGI_FORMAT g_SwapChainFormat = DXGI_FORMAT_R8G8B8A8_UNORM;

    uint32_t g_CurrentBackBufferIndex = 0;
    Microsoft::WRL::ComPtr<ID3D12Device> g_Device = nullptr;
    Microsoft::WRL::ComPtr<ID3D12CommandQueue> g_GraphicsCommandQueue = nullptr;
    Microsoft::WRL::ComPtr<ID3D12CommandQueue> g_ComputeCommandQueue = nullptr;
    Microsoft::WRL::ComPtr<IDXGISwapChain3> g_SwapChain3;
    UINT g_RtvDescriptorSize = 0;
    UINT g_SRVDescriptorSize = 0;
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> g_RTVDescriptorHeap = nullptr;
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> g_DSVDescriptorHeap = nullptr;
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> g_SRVDescriptorHeap = nullptr;
    Microsoft::WRL::ComPtr<ID3D12QueryHeap> g_QueryOcclusionHeap = nullptr;
    Microsoft::WRL::ComPtr<ID3D12Resource> g_BackBuffers[g_SwapChainBufferCount];
    Microsoft::WRL::ComPtr<ID3D12Resource> g_DepthBuffer;
    Microsoft::WRL::ComPtr<ID3D12CommandAllocator> g_GraphicsCommandAllocators[g_SwapChainBufferCount];
    Microsoft::WRL::ComPtr<ID3D12CommandAllocator> g_ComputeCommandAllocators[g_SwapChainBufferCount];
    Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> g_GraphicsCommandList = nullptr;
    Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> g_ComputeCommandList = nullptr;
    Microsoft::WRL::ComPtr<ID3D12Fence> g_Fence = nullptr;
    HANDLE g_FenceEvent = nullptr;
    uint64_t g_FenceValue = 0;
    //uint64_t g_FrameFenceValues[g_SwapChainBufferCount] = {};
	

    constexpr bool g_UseWarpDriver = false;

    const wchar_t* GPUVendorToString(uint32_t vendorID)
    {
        switch (vendorID)
        {
        case vendorID_Nvidia:
            return L"Nvidia";
        case vendorID_AMD:
            return L"AMD";
        case vendorID_Intel:
            return L"Intel";
        default:
            return L"Unknown";
            break;
        }
    }

#if _DEBUG
    std::wstring GetLatestWinPixGpuCapturerPath()
    {
        LPWSTR programFilesPath = nullptr;
        SHGetKnownFolderPath(FOLDERID_ProgramFiles, KF_FLAG_DEFAULT, NULL, &programFilesPath);

        std::filesystem::path pixInstallationPath = programFilesPath;
        pixInstallationPath /= "Microsoft PIX";

        std::wstring newestVersionFound;

        for (auto const& directory_entry : std::filesystem::directory_iterator(pixInstallationPath))
        {
            if (directory_entry.is_directory())
            {
                if (newestVersionFound.empty() || newestVersionFound < directory_entry.path().filename().c_str())
                {
                    newestVersionFound = directory_entry.path().filename().c_str();
                }
            }
        }

        if (newestVersionFound.empty())
        {
            // TODO: Error, no PIX installation found
        }

        return pixInstallationPath / newestVersionFound / L"WinPixGpuCapturer.dll";
    }

    static void ProcessDREDAutoBreadcrumbsOutput(const D3D12_DRED_AUTO_BREADCRUMBS_OUTPUT1& DREDOutput, std::string& output)
    {

    }

    static void ProcessDREDPageFaultOutput(const D3D12_DRED_PAGE_FAULT_OUTPUT1& DREDOutput, std::string& output)
    {
        __debugbreak();
    }

    void GPUCrashCallback(HRESULT errorCode)
    {
        switch (errorCode)
        {
        case DXGI_ERROR_DEVICE_REMOVED:
        case DXGI_ERROR_DEVICE_HUNG:
        case DXGI_ERROR_DEVICE_RESET:
        {
            HRESULT reason = g_Device->GetDeviceRemovedReason();
            Microsoft::WRL::ComPtr<ID3D12DeviceRemovedExtendedData1> pDred;
            if (SUCCEEDED(g_Device->QueryInterface(IID_PPV_ARGS(&pDred))))
            {
                std::string output;
                D3D12_DRED_AUTO_BREADCRUMBS_OUTPUT1 DredAutoBreadcrumbsOutput;
                D3D12_DRED_PAGE_FAULT_OUTPUT1 DredPageFaultOutput;

                if (SUCCEEDED(pDred->GetAutoBreadcrumbsOutput1(&DredAutoBreadcrumbsOutput)))
                {
                    ProcessDREDAutoBreadcrumbsOutput(DredAutoBreadcrumbsOutput, output);
                }

                if (SUCCEEDED(pDred->GetPageFaultAllocationOutput1(&DredPageFaultOutput)))
                {
                    ProcessDREDPageFaultOutput(DredPageFaultOutput, output);
                }
            }
            break;
        }
        default:
            break;
        }
    }
#else
    void GPUCrashCallback(HRESULT errorCode) {}
#endif

    uint32_t GetVendorIdFromDevice(ID3D12Device* pDevice)
    {
        LUID luid = pDevice->GetAdapterLuid();

        // Obtain the DXGI factory
        Microsoft::WRL::ComPtr<IDXGIFactory4> dxgiFactory;
        UINT createFactoryFlags = 0;
#if _DEBUG
        createFactoryFlags = DXGI_CREATE_FACTORY_DEBUG;
#endif
        ASSERT_HRESULT(CreateDXGIFactory2(createFactoryFlags, IID_PPV_ARGS(&dxgiFactory)), "IDXGIFactory4 is not created");

        Microsoft::WRL::ComPtr<IDXGIAdapter1> pAdapter;

        if (SUCCEEDED(dxgiFactory->EnumAdapterByLuid(luid, IID_PPV_ARGS(&pAdapter))))
        {
            DXGI_ADAPTER_DESC1 desc;
            if (SUCCEEDED(pAdapter->GetDesc1(&desc)))
            {
                return desc.VendorId;
            }
        }

        return 0;
    }

    bool CheckTearingSupport()
    {
        BOOL allowTearing = FALSE;

        // Rather than create the DXGI 1.5 factory interface directly, we create the
        // DXGI 1.4 interface and query for the 1.5 interface. This is to enable the 
        // graphics debugging tools which will not support the 1.5 factory interface 
        // until a future update.
        Microsoft::WRL::ComPtr<IDXGIFactory4> factory4;
        if (SUCCEEDED(CreateDXGIFactory1(IID_PPV_ARGS(&factory4))))
        {
            Microsoft::WRL::ComPtr<IDXGIFactory5> factory5;
            if (SUCCEEDED(factory4.As(&factory5)))
            {
                if (FAILED(factory5->CheckFeatureSupport(
                    DXGI_FEATURE_PRESENT_ALLOW_TEARING,
                    &allowTearing, sizeof(allowTearing))))
                {
                    allowTearing = FALSE;
                }
            }
        }

        return allowTearing == TRUE;
    }

    static void ResizeDepthBuffer(int width, int height)
    {
        width = std::max(1, width);
        height = std::max(1, height);

        D3D12_DEPTH_STENCIL_VIEW_DESC Desc = {};
        Desc.Format = DXGI_FORMAT_D32_FLOAT;
        Desc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
        Desc.Texture2D.MipSlice = 0;
        Desc.Flags = D3D12_DSV_FLAG_NONE;

        D3D12_CLEAR_VALUE optimizedClearValue = {};
        optimizedClearValue.Format = DXGI_FORMAT_D32_FLOAT;
        optimizedClearValue.DepthStencil = { 1.0f, 0 };

        D3D12_HEAP_PROPERTIES heapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
        D3D12_RESOURCE_DESC resourceDesc = CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_D32_FLOAT, width, height, 1, 0, 1, 0, D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL);
            SUCCEEDED(g_Device->CreateCommittedResource(
                &heapProperties,
                D3D12_HEAP_FLAG_NONE,
                &resourceDesc,
                D3D12_RESOURCE_STATE_DEPTH_WRITE,
                &optimizedClearValue,
                IID_PPV_ARGS(&g_DepthBuffer)
            ));

        g_Device->CreateDepthStencilView(g_DepthBuffer.Get(), &Desc, g_DSVDescriptorHeap->GetCPUDescriptorHandleForHeapStart());
    }

    uint64_t Signal(Microsoft::WRL::ComPtr<ID3D12CommandQueue> commandQueue)
    {
        ASSERT_HRESULT(commandQueue->Signal(g_Fence.Get(), ++g_FenceValue), "Error: ID3D12CommandQueue::Signal");
        return g_FenceValue;
    }

    void WaitForFenceValue()
    {
        if (g_Fence->GetCompletedValue() < g_FenceValue)
        {
            ASSERT_HRESULT(g_Fence->SetEventOnCompletion(g_FenceValue, g_FenceEvent), "Error: ID3D12Fence::SetEventOnCompletion");
            ::WaitForSingleObject(g_FenceEvent, static_cast<DWORD>(std::chrono::milliseconds::max().count()));
        }
    }

    void Flush()
    {
        Signal(g_ComputeCommandQueue);
        WaitForFenceValue();
        Signal(g_GraphicsCommandQueue);
        WaitForFenceValue();
    }

    void Initialize(void)
    {
        Microsoft::WRL::ComPtr<ID3D12Device> pDevice;

        bool useDebugLayers = false;
#if _DEBUG
        if (GetModuleHandle(L"WinPixGpuCapturer.dll") == 0)
        {
            LoadLibrary(GetLatestWinPixGpuCapturerPath().c_str());
        }

        useDebugLayers = true;
#endif

        DWORD dxgiFactoryFlags = 0;

        if (useDebugLayers)
        {
            //DRED
            Microsoft::WRL::ComPtr<ID3D12DeviceRemovedExtendedDataSettings1> pDredSettings;
            if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&pDredSettings))))
            {
                // Turn on AutoBreadcrumbs and Page Fault reporting
                pDredSettings->SetAutoBreadcrumbsEnablement(D3D12_DRED_ENABLEMENT_FORCED_ON);
                pDredSettings->SetPageFaultEnablement(D3D12_DRED_ENABLEMENT_FORCED_ON);
            }


            Microsoft::WRL::ComPtr<ID3D12Debug> debugInterface;
            if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugInterface))))
            {
                debugInterface->EnableDebugLayer();

                Microsoft::WRL::ComPtr<ID3D12Debug1> debugInterface1;
                if (SUCCEEDED((debugInterface->QueryInterface(IID_PPV_ARGS(&debugInterface1)))))
                {
                    debugInterface1->SetEnableGPUBasedValidation(true);
                }
            }
            else
            {
                Utility::Print("WARNING:  Unable to enable D3D12 debug validation layer\n");
            }

#if _DEBUG
            Microsoft::WRL::ComPtr<IDXGIInfoQueue> dxgiInfoQueue;
            if (SUCCEEDED(DXGIGetDebugInterface1(0, IID_PPV_ARGS(dxgiInfoQueue.GetAddressOf()))))
            {
                dxgiFactoryFlags = DXGI_CREATE_FACTORY_DEBUG;

                dxgiInfoQueue->SetBreakOnSeverity(DXGI_DEBUG_ALL, DXGI_INFO_QUEUE_MESSAGE_SEVERITY_CORRUPTION, true);
                dxgiInfoQueue->SetBreakOnSeverity(DXGI_DEBUG_ALL, DXGI_INFO_QUEUE_MESSAGE_SEVERITY_ERROR, true);
                dxgiInfoQueue->SetBreakOnSeverity(DXGI_DEBUG_ALL, DXGI_INFO_QUEUE_MESSAGE_SEVERITY_WARNING, true);
                dxgiInfoQueue->SetBreakOnSeverity(DXGI_DEBUG_ALL, DXGI_INFO_QUEUE_MESSAGE_SEVERITY_INFO, true);
                dxgiInfoQueue->SetBreakOnSeverity(DXGI_DEBUG_ALL, DXGI_INFO_QUEUE_MESSAGE_SEVERITY_MESSAGE, true);

                DXGI_INFO_QUEUE_MESSAGE_ID hide[] =
                {
                    80 /* IDXGISwapChain::GetContainingOutput: The swapchain's adapter does not control the output on which the swapchain's window resides. */,
                };
                DXGI_INFO_QUEUE_FILTER filter = {};
                filter.DenyList.NumIDs = _countof(hide);
                filter.DenyList.pIDList = hide;
                dxgiInfoQueue->AddStorageFilterEntries(DXGI_DEBUG_DXGI, &filter);
            }
#endif
        }

        // Obtain the DXGI factory
        Microsoft::WRL::ComPtr<IDXGIFactory6> dxgiFactory;
        ASSERT_HRESULT(CreateDXGIFactory2(dxgiFactoryFlags, IID_PPV_ARGS(&dxgiFactory)), "IDXGIFactory6 is not created");

        // Create the D3D graphics device
        Microsoft::WRL::ComPtr<IDXGIAdapter1> pAdapter;

        // Temporary workaround because SetStablePowerState() is crashing
        D3D12EnableExperimentalFeatures(0, nullptr, nullptr, nullptr);

        if (!g_UseWarpDriver)
        {
            SIZE_T MaxSize = 0;

            for (uint32_t Idx = 0; DXGI_ERROR_NOT_FOUND != dxgiFactory->EnumAdapters1(Idx, &pAdapter); ++Idx)
            {
                DXGI_ADAPTER_DESC1 desc;
                pAdapter->GetDesc1(&desc);
                if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
                    continue;

                if ((desc.DedicatedVideoMemory > MaxSize) && SUCCEEDED(D3D12CreateDevice(pAdapter.Get(), D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&pDevice))))
                {
                    Utility::Printf(L"D3D12-capable hardware found:  %s (%u MB)\n", desc.Description, desc.DedicatedVideoMemory >> 20);
                    MaxSize = desc.DedicatedVideoMemory;
                }
            }

            if (MaxSize > 0)
                g_Device = pDevice.Detach();
        }

        if (g_Device == nullptr)
        {
            if (g_UseWarpDriver)
                Utility::Print("WARP software adapter requested.  Initializing...\n");
            else
                Utility::Print("Failed to find a hardware adapter.  Falling back to WARP.\n");
            SUCCEEDED(dxgiFactory->EnumWarpAdapter(IID_PPV_ARGS(&pAdapter)));
            SUCCEEDED(D3D12CreateDevice(pAdapter.Get(), D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&pDevice)));
            g_Device = pDevice.Detach();
        }
#ifndef RELEASE
        else
        {
            bool DeveloperModeEnabled = false;

            // Look in the Windows Registry to determine if Developer Mode is enabled
            HKEY hKey;
            LSTATUS result = RegOpenKeyEx(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\AppModelUnlock", 0, KEY_READ, &hKey);
            if (result == ERROR_SUCCESS)
            {
                DWORD keyValue, keySize = sizeof(DWORD);
                result = RegQueryValueEx(hKey, L"AllowDevelopmentWithoutDevLicense", 0, NULL, (byte*)&keyValue, &keySize);
                if (result == ERROR_SUCCESS && keyValue == 1)
                    DeveloperModeEnabled = true;
                RegCloseKey(hKey);
            }

            WARN_ONCE_IF_NOT(DeveloperModeEnabled, "Enable Developer Mode on Windows 10 to get consistent profiling results");

            // Prevent the GPU from overclocking or underclocking to get consistent timings
            if (DeveloperModeEnabled)
                g_Device->SetStablePowerState(TRUE);
        }
#endif

#if _DEBUG
        Microsoft::WRL::ComPtr<ID3D12InfoQueue> pInfoQueue = nullptr;
        if (SUCCEEDED(g_Device->QueryInterface(IID_PPV_ARGS(&pInfoQueue))))
        {
            // Suppress whole categories of messages
            //D3D12_MESSAGE_CATEGORY Categories[] = {};

            // Suppress messages based on their severity level
            D3D12_MESSAGE_SEVERITY Severities[] =
            {
                D3D12_MESSAGE_SEVERITY_INFO
            };

            // Suppress individual messages by their ID
            D3D12_MESSAGE_ID DenyIds[] =
            {
                // This occurs when there are uninitialized descriptors in a descriptor table, even when a
                // shader does not access the missing descriptors.  I find this is common when switching
                // shader permutations and not wanting to change much code to reorder resources.
                D3D12_MESSAGE_ID_INVALID_DESCRIPTOR_HANDLE,

                // Triggered when a shader does not export all color components of a render target, such as
                // when only writing RGB to an R10G10B10A2 buffer, ignoring alpha.
                D3D12_MESSAGE_ID_CREATEGRAPHICSPIPELINESTATE_PS_OUTPUT_RT_OUTPUT_MISMATCH,

                // This occurs when a descriptor table is unbound even when a shader does not access the missing
                // descriptors.  This is common with a root signature shared between disparate shaders that
                // don't all need the same types of resources.
                D3D12_MESSAGE_ID_COMMAND_LIST_DESCRIPTOR_TABLE_NOT_SET,

                // RESOURCE_BARRIER_DUPLICATE_SUBRESOURCE_TRANSITIONS
                (D3D12_MESSAGE_ID)1008,
            };

            D3D12_INFO_QUEUE_FILTER NewFilter = {};
            //NewFilter.DenyList.NumCategories = _countof(Categories);
            //NewFilter.DenyList.pCategoryList = Categories;
            NewFilter.DenyList.NumSeverities = _countof(Severities);
            NewFilter.DenyList.pSeverityList = Severities;
            NewFilter.DenyList.NumIDs = _countof(DenyIds);
            NewFilter.DenyList.pIDList = DenyIds;

            pInfoQueue->PushStorageFilter(&NewFilter);
        }
#endif

        // We like to do read-modify-write operations on UAVs during post processing.  To support that, we
        // need to either have the hardware do typed UAV loads of R11G11B10_FLOAT or we need to manually
        // decode an R32_UINT representation of the same buffer.  This code determines if we get the hardware
        // load support.
        D3D12_FEATURE_DATA_D3D12_OPTIONS FeatureData = {};
        if (SUCCEEDED(g_Device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS, &FeatureData, sizeof(FeatureData))))
        {
            if (FeatureData.TypedUAVLoadAdditionalFormats)
            {
                D3D12_FEATURE_DATA_FORMAT_SUPPORT Support =
                {
                    DXGI_FORMAT_R11G11B10_FLOAT, D3D12_FORMAT_SUPPORT1_NONE, D3D12_FORMAT_SUPPORT2_NONE
                };

                if (SUCCEEDED(g_Device->CheckFeatureSupport(D3D12_FEATURE_FORMAT_SUPPORT, &Support, sizeof(Support))) &&
                    (Support.Support2 & D3D12_FORMAT_SUPPORT2_UAV_TYPED_LOAD) != 0)
                {
                    //g_bTypedUAVLoadSupport_R11G11B10_FLOAT = true;
                }

                Support.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;

                if (SUCCEEDED(g_Device->CheckFeatureSupport(D3D12_FEATURE_FORMAT_SUPPORT, &Support, sizeof(Support))) &&
                    (Support.Support2 & D3D12_FORMAT_SUPPORT2_UAV_TYPED_LOAD) != 0)
                {
                    //g_bTypedUAVLoadSupport_R16G16B16A16_FLOAT = true;
                }
            }
        }

        //Create the command queue
        {
            D3D12_COMMAND_QUEUE_DESC queueDesc = {};
            queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
            queueDesc.NodeMask = 0;
            queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
            queueDesc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;

            SUCCEEDED(g_Device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&g_GraphicsCommandQueue)));

            queueDesc.Type = D3D12_COMMAND_LIST_TYPE_COMPUTE;

            SUCCEEDED(g_Device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&g_ComputeCommandQueue)));
        }

        g_TearingSupported = CheckTearingSupport();

        //Create the swap chain
        {
            UINT createFactoryFlags = 0;
#if _DEBUG
            createFactoryFlags = DXGI_CREATE_FACTORY_DEBUG;
#endif
            Microsoft::WRL::ComPtr<IDXGIFactory4> dxgiFactory4;
            SUCCEEDED(CreateDXGIFactory2(createFactoryFlags, IID_PPV_ARGS(&dxgiFactory4)));

            DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
            swapChainDesc.Width = g_DisplayWidth;
            swapChainDesc.Height = g_DisplayHeight;
            swapChainDesc.Format = g_SwapChainFormat;
            swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
            swapChainDesc.BufferCount = g_SwapChainBufferCount;
            swapChainDesc.SampleDesc.Count = 1;
            swapChainDesc.SampleDesc.Quality = 0;
            swapChainDesc.Scaling = DXGI_SCALING_NONE;
            swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
            swapChainDesc.AlphaMode = DXGI_ALPHA_MODE_IGNORE;
            swapChainDesc.Flags = g_TearingSupported ? DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING : 0;
            swapChainDesc.Flags |= DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

            DXGI_SWAP_CHAIN_FULLSCREEN_DESC fsSwapChainDesc = {};
            fsSwapChainDesc.Windowed = TRUE;

            Microsoft::WRL::ComPtr<IDXGISwapChain1> swapChain = nullptr;
            SUCCEEDED(dxgiFactory4->CreateSwapChainForHwnd(g_GraphicsCommandQueue.Get(), g_hWnd, &swapChainDesc, &fsSwapChainDesc, nullptr, &swapChain));
            SUCCEEDED(swapChain.As(&g_SwapChain3));
            g_CurrentBackBufferIndex = g_SwapChain3->GetCurrentBackBufferIndex();
        }

        //Create RTV descriptor heap
        {
            D3D12_DESCRIPTOR_HEAP_DESC Desc;
            Desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
            Desc.NumDescriptors = g_SwapChainBufferCount;
            Desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
            Desc.NodeMask = 1;

            SUCCEEDED(Graphics::g_Device->CreateDescriptorHeap(&Desc, IID_PPV_ARGS(&g_RTVDescriptorHeap)));
        }

        //Create RTVs
        {
            g_RtvDescriptorSize = g_Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

            CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(g_RTVDescriptorHeap->GetCPUDescriptorHandleForHeapStart());

            for (int i = 0; i < g_SwapChainBufferCount; ++i)
            {
                SUCCEEDED(g_SwapChain3->GetBuffer(i, IID_PPV_ARGS(&g_BackBuffers[i])));
                g_Device->CreateRenderTargetView(g_BackBuffers[i].Get(), nullptr, rtvHandle);
                rtvHandle.Offset(g_RtvDescriptorSize);
            }
        }

        //Create command allocators
        {
            for (int i = 0; i < g_SwapChainBufferCount; ++i)
            {
                SUCCEEDED(g_Device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&g_GraphicsCommandAllocators[i])));
                SUCCEEDED(g_Device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_COMPUTE, IID_PPV_ARGS(&g_ComputeCommandAllocators[i])));
            }
        }

        //Create command list
        {
            SUCCEEDED(g_Device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, g_GraphicsCommandAllocators[g_CurrentBackBufferIndex].Get(), nullptr, IID_PPV_ARGS(&g_GraphicsCommandList)));
            SUCCEEDED(g_GraphicsCommandList->Close());

            SUCCEEDED(g_Device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_COMPUTE, g_ComputeCommandAllocators[g_CurrentBackBufferIndex].Get(), nullptr, IID_PPV_ARGS(&g_ComputeCommandList)));
            SUCCEEDED(g_ComputeCommandList->Close());
        }

        //Create fence
        {
            SUCCEEDED(g_Device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&g_Fence)));
        }

        //Create fence event
        {
            g_FenceEvent = ::CreateEvent(NULL, FALSE, FALSE, NULL);
            ASSERT(g_FenceEvent, "Failed to create fence event.");
        }

        //Create DSV descriptor heap
        {
            D3D12_DESCRIPTOR_HEAP_DESC Desc;
            Desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
            Desc.NumDescriptors = 1;
            Desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
            Desc.NodeMask = 1;

            SUCCEEDED(g_Device->CreateDescriptorHeap(&Desc, IID_PPV_ARGS(&g_DSVDescriptorHeap)));
        }

        //Create a heap for occlusion queries
        {
            D3D12_QUERY_HEAP_DESC queryHeapDesc = {};
            queryHeapDesc.Count = 1;
            queryHeapDesc.Type = D3D12_QUERY_HEAP_TYPE_OCCLUSION;
            SUCCEEDED(g_Device->CreateQueryHeap(&queryHeapDesc, IID_PPV_ARGS(&g_QueryOcclusionHeap)));
        }

        //Get SRV descriptor size
        g_SRVDescriptorSize = g_Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

        ResizeDepthBuffer(g_DisplayWidth, g_DisplayHeight);
    }

    void Resize(int width, int height)
    {
        Flush();
        ResizeDepthBuffer(width, height);
    }

    void Shutdown(void)
    {
        Flush();

        ::CloseHandle(g_FenceEvent);

        g_GraphicsCommandQueue.Reset();
        g_RTVDescriptorHeap.Reset();
        for (int i = 0; i < g_SwapChainBufferCount; ++i)
        {
            g_GraphicsCommandAllocators[i].Reset();
            g_ComputeCommandAllocators[i].Reset();
        }
        g_GraphicsCommandList.Reset();
        g_ComputeCommandList.Reset();
        g_Fence.Reset();
        g_Device.Reset();

#if _DEBUG
        Microsoft::WRL::ComPtr<IDXGIDebug> dxfiDebug = nullptr;
        if (SUCCEEDED(DXGIGetDebugInterface1(0, IID_PPV_ARGS(&dxfiDebug))))
        {
            dxfiDebug->ReportLiveObjects(DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_ALL);
        }
#endif
    }

    void Present(void)
    {
        UINT syncInterval = g_VSync ? 1 : 0;
        UINT presentFlags = g_TearingSupported && !g_VSync ? DXGI_PRESENT_ALLOW_TEARING : 0;
        ASSERT_HRESULT(g_SwapChain3->Present(syncInterval, presentFlags), "Error: IDXGISwapChain3::Present");
 
        /*g_FrameFenceValues[g_CurrentBackBufferIndex] = */Signal(g_GraphicsCommandQueue);

        g_CurrentBackBufferIndex = g_SwapChain3->GetCurrentBackBufferIndex();

        WaitForFenceValue();
    }
}
