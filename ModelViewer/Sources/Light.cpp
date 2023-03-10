#include "pch.h"
#include "Light.h"
#include "Utility.h"
#include "Buffer.h"
#include "Material.h"

static std::vector<Light> gLights;

static Microsoft::WRL::ComPtr<ID3D12RootSignature> g_RootSignature;
static Microsoft::WRL::ComPtr<ID3D12PipelineState> g_PipelineState;

static Buffer mLightsStructuredBuffer;

static struct CSLightRootConstants
{
    DirectX::XMMATRIX MV;
    UINT32 LightsCount;
} g_CSLightRootConstants;

static Light CreateDirectionalLight(DirectX::XMFLOAT3 directionWS, DirectX::XMFLOAT4 color, float intensity)
{
    Light light;
    light.DirectionWS = directionWS;
    light.Color = color;
    light.Intensity = intensity;
    light.Type = static_cast<UINT32>(LightType::Directional);
    return light;
}

static void InitLights()
{
    gLights.push_back(CreateDirectionalLight(DirectX::XMFLOAT3(0.0f, -1.0f, -1.0f), DirectX::XMFLOAT4(0.1f, 1.0f, 0.1f, 1.0f), 1.0f));
}

namespace Lightning
{
    UINT64 GetLightsCount()
    {
        return gLights.size();
    }

    void Startup()
    {
        Microsoft::WRL::ComPtr<ID3DBlob> computeShaderBlob;
        ASSERT_HRESULT(D3DReadFileToBlob(L"ComputeLightInVS.cso", &computeShaderBlob), "Compute shader compilation failed");

        D3D12_FEATURE_DATA_ROOT_SIGNATURE featureData = {};
        featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_1;
        if (FAILED(Graphics::g_Device->CheckFeatureSupport(D3D12_FEATURE_ROOT_SIGNATURE, &featureData, sizeof(featureData))))
        {
            featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_0;
        }

        CD3DX12_DESCRIPTOR_RANGE1 ranges[1];
        ranges[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 0, 0, D3D12_DESCRIPTOR_RANGE_FLAG_DESCRIPTORS_VOLATILE);

        CD3DX12_ROOT_PARAMETER1 rootParameters[2];
        rootParameters[0].InitAsConstants(sizeof(CSLightRootConstants) / sizeof(float), 0, 0);
        rootParameters[1].InitAsDescriptorTable(1, &ranges[0]);

        CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDescription;
        rootSignatureDescription.Init_1_1(_countof(rootParameters), rootParameters);

        Microsoft::WRL::ComPtr<ID3DBlob> rootSignatureBlob;
        Microsoft::WRL::ComPtr<ID3DBlob> errorBlob;
        ASSERT_HRESULT(D3DX12SerializeVersionedRootSignature(&rootSignatureDescription, featureData.HighestVersion, &rootSignatureBlob, &errorBlob), "Failed to serialize versioned root signature.");

        // Create the root signature.
        ASSERT_HRESULT(Graphics::g_Device->CreateRootSignature(0, rootSignatureBlob->GetBufferPointer(), rootSignatureBlob->GetBufferSize(), IID_PPV_ARGS(&g_RootSignature)), "Failed to create root signature.");

        struct PipelineStateStream
        {
            CD3DX12_PIPELINE_STATE_STREAM_ROOT_SIGNATURE pRootSignature;
            CD3DX12_PIPELINE_STATE_STREAM_CS CS;
        } pipelineStateStream;

        pipelineStateStream.pRootSignature = g_RootSignature.Get();
        pipelineStateStream.CS = CD3DX12_SHADER_BYTECODE(computeShaderBlob.Get());

        D3D12_PIPELINE_STATE_STREAM_DESC pipelineStateStreamDesc = {
        sizeof(PipelineStateStream), &pipelineStateStream
        };

        Microsoft::WRL::ComPtr<ID3D12Device2> device2;
        ASSERT_HRESULT(Graphics::g_Device.As<ID3D12Device2>(&device2), "Failed to receive ID3D12Device2 form ID3D12Device.");
        ASSERT_HRESULT(device2->CreatePipelineState(&pipelineStateStreamDesc, IID_PPV_ARGS(&g_PipelineState)), "Failed to create pipline state object.");
        
        InitLights();

        mLightsStructuredBuffer = Buffer(L"Lights Structured Buffer", GetLightsCount(), sizeof(Light), gLights.data(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
        mLightsStructuredBuffer.CreateSRV(Graphics::g_SRVDescriptorHeap, Materials::GetMaterialCount() * MATERIAL_TEXTURES_COUNT);
        mLightsStructuredBuffer.CreateUAV(Graphics::g_SRVDescriptorHeap, Materials::GetMaterialCount() * MATERIAL_TEXTURES_COUNT + 1);
    }

    void Update(const DirectX::XMMATRIX& MV)
    {
        g_CSLightRootConstants.LightsCount = GetLightsCount();
        g_CSLightRootConstants.MV = MV;

        Microsoft::WRL::ComPtr<ID3D12CommandAllocator> commandAllocator = Graphics::g_ComputeCommandAllocators[Graphics::g_CurrentBackBufferIndex];

        commandAllocator->Reset();
        Graphics::g_ComputeCommandList->Reset(commandAllocator.Get(), nullptr);

        Graphics::g_ComputeCommandList->SetPipelineState(g_PipelineState.Get());
        Graphics::g_ComputeCommandList->SetComputeRootSignature(g_RootSignature.Get());

        Graphics::g_ComputeCommandList->SetComputeRoot32BitConstants(0, sizeof(CSLightRootConstants) / sizeof(float), &g_CSLightRootConstants, 0);

        ID3D12DescriptorHeap* heaps[] = { Graphics::g_SRVDescriptorHeap.Get() };
        Graphics::g_ComputeCommandList->SetDescriptorHeaps(1, heaps);

        CD3DX12_GPU_DESCRIPTOR_HANDLE srvHandle;
        srvHandle.InitOffsetted(Graphics::g_SRVDescriptorHeap->GetGPUDescriptorHandleForHeapStart(), (Materials::GetMaterialCount() * MATERIAL_TEXTURES_COUNT + 1) * Graphics::g_SRVDescriptorSize);
        Graphics::g_ComputeCommandList->SetComputeRootDescriptorTable(1, srvHandle);

        Graphics::g_ComputeCommandList->Dispatch(ceil(GetLightsCount() / 64.0), 1, 1);

        Graphics::g_ComputeCommandList->Close();

        ID3D12CommandList* const commandLists[] = { Graphics::g_ComputeCommandList.Get() };
        Graphics::g_ComputeCommandQueue->ExecuteCommandLists(_countof(commandLists), commandLists);

        Graphics::Signal(Graphics::g_ComputeCommandQueue);
        Graphics::WaitForFenceValue();
    }
}