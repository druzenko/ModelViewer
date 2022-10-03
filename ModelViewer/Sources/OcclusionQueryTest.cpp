#include "pch.h"
#include "Application.h"
#include "Model.h"
#include "Camera.h"

#if _DEBUG
#include "pix3.h"
#endif

#include "Utility.h"

class OcclusionQueryTest : public IApplication
{
    DirectX::XMMATRIX m_FarQuadModelMatrix;
    DirectX::XMMATRIX m_NearQuadModelMatrix;
    DirectX::XMMATRIX m_ProjectionMatrix;

    D3D12_VIEWPORT m_Viewport;
    D3D12_RECT m_ScissorRect;

    Camera mCamera;

    Microsoft::WRL::ComPtr<ID3D12RootSignature> m_RootSignature;
    Microsoft::WRL::ComPtr<ID3D12PipelineState> m_PipelineState;
    Microsoft::WRL::ComPtr<ID3D12PipelineState> m_QueryPipelineState;
    Buffer mQueryResultBuffer;

    Buffer mQuadsVertexBuffer;
    Buffer mQuadsIndexBuffer;
    D3D12_VERTEX_BUFFER_VIEW mQuadsVertexBufferView;
    D3D12_INDEX_BUFFER_VIEW mQuadsIndexBufferView;

    float m_LastFrameTime;
    float m_CameraSpeed = 10.0f;

    float mLastDeltaX = Graphics::g_DisplayWidth / 2;
    float mLastDeltaY = Graphics::g_DisplayWidth / 2;
    bool mIsFirstMouseMove = true;

public:

    OcclusionQueryTest();

    void Startup(void) override;
    void Cleanup(void) override;
    void Update(double deltaT) override;
    void RenderScene(void) override;
    void OnKeyEvent(const KeyEvent& keyEvent) override;
    void OnMouseMoved(int aDeltaX, int aDeltaY) override;
};

//CREATE_APPLICATION(OcclusionQueryTest)

OcclusionQueryTest::OcclusionQueryTest()
    : mCamera(DirectX::XMVectorSet(0, 0, -5, 1), DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 1.0f), gYaw, gPitch)
{

}

