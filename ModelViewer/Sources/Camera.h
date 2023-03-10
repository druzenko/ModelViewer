#pragma once

#include <DirectXMath.h>

constexpr float gYaw = 90.0f;
constexpr float gPitch = 0.0f;
constexpr float gSpeed = 1000.0f;
constexpr float gSensitivity = 0.1f;
constexpr float gZoom = 45.0f;

class Camera
{
public:

	enum class eMovementDirection
	{
		FORWARD = 0,
		BACKWARD,
		LEFT,
		RIGHT
	};

private:

	DirectX::XMVECTOR mPosition;
	DirectX::XMVECTOR mFront;
	DirectX::XMVECTOR mRight;
	DirectX::XMVECTOR mUp;
	const DirectX::XMVECTOR cWorldUp;
	float mYaw;
	float mPitch;
	float mSpeed;
	float mSensitivity;
	float mZoom;

	Camera() = delete;

	void updateCameraVectors();

public:

	Camera(DirectX::XMVECTOR aPosition = DirectX::XMVectorSet(0.0f, 0.0f, 0.0f, 1.0f), DirectX::XMVECTOR aUp = DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 1.0f), float aYaw = gYaw, float aPitc = gPitch);
	~Camera();

	DirectX::XMMATRIX getViewMatrix() const;
	void processKeyboard(eMovementDirection aDirection, float aDeltaTime);
	void processMouseMovement(float aOffsetX, float aOffsetY);
	void processMouseScroll(float aOffsetY);
	float getZoom() const { return mZoom; }
	const DirectX::XMVECTOR& getPosition() const { return mPosition; }
	const DirectX::XMVECTOR& getDirection() const { return mFront; }
};

