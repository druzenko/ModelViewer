#include "pch.h"
#include "Application.h"
#include "Model.h"
#include "Camera.h"
#include "Material.h"
#include <assimp/scene.h>

#if _DEBUG
#include "pix3.h"
#endif

#include "Utility.h"

struct Transform
{
    DirectX::XMMATRIX MVP;
    DirectX::XMMATRIX MV;
};

class ModelViewer : public IApplication
{
    DirectX::XMMATRIX m_ModelMatrix;
    DirectX::XMMATRIX m_ProjectionMatrix;

    D3D12_VIEWPORT m_Viewport;
    D3D12_RECT m_ScissorRect;

    Camera mCamera;

    Microsoft::WRL::ComPtr<ID3D12RootSignature> m_RootSignature;
    Microsoft::WRL::ComPtr<ID3D12PipelineState> m_PipelineState;

    Model m_Model;

    Material mMaterial;
    Buffer mMaterialCBV;

    float m_LastFrameTime;
    float m_CameraSpeed = 10.0f;

    float mLastDeltaX = Graphics::g_DisplayWidth / 2;
    float mLastDeltaY = Graphics::g_DisplayWidth / 2;
    bool mIsFirstMouseMove = true;

public:

    ModelViewer();

    void Startup(void) override;
    void Cleanup(void) override;
    void Update(double deltaT) override;
    void RenderScene(void) override;
    void OnKeyEvent(const KeyEvent& keyEvent) override;
    void OnMouseMoved(int aDeltaX, int aDeltaY) override;
};

CREATE_APPLICATION(ModelViewer)

ModelViewer::ModelViewer()
    : mCamera(DirectX::XMVectorSet(0, 10, -15, 1), DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 1.0f), gYaw, gPitch)
{

}

void ModelViewer::Startup(void)
{
    m_Model = Model("../../Scenes/sponza/sponza.obj");
    //m_Model = Model("../../Scenes/nanosuit/nanosuit.obj");

    // Load the vertex shader.
    Microsoft::WRL::ComPtr<ID3DBlob> vertexShaderBlob;
    ASSERT_HRESULT(D3DReadFileToBlob(L"VertexShader.cso", &vertexShaderBlob), "Vertex shader compilation failed");

    // Load the pixel shader.
    Microsoft::WRL::ComPtr<ID3DBlob> pixelShaderBlob;
    ASSERT_HRESULT(D3DReadFileToBlob(L"PixelShader.cso", &pixelShaderBlob), "Pixel shader compilation failed");

    // Create the vertex input layout
    D3D12_INPUT_ELEMENT_DESC inputLayout[] = {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "TANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "BINORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
    };

    // Create a root signature.
    D3D12_FEATURE_DATA_ROOT_SIGNATURE featureData = {};
    featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_1;
    if (FAILED(Graphics::g_Device->CheckFeatureSupport(D3D12_FEATURE_ROOT_SIGNATURE, &featureData, sizeof(featureData))))
    {
        featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_0;
    }

    // Allow input layout and deny unnecessary access to certain pipeline stages.
    D3D12_ROOT_SIGNATURE_FLAGS rootSignatureFlags =
        D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
        D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
        D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
        D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS;// |
        //D3D12_ROOT_SIGNATURE_FLAG_DENY_PIXEL_SHADER_ROOT_ACCESS;

    // A single 32-bit constant root parameter that is used by the vertex shader.
    CD3DX12_ROOT_PARAMETER1 rootParameters[3];
    rootParameters[0].InitAsConstants(sizeof(Transform) / sizeof(float), 0, 0, D3D12_SHADER_VISIBILITY_VERTEX);

    CD3DX12_DESCRIPTOR_RANGE1 range;
    range.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, AI_TEXTURE_TYPE_MAX, 0);

    rootParameters[1].InitAsDescriptorTable(1, &range, D3D12_SHADER_VISIBILITY_PIXEL);

    rootParameters[2].InitAsConstantBufferView(0, 0, D3D12_ROOT_DESCRIPTOR_FLAG_NONE, D3D12_SHADER_VISIBILITY_PIXEL);

    CD3DX12_STATIC_SAMPLER_DESC samplerDesc;
    samplerDesc.Init(0, D3D12_FILTER_MIN_MAG_LINEAR_MIP_POINT);

    CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDescription;
    rootSignatureDescription.Init_1_1(_countof(rootParameters), rootParameters, 1, &samplerDesc, rootSignatureFlags);

    // Serialize the root signature.
    Microsoft::WRL::ComPtr<ID3DBlob> rootSignatureBlob;
    Microsoft::WRL::ComPtr<ID3DBlob> errorBlob;
    ASSERT_HRESULT(D3DX12SerializeVersionedRootSignature(&rootSignatureDescription, featureData.HighestVersion, &rootSignatureBlob, &errorBlob), "Failed to serialize versioned root signature.");

    // Create the root signature.
    ASSERT_HRESULT(Graphics::g_Device->CreateRootSignature(0, rootSignatureBlob->GetBufferPointer(), rootSignatureBlob->GetBufferSize(), IID_PPV_ARGS(&m_RootSignature)), "Failed to create root signature.");

    struct PipelineStateStream
    {
        CD3DX12_PIPELINE_STATE_STREAM_ROOT_SIGNATURE pRootSignature;
        CD3DX12_PIPELINE_STATE_STREAM_INPUT_LAYOUT InputLayout;
        CD3DX12_PIPELINE_STATE_STREAM_PRIMITIVE_TOPOLOGY PrimitiveTopologyType;
        CD3DX12_PIPELINE_STATE_STREAM_VS VS;
        CD3DX12_PIPELINE_STATE_STREAM_PS PS;
        CD3DX12_PIPELINE_STATE_STREAM_DEPTH_STENCIL_FORMAT DSVFormat;
        CD3DX12_PIPELINE_STATE_STREAM_RENDER_TARGET_FORMATS RTVFormats;
    } pipelineStateStream;

    D3D12_RT_FORMAT_ARRAY rtvFormats = {};
    rtvFormats.NumRenderTargets = 1;
    rtvFormats.RTFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;

    pipelineStateStream.pRootSignature = m_RootSignature.Get();
    pipelineStateStream.InputLayout = { inputLayout, _countof(inputLayout) };
    pipelineStateStream.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    pipelineStateStream.VS = CD3DX12_SHADER_BYTECODE(vertexShaderBlob.Get());
    pipelineStateStream.PS = CD3DX12_SHADER_BYTECODE(pixelShaderBlob.Get());
    pipelineStateStream.DSVFormat = DXGI_FORMAT_D32_FLOAT;
    pipelineStateStream.RTVFormats = rtvFormats;

    D3D12_PIPELINE_STATE_STREAM_DESC pipelineStateStreamDesc = {
        sizeof(PipelineStateStream), &pipelineStateStream
    };

    Microsoft::WRL::ComPtr<ID3D12Device2> device2;
    ASSERT_HRESULT(Graphics::g_Device.As<ID3D12Device2>(&device2), "Failed to receive ID3D12Device2 form ID3D12Device.");
    ASSERT_HRESULT(device2->CreatePipelineState(&pipelineStateStreamDesc, IID_PPV_ARGS(&m_PipelineState)), "Failed to create pipline state object.");

    m_ModelMatrix = DirectX::XMMatrixIdentity();
    m_ModelMatrix = DirectX::XMMatrixScaling(0.01, 0.01, 0.01);

    m_ScissorRect = CD3DX12_RECT(0, 0, LONG_MAX, LONG_MAX);

    mMaterial.AlphaThreshold = 0.1;
    mMaterialCBV = Buffer(sizeof(Material) / sizeof(float), sizeof(float), &mMaterial);
}

