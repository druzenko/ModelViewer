#include <pch.h>
#include <imgui/ImGuiHelper.h>

#include <Application.h>
#include <Graphics.h>
#include <Utility.h>
#include <imgui_impl_win32.h>
#include <imgui_impl_dx12.h>

namespace ImGuiHelper
{
    static constexpr int s_ImGuiSrvHeapSize = 64;

    struct ImGuiDescriptorHeapAllocator
    {
        ImVector<int> m_FreeIndices;
        Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_Heap = nullptr;
        D3D12_CPU_DESCRIPTOR_HANDLE m_HeapStartCpu;
        D3D12_GPU_DESCRIPTOR_HANDLE m_HeapStartGpu;

        void Create()
        {
            ASSERT(m_Heap.Get() == nullptr && m_FreeIndices.empty(), "ImGuiDescriptorHeapAllocator::Create: Heap already created or free indices not empty");

            D3D12_DESCRIPTOR_HEAP_DESC descriptorHeapDesc = {};
            descriptorHeapDesc.NumDescriptors = s_ImGuiSrvHeapSize;
            descriptorHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
            descriptorHeapDesc.NodeMask = 0;
            descriptorHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
            ASSERT_HRESULT(Graphics::g_Device->CreateDescriptorHeap(&descriptorHeapDesc, IID_PPV_ARGS(&m_Heap)), "Failed to create SRV descriptor heap");

            m_HeapStartCpu = m_Heap->GetCPUDescriptorHandleForHeapStart();
            m_HeapStartGpu = m_Heap->GetGPUDescriptorHandleForHeapStart();
            m_FreeIndices.reserve((int)descriptorHeapDesc.NumDescriptors);
            for (int n = descriptorHeapDesc.NumDescriptors; n > 0; n--)
                m_FreeIndices.push_back(n - 1);
        }
        void Destroy()
        {
            m_Heap.Reset();
            m_FreeIndices.clear();
        }
        void Alloc(D3D12_CPU_DESCRIPTOR_HANDLE* out_cpu_desc_handle, D3D12_GPU_DESCRIPTOR_HANDLE* out_gpu_desc_handle)
        {
            ASSERT(m_FreeIndices.Size > 0, "ImGuiDescriptorHeapAllocator::Alloc: No free descriptors available");
            const int idx = m_FreeIndices.back();
            m_FreeIndices.pop_back();
            out_cpu_desc_handle->ptr = m_HeapStartCpu.ptr + (idx * Graphics::g_SRVDescriptorSize);
            out_gpu_desc_handle->ptr = m_HeapStartGpu.ptr + (idx * Graphics::g_SRVDescriptorSize);
        }
        void Free(D3D12_CPU_DESCRIPTOR_HANDLE out_cpu_desc_handle, D3D12_GPU_DESCRIPTOR_HANDLE out_gpu_desc_handle)
        {
            const int cpu_idx = (int)((out_cpu_desc_handle.ptr - m_HeapStartCpu.ptr) / Graphics::g_SRVDescriptorSize);
            const int gpu_idx = (int)((out_gpu_desc_handle.ptr - m_HeapStartGpu.ptr) / Graphics::g_SRVDescriptorSize);
            ASSERT(cpu_idx == gpu_idx, "ImGuiDescriptorHeapAllocator::Free: CPU and GPU descriptor indices do not match");
            m_FreeIndices.push_back(cpu_idx);
        }
    };

    static ImGuiDescriptorHeapAllocator s_ImGuiSrvDescHeapAlloc;

	void Initialize()
	{
        // Setup Dear ImGui context
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO();
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

        s_ImGuiSrvDescHeapAlloc.Create();

        // Setup Platform/Renderer backends
        ImGui_ImplDX12_InitInfo init_info = {};
        init_info.Device = Graphics::g_Device.Get();
        init_info.CommandQueue = Graphics::g_GraphicsCommandQueue.Get();
        init_info.NumFramesInFlight = Graphics::g_SwapChainBufferCount;
        init_info.RTVFormat = Graphics::g_RTVFormat;

        // Allocating SRV descriptors (for textures) is up to the application, so we provide callbacks.
        // The example_win32_directx12/main.cpp application include a simple free-list based allocator.
        init_info.SrvDescriptorHeap = Graphics::g_SRVDescriptorHeap.Get();
        init_info.SrvDescriptorAllocFn = [](ImGui_ImplDX12_InitInfo*, D3D12_CPU_DESCRIPTOR_HANDLE* out_cpu_handle, D3D12_GPU_DESCRIPTOR_HANDLE* out_gpu_handle) { return s_ImGuiSrvDescHeapAlloc.Alloc(out_cpu_handle, out_gpu_handle); };
        init_info.SrvDescriptorFreeFn = [](ImGui_ImplDX12_InitInfo*, D3D12_CPU_DESCRIPTOR_HANDLE cpu_handle, D3D12_GPU_DESCRIPTOR_HANDLE gpu_handle) { return s_ImGuiSrvDescHeapAlloc.Free(cpu_handle, gpu_handle); };

        // (before 1.91.6 the DirectX12 backend required a single SRV descriptor passed)
        // (there is a legacy version of ImGui_ImplDX12_Init() that supports those, but a future version of Dear ImGuii will requires more descriptors to be allocated)

		ImGui_ImplWin32_Init(g_hWnd);
        ImGui_ImplDX12_Init(&init_info);
	}

	void Shutdown()
	{
        ImGui_ImplDX12_Shutdown();
        ImGui_ImplWin32_Shutdown();
        ImGui::DestroyContext();

        s_ImGuiSrvDescHeapAlloc.Destroy();
	}

	void StartFrame()
	{
        ImGui_ImplDX12_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();
        ImGui::ShowDemoWindow();
	}

	void EndFrame()
	{
        // Rendering
        // (Your code clears your framebuffer, renders your other stuff etc.)
        ImGui::Render();
        ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), Graphics::g_GraphicsCommandList.Get());
        // (Your code calls ExecuteCommandLists, swapchain's Present(), etc.)
	}
}