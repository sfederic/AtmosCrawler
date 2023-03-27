#pragma once

#include "../Actor.h"
#include "../ActorSystem.h"

struct BoxTriggerComponent;
struct MeshComponent;
class EnemyHealthWidget;
struct WidgetComponent;

class Enemy : public Actor
{
public:
	ACTOR_SYSTEM(Enemy);

	Enemy();
	virtual void Start() override;
	virtual void Tick(float deltaTime) override;
	virtual Properties GetProps() override;

	void InflictDamage(int damageAmount);

private:
	void PlayerEnteredAggroTrigger();

	//Trigger that shows the enemy's aggro field.
	BoxTriggerComponent* aggroTrigger = nullptr;

	MeshComponent* mesh = nullptr;

	WidgetComponent* healthWidget = nullptr;

	int healthPoints = 3;

	bool inCombat = false;
};
