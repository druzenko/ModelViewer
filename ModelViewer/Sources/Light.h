#pragma once

#include <DirectXMath.h>

enum class LightType
{
    Point,
    Spot,
    Directional
};

struct Light
{
    DirectX::XMFLOAT3 PositionWS;
    DirectX::XMFLOAT3 DirectionWS;
    DirectX::XMFLOAT3 PositionVS;
    DirectX::XMFLOAT3 DirectionVS;
    DirectX::XMFLOAT4 Color;
    float SpotlightAngle = 0.0f;
    float Range = 0.0f;
    float Intensity = 0.0f;
    UINT32 Enabled = 1;
    UINT32 Selected = 0;
    UINT32 Type = 0;
};

namespace Lightning
{
    UINT64 GetLightsCount();
    void Startup();
    void Update(const DirectX::XMMATRIX& MV);
}