void ModelViewer::Cleanup(void)
{

}

void ModelViewer::OnMouseMoved(int aDeltaX, int aDeltaY)
{
    if (mIsFirstMouseMove)
    {
        mLastDeltaX = aDeltaX;
        mLastDeltaY = aDeltaY;
        mIsFirstMouseMove = false;
    }

    float offsetX = aDeltaX - mLastDeltaX;
    float offsetY = mLastDeltaY - aDeltaY;

    mLastDeltaX = aDeltaX;
    mLastDeltaY = aDeltaY;

    mCamera.processMouseMovement(offsetX, offsetY);
}

void ModelViewer::OnKeyEvent(const KeyEvent& keyEvent)
{
    if (keyEvent.State == KeyEvent::KeyState::Pressed)
    {
        switch (keyEvent.Key)
        {
        case KeyEvent::KeyCode::W:
            mCamera.processKeyboard(Camera::eMovementDirection::FORWARD, m_LastFrameTime);
            break;
        case KeyEvent::KeyCode::S:
            mCamera.processKeyboard(Camera::eMovementDirection::BACKWARD, m_LastFrameTime);
            break;
        case KeyEvent::KeyCode::A:
            mCamera.processKeyboard(Camera::eMovementDirection::LEFT, m_LastFrameTime);
            break;
        case KeyEvent::KeyCode::D:
            mCamera.processKeyboard(Camera::eMovementDirection::RIGHT, m_LastFrameTime);
            break;
        default:
            break;
        }
    }
}

void ModelViewer::Update(double deltaT)
{
    Utility::Printf("Frame time: %f, FPS: %f\n", deltaT, 1.0f/deltaT);

    m_LastFrameTime = deltaT;

    float aspectRatio = Graphics::g_DisplayWidth / static_cast<float>(Graphics::g_DisplayHeight);
    m_ProjectionMatrix = DirectX::XMMatrixPerspectiveFovLH(DirectX::XMConvertToRadians(Graphics::m_FoV), aspectRatio, 0.1f, 100.0f);

    m_Viewport = CD3DX12_VIEWPORT(0.0f, 0.0f, static_cast<float>(Graphics::g_DisplayWidth), static_cast<float>(Graphics::g_DisplayHeight));
}

