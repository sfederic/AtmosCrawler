#pragma once
#include "Actor.h"
#include "ActorSystem.h"

struct DestructibleMeshComponent;

//@Todo: because this the destructible component uses convex bounds, you can't
//click on this actor in the editor, or delete it. To fix this, you'd have to
//move raycasts and collision completely to PhysX.
struct DestructibleActor : Actor
{
	ACTOR_SYSTEM(DestructibleActor);

	DestructibleMeshComponent* destructibleMesh = nullptr;

	DestructibleActor();
	void Start() override;
	void Tick(float deltaTime) override;
	Properties GetProps() override;
};
