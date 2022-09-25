#pragma once
#include <vector>
#include <DirectXCollision.h>
#include "Component.h"
#include "Transform.h"
#include "Physics/CollisionLayers.h"

using namespace DirectX;

//@Todo: there's a bit of opaque workings of World vs Local space with the transforms here.
//Basically every SetX() function is local and every SetWorldX() is global, meaning local is the default.
//Take a look at renaming things and making it make more sense.

struct SpatialComponent : Component
{
	Transform transform;
	BoundingOrientedBox boundingBox;
	SpatialComponent* parent = nullptr;
	std::vector<SpatialComponent*> children;

	CollisionLayers layer = CollisionLayers::All;

	Properties GetProps() override;

	void AddChild(SpatialComponent* component);
	void RemoveChild(SpatialComponent* component);
	XMMATRIX GetWorldMatrix();
	void UpdateTransform(XMMATRIX parentWorld = XMMatrixIdentity());

	XMFLOAT3 GetPosition();
	XMVECTOR GetPositionV();
	XMVECTOR GetWorldPositionV();
	void SetPosition(float x, float y, float z);
	void SetPosition(XMFLOAT3 newPosition);
	void SetPosition(XMVECTOR newPosition);
	void SetWorldPosition(XMVECTOR position);

	XMFLOAT3 GetScale();
	XMVECTOR GetScaleV();
	void SetScale(float x, float y, float z);
	void SetScale(XMFLOAT3 newScale);
	void SetScale(XMVECTOR newScale);
	void SetWorldScale(XMVECTOR scale);

	XMFLOAT4 GetRotation();
	XMVECTOR GetRotationV();
	void SetRotation(float x, float y, float z, float w);
	void SetRotation(XMFLOAT4 newRotation);
	void SetRotation(XMVECTOR newRotation);
	void SetWorldRotation(XMVECTOR newRotation);

	XMFLOAT3 GetForwardVector();
	XMVECTOR GetForwardVectorV();
	XMFLOAT3 GetRightVector();
	XMVECTOR GetRightVectorV();
	XMFLOAT3 GetUpVector();
	XMVECTOR GetUpVectorV();
};
