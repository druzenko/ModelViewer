#pragma once

namespace Graphics
{
    void Initialize(void);
    void Shutdown(void);
    void Present(void);
    uint64_t Signal(Microsoft::WRL::ComPtr<ID3D12CommandQueue> commandQueue);
    void WaitForFenceValue();
    void Flush(void);
    void Resize(int width, int height);
    void GPUCrashCallback(HRESULT errorCode);

    extern constexpr uint32_t g_SwapChainBufferCount = 3;
    extern constexpr uint32_t g_DisplayWidth = 1920;
    extern constexpr uint32_t g_DisplayHeight = 1080;
    extern constexpr float m_FoV = 45.0f;
    extern uint32_t g_CurrentBackBufferIndex;
    extern Microsoft::WRL::ComPtr<ID3D12Device> g_Device;
    extern Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> g_RTVDescriptorHeap;
    extern Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> g_DSVDescriptorHeap;
    extern Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> g_SRVDescriptorHeap;
    extern Microsoft::WRL::ComPtr<ID3D12QueryHeap> g_QueryOcclusionHeap;
    extern UINT g_RtvDescriptorSize;
    extern UINT g_SRVDescriptorSize;
    extern Microsoft::WRL::ComPtr<ID3D12Resource> g_BackBuffers[g_SwapChainBufferCount];
    extern Microsoft::WRL::ComPtr<ID3D12Resource> g_DepthBuffer;
    extern Microsoft::WRL::ComPtr<ID3D12CommandQueue> g_GraphicsCommandQueue;
    extern Microsoft::WRL::ComPtr<ID3D12CommandQueue> g_ComputeCommandQueue;
    extern Microsoft::WRL::ComPtr<ID3D12CommandAllocator> g_GraphicsCommandAllocators[g_SwapChainBufferCount];
    extern Microsoft::WRL::ComPtr<ID3D12CommandAllocator> g_ComputeCommandAllocators[g_SwapChainBufferCount];
    extern Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> g_GraphicsCommandList;
    extern Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> g_ComputeCommandList;
}