void OcclusionQueryTest::Startup(void)
{
                                            //POSITION              //COLOR
                                            //far quad
    std::array<float, 84> quadsVertices = { -0.5f, 0.5f, 0.5f,      1.0f, 1.0f, 1.0f, 1.0f,
                                            0.5f, 0.5f, 0.5f,       1.0f, 1.0f, 1.0f, 1.0f,
                                            0.5f, -0.5f, 0.5f,      1.0f, 1.0f, 1.0f, 1.0f,
                                            -0.5f, -0.5f, 0.5f,     1.0f, 1.0f, 1.0f, 1.0f,
                                            //near quad
                                            -0.95f, 0.95f, 0.0f,    1.0f, 0.0f, 0.0f, 0.65f,
                                            0.95f, 0.95f, 0.0f,     1.0f, 1.0f, 0.0f, 0.65f,
                                            0.95f, -0.95f, 0.0f,    1.0f, 1.0f, 0.0f, 0.65f,
                                            -0.95f, -0.95f, 0.0f,   1.0f, 0.0f, 0.0f, 0.65f,
                                            // //far quad bounding box used for occlusion query (offset slightly to avoid z-fighting)
                                            -0.5f, 0.5f, 0.4999f,      0.0f, 0.0f, 0.0f, 1.0f,
                                            0.5f, 0.5f, 0.4999f,       0.0f, 0.0f, 0.0f, 1.0f,
                                            0.5f, -0.5f, 0.4999f,      0.0f, 0.0f, 0.0f, 1.0f,
                                            -0.5f, -0.5f, 0.4999f,     0.0f, 0.0f, 0.0f, 1.0f
    };
    std::array<unsigned short, 6> quadsIndices = { 0, 1, 2, 2, 3, 0 };
    mQuadsVertexBuffer = Buffer(quadsVertices.size() / 7, sizeof(float) * 7, quadsVertices.data());
    mQuadsIndexBuffer = Buffer(quadsIndices.size(), sizeof(unsigned short), quadsIndices.data());

    mQuadsVertexBufferView.BufferLocation = mQuadsVertexBuffer.GetGpuVirtualAddress();
    mQuadsVertexBufferView.SizeInBytes = mQuadsVertexBuffer.GetSizeInBytes();
    mQuadsVertexBufferView.StrideInBytes = mQuadsVertexBuffer.GetElementSizeInBytes();

    mQuadsIndexBufferView.BufferLocation = mQuadsIndexBuffer.GetGpuVirtualAddress();
    mQuadsIndexBufferView.Format = DXGI_FORMAT_R16_UINT;
    mQuadsIndexBufferView.SizeInBytes = mQuadsIndexBuffer.GetSizeInBytes();


    mQueryResultBuffer = Buffer(1, 8, nullptr, D3D12_RESOURCE_STATE_PREDICATION);

    // Load the vertex shader.
    Microsoft::WRL::ComPtr<ID3DBlob> vertexShaderBlob;
    ASSERT_HRESULT(D3DReadFileToBlob(L"OcclusionQueryTestVS.cso", &vertexShaderBlob), "Vertex shader compilation failed");

    // Load the pixel shader.
    Microsoft::WRL::ComPtr<ID3DBlob> pixelShaderBlob;
    ASSERT_HRESULT(D3DReadFileToBlob(L"OcclusionQueryTestPS.cso", &pixelShaderBlob), "Pixel shader compilation failed");

    // Create the vertex input layout
    D3D12_INPUT_ELEMENT_DESC inputLayout[] = {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
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
    CD3DX12_ROOT_PARAMETER1 rootParameters[1];
    rootParameters[0].InitAsConstants(sizeof(DirectX::XMMATRIX) / sizeof(float), 0, 0, D3D12_SHADER_VISIBILITY_VERTEX);

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

    CD3DX12_BLEND_DESC blendDesc(D3D12_DEFAULT);
    blendDesc.RenderTarget[0] =
    {
           TRUE, FALSE,
           D3D12_BLEND_SRC_ALPHA, D3D12_BLEND_INV_SRC_ALPHA, D3D12_BLEND_OP_ADD,
           D3D12_BLEND_ONE, D3D12_BLEND_ZERO, D3D12_BLEND_OP_ADD,
           D3D12_LOGIC_OP_NOOP,
           D3D12_COLOR_WRITE_ENABLE_ALL,
    };

    struct PipelineStateStream
    {
        CD3DX12_PIPELINE_STATE_STREAM_ROOT_SIGNATURE pRootSignature;
        CD3DX12_PIPELINE_STATE_STREAM_INPUT_LAYOUT InputLayout;
        CD3DX12_PIPELINE_STATE_STREAM_PRIMITIVE_TOPOLOGY PrimitiveTopologyType;
        CD3DX12_PIPELINE_STATE_STREAM_VS VS;
        CD3DX12_PIPELINE_STATE_STREAM_PS PS;
        CD3DX12_PIPELINE_STATE_STREAM_DEPTH_STENCIL DSV;
        CD3DX12_PIPELINE_STATE_STREAM_DEPTH_STENCIL_FORMAT DSVFormat;
        CD3DX12_PIPELINE_STATE_STREAM_RENDER_TARGET_FORMATS RTVFormats;
        CD3DX12_PIPELINE_STATE_STREAM_BLEND_DESC Blend;
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
    pipelineStateStream.Blend = blendDesc;

    D3D12_PIPELINE_STATE_STREAM_DESC pipelineStateStreamDesc = {
        sizeof(PipelineStateStream), &pipelineStateStream
    };

    Microsoft::WRL::ComPtr<ID3D12Device2> device2;
    ASSERT_HRESULT(Graphics::g_Device.As<ID3D12Device2>(&device2), "Failed to receive ID3D12Device2 form ID3D12Device.");
    ASSERT_HRESULT(device2->CreatePipelineState(&pipelineStateStreamDesc, IID_PPV_ARGS(&m_PipelineState)), "Failed to create pipline state object.");

    pipelineStateStream.Blend.operator CD3DX12_BLEND_DESC& ().RenderTarget[0].RenderTargetWriteMask = 0;
    pipelineStateStream.DSV.operator CD3DX12_DEPTH_STENCIL_DESC& ().DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
    ASSERT_HRESULT(device2->CreatePipelineState(&pipelineStateStreamDesc, IID_PPV_ARGS(&m_QueryPipelineState)), "Failed to create pipline state object.");

    m_FarQuadModelMatrix = DirectX::XMMatrixIdentity();
    m_NearQuadModelMatrix = DirectX::XMMatrixIdentity();

    m_ScissorRect = CD3DX12_RECT(0, 0, LONG_MAX, LONG_MAX);
}

void OcclusionQueryTest::Cleanup(void)
{

}

void OcclusionQueryTest::OnMouseMoved(int aDeltaX, int aDeltaY)
{
    return;
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

void OcclusionQueryTest::OnKeyEvent(const KeyEvent& keyEvent)
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

void OcclusionQueryTest::Update(double deltaT)
{
    Utility::Printf("Frame time: %f, FPS: %f\n", deltaT, 1.0f / deltaT);

    m_LastFrameTime = deltaT;

    float aspectRatio = Graphics::g_DisplayWidth / static_cast<float>(Graphics::g_DisplayHeight);
    m_ProjectionMatrix = DirectX::XMMatrixPerspectiveFovLH(DirectX::XMConvertToRadians(Graphics::m_FoV), aspectRatio, 0.1f, 100.0f);

    m_Viewport = CD3DX12_VIEWPORT(0.0f, 0.0f, static_cast<float>(Graphics::g_DisplayWidth), static_cast<float>(Graphics::g_DisplayHeight));

    static float direction = 1.0f;

    m_NearQuadModelMatrix = DirectX::XMMatrixMultiply(m_NearQuadModelMatrix, DirectX::XMMatrixTranslation(direction * deltaT, 0.0f, 0.0f));

    if (m_NearQuadModelMatrix.r[3].m128_f32[0] >= 2.0f)
    {
        direction = -1.0f;
    }
    else if (m_NearQuadModelMatrix.r[3].m128_f32[0] <= -2.0f)
    {
        direction = 1.0f;
    }
}

void OcclusionQueryTest::RenderScene(void)
{
    Microsoft::WRL::ComPtr<ID3D12CommandAllocator> commandAllocator = Graphics::g_CommandAllocators[Graphics::g_CurrentBackBufferIndex];
    Microsoft::WRL::ComPtr<ID3D12Resource> backBuffer = Graphics::g_BackBuffers[Graphics::g_CurrentBackBufferIndex];

    commandAllocator->Reset();
    ASSERT_HRESULT(Graphics::g_CommandList->Reset(commandAllocator.Get(), m_PipelineState.Get()), "Error: ID3D12GraphicsCommandList::Reset");

    CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(backBuffer.Get(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
    Graphics::g_CommandList->ResourceBarrier(1, &barrier);

    FLOAT clearColor[] = { 0.0f, 0.0f, 0.9f, 1.0f };
    CD3DX12_CPU_DESCRIPTOR_HANDLE rtv(Graphics::g_RTVDescriptorHeap->GetCPUDescriptorHandleForHeapStart(), Graphics::g_CurrentBackBufferIndex, Graphics::g_RtvDescriptorSize);
    Graphics::g_CommandList->ClearRenderTargetView(rtv, clearColor, 0, nullptr);

    CD3DX12_CPU_DESCRIPTOR_HANDLE dsv(Graphics::g_DSVDescriptorHeap->GetCPUDescriptorHandleForHeapStart());
    Graphics::g_CommandList->ClearDepthStencilView(dsv, D3D12_CLEAR_FLAG_DEPTH, 1, 0, 0, nullptr);

    Graphics::g_CommandList->SetGraphicsRootSignature(m_RootSignature.Get());
    Graphics::g_CommandList->RSSetViewports(1, &m_Viewport);
    Graphics::g_CommandList->RSSetScissorRects(1, &m_ScissorRect);
    Graphics::g_CommandList->OMSetRenderTargets(1, &rtv, FALSE, &dsv);
    Graphics::g_CommandList->IASetVertexBuffers(0, 1, &mQuadsVertexBufferView);
    Graphics::g_CommandList->IASetIndexBuffer(&mQuadsIndexBufferView);
    Graphics::g_CommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    //Draw far quad
    DirectX::XMMATRIX FarMVP = XMMatrixMultiply(m_FarQuadModelMatrix, mCamera.getViewMatrix());
    FarMVP = XMMatrixMultiply(FarMVP, m_ProjectionMatrix);
    Graphics::g_CommandList->SetGraphicsRoot32BitConstants(0, sizeof(FarMVP) / sizeof(float), &FarMVP, 0);
    Graphics::g_CommandList->SetPredication(mQueryResultBuffer.GetResource(), 0, D3D12_PREDICATION_OP_EQUAL_ZERO);
    Graphics::g_CommandList->DrawIndexedInstanced(mQuadsIndexBufferView.SizeInBytes / sizeof(unsigned short), 1, 0, 0, 0);


    // Draw near quad
    DirectX::XMMATRIX NearMVP = XMMatrixMultiply(m_NearQuadModelMatrix, mCamera.getViewMatrix());
    NearMVP = XMMatrixMultiply(NearMVP, m_ProjectionMatrix);
    Graphics::g_CommandList->SetGraphicsRoot32BitConstants(0, sizeof(NearMVP) / sizeof(float), &NearMVP, 0);
    Graphics::g_CommandList->SetPredication(nullptr, 0, D3D12_PREDICATION_OP_EQUAL_ZERO);
    Graphics::g_CommandList->DrawIndexedInstanced(mQuadsIndexBufferView.SizeInBytes / sizeof(unsigned short), 1, 0, 4, 0);

    // Run the occlusion query with the bounding box quad.
    Graphics::g_CommandList->SetGraphicsRoot32BitConstants(0, sizeof(FarMVP) / sizeof(float), &FarMVP, 0);
    Graphics::g_CommandList->SetPipelineState(m_QueryPipelineState.Get());
    Graphics::g_CommandList->BeginQuery(Graphics::g_QueryOcclusionHeap.Get(), D3D12_QUERY_TYPE_BINARY_OCCLUSION, 0);
    Graphics::g_CommandList->DrawIndexedInstanced(mQuadsIndexBufferView.SizeInBytes / sizeof(unsigned short), 1, 0, 8, 0);
    Graphics::g_CommandList->EndQuery(Graphics::g_QueryOcclusionHeap.Get(), D3D12_QUERY_TYPE_BINARY_OCCLUSION, 0);

    // Resolve the occlusion query and store the results in the query result buffer
    // to be used on the subsequent frame.
    barrier = CD3DX12_RESOURCE_BARRIER::Transition(mQueryResultBuffer.GetResource(), D3D12_RESOURCE_STATE_PREDICATION, D3D12_RESOURCE_STATE_COPY_DEST);
    Graphics::g_CommandList->ResourceBarrier(1, &barrier);
    Graphics::g_CommandList->ResolveQueryData(Graphics::g_QueryOcclusionHeap.Get(), D3D12_QUERY_TYPE_BINARY_OCCLUSION, 0, 1, mQueryResultBuffer.GetResource(), 0);
    barrier = CD3DX12_RESOURCE_BARRIER::Transition(mQueryResultBuffer.GetResource(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PREDICATION);
    Graphics::g_CommandList->ResourceBarrier(1, &barrier);

    barrier = CD3DX12_RESOURCE_BARRIER::Transition(backBuffer.Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
    Graphics::g_CommandList->ResourceBarrier(1, &barrier);

    Graphics::g_CommandList->Close();

    ID3D12CommandList* const commandLists[] = { Graphics::g_CommandList.Get() };
    Graphics::g_CommandQueue->ExecuteCommandLists(_countof(commandLists), commandLists);
}
