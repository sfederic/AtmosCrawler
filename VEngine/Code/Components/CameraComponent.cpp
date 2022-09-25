#include "vpch.h"
#include "CameraComponent.h"
#include <algorithm>
#include "VMath.h"
#include "Core.h"
#include "Render/Renderer.h"
#include "Editor/Editor.h"

CameraComponent::CameraComponent()
{
	upViewVector = VMath::GlobalUpVector();
}

CameraComponent::CameraComponent(XMFLOAT3 startPos)
{
	upViewVector = VMath::GlobalUpVector();
	UpdateTransform();
}

void CameraComponent::Tick(float deltaTime)
{
	const int x = editor->centerOffsetX;
	const int y = editor->centerOffsetY;

	const float dx = -XMConvertToRadians(0.25f * (float)x);
	const float dy = -XMConvertToRadians(0.25f * (float)y);

	Pitch(dy);
	RotateY(dx);
}

XMMATRIX CameraComponent::GetViewMatrix()
{
	XMVECTOR worldPos = GetWorldPositionV();

	if (targetActor)
	{
		focusPoint = targetActor->GetPositionV() + targetActor->GetForwardVectorV();
	}
	else
	{
		focusPoint = worldPos + GetForwardVectorV();
	}

	XMMATRIX view = XMMatrixLookAtLH(worldPos, focusPoint, upViewVector);

	//Camera translation shaking
	XMVECTOR shakeVector = Shake();
	view.r[3] += shakeVector;

	return view;
}

XMMATRIX CameraComponent::GetProjectionMatrix()
{
	const float FOVRadian = XMConvertToRadians(FOV);
	return XMMatrixPerspectiveFovLH(FOVRadian, Renderer::GetAspectRatio(), nearZ, farZ);
}

void CameraComponent::Pitch(float angle)
{
	//The RightFromQuat is important here as GetRightVector() grabs the global directional vector, 
	//meaning the wall crawling mechanic would mess up FPS controls.
	const XMMATRIX r = XMMatrixRotationAxis(VMath::RightFromQuat(GetRotationV()), angle);
	XMVECTOR q = XMQuaternionMultiply(GetRotationV(), XMQuaternionRotationMatrix(r));

	float roll = 0.f, pitch = 0.f, yaw = 0.f;
	VMath::PitchYawRollFromQuaternion(roll, pitch, yaw, q);
	pitch = XMConvertToDegrees(pitch);
	if (pitch > 80.f || pitch < -80.f) 
	{
		return; 
	}

	SetRotation(q);
}

void CameraComponent::RotateY(float angle)
{
	const XMMATRIX r = XMMatrixRotationY(angle);
	const XMVECTOR q = XMQuaternionMultiply(GetRotationV(), XMQuaternionRotationMatrix(r));
	SetRotation(q);
}

void CameraComponent::Move(float d, XMVECTOR axis)
{
	const XMVECTOR s = XMVectorReplicate(d);
	XMVECTOR position = GetPositionV();
	position = XMVectorMultiplyAdd(s, axis, position);
	SetPosition(position);
}

void CameraComponent::ZoomTo(Actor* actor)
{
	XMVECTOR forward = GetForwardVectorV();

	//Trace the camera down the line its pointing towards the actor
	XMVECTOR actorPos = actor->GetPositionV();
	XMVECTOR zoomPos = actorPos - (forward * 5.f);

	SetPosition(zoomPos);
}

//Only works with translation for now.
//Ref: https://gdcvault.com/play/1023146/Math-for-Game-Programmers-Juicing
//@Todo: there's a lot more to do here to give the camera shake a better falloff. Look at ref for ideas.
XMVECTOR CameraComponent::Shake()
{
	if (shakeLevel <= 0.f)
	{
		return XMVectorZero();
	}

	shakeLevel -= Core::GetDeltaTime();

	const float range = VMath::RandomRange(-1.f, 1.f);

	const float maxShake = 0.1f;
	const float xOffset = maxShake * shakeLevel * range;
	const float yOffset = maxShake * shakeLevel * range;
	const float zOffset = maxShake * shakeLevel * range;

	return XMVectorSet(xOffset, yOffset, zOffset, 0.f); //Make sure the w here is 0
}

Properties CameraComponent::GetProps()
{
	auto props = __super::GetProps();
	props.title = "CameraComponent";
	props.Add("FOV", &FOV);
	props.Add("Near Z", &nearZ);
	props.Add("Far Z", &farZ);
	return props;
}