void ModelViewer::RenderScene(void)
{
    /*
    Microsoft::WRL::ComPtr<ID3D12CommandAllocator> commandAllocator = Graphics::g_CommandAllocators[Graphics::g_CurrentBackBufferIndex];
    Microsoft::WRL::ComPtr<ID3D12Resource> backBuffer = Graphics::g_BackBuffers[Graphics::g_CurrentBackBufferIndex];

    commandAllocator->Reset();
    Graphics::g_CommandList->Reset(commandAllocator.Get(), nullptr);

#if _DEBUG
    //PIXBeginEvent(Graphics::g_CommandList.Get(), 0, "Begin");
#endif

    CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(backBuffer.Get(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
    Graphics::g_CommandList->ResourceBarrier(1, &barrier);

    FLOAT clearColor[] = {0.4f, 0.6f, 0.9f, 1.0f};
    CD3DX12_CPU_DESCRIPTOR_HANDLE rtv(Graphics::g_RTVDescriptorHeap->GetCPUDescriptorHandleForHeapStart(), Graphics::g_CurrentBackBufferIndex, Graphics::g_RtvDescriptorSize);
    Graphics::g_CommandList->ClearRenderTargetView(rtv, clearColor, 0, nullptr);

    barrier = CD3DX12_RESOURCE_BARRIER::Transition(backBuffer.Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
    Graphics::g_CommandList->ResourceBarrier(1, &barrier);

    Graphics::g_CommandList->Close();

#if _DEBUG
    //PIXEndEvent(Graphics::g_CommandList.Get());
#endif

    ID3D12CommandList* const commandLists[] = { Graphics::g_CommandList.Get() };
    Graphics::g_CommandQueue->ExecuteCommandLists(_countof(commandLists), commandLists);
    */

    Microsoft::WRL::ComPtr<ID3D12CommandAllocator> commandAllocator = Graphics::g_CommandAllocators[Graphics::g_CurrentBackBufferIndex];
    Microsoft::WRL::ComPtr<ID3D12Resource> backBuffer = Graphics::g_BackBuffers[Graphics::g_CurrentBackBufferIndex];

    commandAllocator->Reset();
    Graphics::g_CommandList->Reset(commandAllocator.Get(), nullptr);

#if _DEBUG
    //PIXBeginEvent(Graphics::g_CommandList.Get(), 0, "Begin");
#endif

    CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(backBuffer.Get(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
    Graphics::g_CommandList->ResourceBarrier(1, &barrier);

    FLOAT clearColor[] = { 0.5f, 0.5f, 0.5f, 1.0f };
    CD3DX12_CPU_DESCRIPTOR_HANDLE rtv(Graphics::g_RTVDescriptorHeap->GetCPUDescriptorHandleForHeapStart(), Graphics::g_CurrentBackBufferIndex, Graphics::g_RtvDescriptorSize);
    Graphics::g_CommandList->ClearRenderTargetView(rtv, clearColor, 0, nullptr);

    CD3DX12_CPU_DESCRIPTOR_HANDLE dsv(Graphics::g_DSVDescriptorHeap->GetCPUDescriptorHandleForHeapStart());
    Graphics::g_CommandList->ClearDepthStencilView(dsv, D3D12_CLEAR_FLAG_DEPTH, 1, 0, 0, nullptr);

    Graphics::g_CommandList->SetPipelineState(m_PipelineState.Get());
    Graphics::g_CommandList->SetGraphicsRootSignature(m_RootSignature.Get());
    Graphics::g_CommandList->RSSetViewports(1, &m_Viewport);
    Graphics::g_CommandList->RSSetScissorRects(1, &m_ScissorRect);
    Graphics::g_CommandList->OMSetRenderTargets(1, &rtv, FALSE, &dsv);

    Transform transform;
    transform.MV = XMMatrixMultiply(m_ModelMatrix, mCamera.getViewMatrix());
    transform.MVP = XMMatrixMultiply(transform.MV, m_ProjectionMatrix);
    Graphics::g_CommandList->SetGraphicsRoot32BitConstants(0, sizeof(Transform) / sizeof(float), &transform, 0);
    Graphics::g_CommandList->SetGraphicsRootConstantBufferView(2, mMaterialCBV->GetGPUVirtualAddress());

    m_Model.Render(Graphics::g_CommandList, 1);

    barrier = CD3DX12_RESOURCE_BARRIER::Transition(backBuffer.Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
    Graphics::g_CommandList->ResourceBarrier(1, &barrier);

    Graphics::g_CommandList->Close();

#if _DEBUG
    //PIXEndEvent(Graphics::g_CommandList.Get());
#endif

    ID3D12CommandList* const commandLists[] = { Graphics::g_CommandList.Get() };
    Graphics::g_CommandQueue->ExecuteCommandLists(_countof(commandLists), commandLists);
}
