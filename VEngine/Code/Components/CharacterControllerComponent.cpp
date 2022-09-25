#include "vpch.h"
#include "CharacterControllerComponent.h"
#include "Physics/PhysicsSystem.h"

CharacterControllerComponent::CharacterControllerComponent()
{
}

void CharacterControllerComponent::Start()
{
}

void CharacterControllerComponent::Tick(float deltaTime)
{
}

void CharacterControllerComponent::Create()
{
	physicsSystem.CreateCharacterController(this);
}

Properties CharacterControllerComponent::GetProps()
{
	auto props = __super::GetProps();
	props.title = "CharacterController";
	props.AddProp(radius);
	props.AddProp(height);
	return props;
}

void CharacterControllerComponent::Move(XMFLOAT3 displacement, float deltaTime)
{
	auto disp = Physics::Float3ToPxVec3(displacement);
	PxControllerFilters filters;
	controller->move(disp, 0.001f, deltaTime, filters);

	auto& controllerPos = controller->getPosition();
	SetPosition(controllerPos.x, controllerPos.y, controllerPos.z);
}
