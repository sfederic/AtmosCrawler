#include "vpch.h"
#include "TitleScreenWidget.h"
#include "Gameplay/GameUtils.h"
#include "Gameplay/GameInstance.h"

void TitleScreenWidget::Draw(float deltaTime)
{
	Text(L"Vagrant Tactics", AlignLayout(300.f, 100.f, Align::Center), TextAlign::Center, Colours::White);

	if (Button(L"Begin", AlignLayout(100.f, 50.f, Align::BottomLeft), 1.f, TextAlign::Center, Colours::White))
	{
		GameUtils::LoadWorld("church_hall.vmap");
	}

	if (Button(L"Continue", AlignLayout(100.f, 50.f, Align::BottomRight), 1.f, TextAlign::Center, Colours::White))
	{
		GameInstance::useGameSaves = true;
		GameUtils::LoadGameInstanceData();
		GameUtils::LoadWorld(GameInstance::mapToLoadOnContinue);
	}
}
