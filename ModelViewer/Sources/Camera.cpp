#include "pch.h"
#include "Camera.h"

#include "Camera.h"

Camera::Camera(DirectX::XMVECTOR aPosition, DirectX::XMVECTOR aUp, float aYaw, float aPitc)
	: mPosition(aPosition)
	, mFront(DirectX::XMVectorSet(0.0f, 0.0f, -1.0f, 1.0f))
	, mUp(aUp)
	, cWorldUp(aUp)
	, mYaw(aYaw)
	, mPitch(aPitc)
	, mSpeed(gSpeed)
	, mSensitivity(gSensitivity)
	, mZoom(gZoom)
{
	updateCameraVectors();
}

Camera::~Camera()
{

}

void Camera::updateCameraVectors()
{
	mFront.m128_f32[0] = cos(DirectX::XMConvertToRadians(mYaw)) * cos(DirectX::XMConvertToRadians(mPitch));
	mFront.m128_f32[1] = sin(DirectX::XMConvertToRadians(mPitch));
	mFront.m128_f32[2] = sin(DirectX::XMConvertToRadians(mYaw)) * cos(DirectX::XMConvertToRadians(mPitch));
	mFront = DirectX::XMVector3Normalize(mFront);

	mRight = DirectX::XMVector3Normalize(DirectX::XMVector3Cross(cWorldUp, mFront));
	mRight.m128_f32[3] = 1.0f;

	mUp = DirectX::XMVector3Normalize(DirectX::XMVector3Cross(mFront, mRight));
	mUp.m128_f32[3] = 1.0f;
}

DirectX::XMMATRIX Camera::getViewMatrix() const
{
	return DirectX::XMMatrixLookToLH(mPosition, mFront, mUp);
}

void Camera::processKeyboard(eMovementDirection aDirection, float aDeltaTime)
{
	float velocity = mSpeed * aDeltaTime;
	if (aDirection == eMovementDirection::FORWARD)
	{
		mPosition = DirectX::XMVectorAdd(mPosition, DirectX::XMVectorScale(mFront, velocity));
	}
	if (aDirection == eMovementDirection::BACKWARD)
	{
		mPosition = DirectX::XMVectorSubtract(mPosition, DirectX::XMVectorScale(mFront, velocity));
	}
	if (aDirection == eMovementDirection::LEFT)
	{
		mPosition = DirectX::XMVectorSubtract(mPosition, DirectX::XMVectorScale(mRight, velocity));
	}
	if (aDirection == eMovementDirection::RIGHT)
	{
		mPosition = DirectX::XMVectorAdd(mPosition, DirectX::XMVectorScale(mRight, velocity));
	}
}

void Camera::processMouseMovement(float aOffsetX, float aOffsetY)
{
	aOffsetX *= mSensitivity;
	aOffsetY *= mSensitivity;

	mYaw -= aOffsetX;
	mPitch += aOffsetY;

	if (mPitch > 89.0f)
	{
		mPitch = 89.0f;
	}
	else if (mPitch < -89.0f)
	{
		mPitch = -89.0f;
	}

	updateCameraVectors();
}

void Camera::processMouseScroll(float aOffsetY)
{
	mZoom -= aOffsetY;

	if (mZoom < 1.0f)
	{
		mZoom = 1.0f;
	}
	else if (mZoom > 45.0f)
	{
		mZoom = 45.0f;
	}
}

