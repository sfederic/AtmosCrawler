#pragma once
#include "Widget.h"

//Throws a fullscreen rect over the viewport and fades transparency.
//There's maybe a nicer way to do it with D3D/GL and RenderTargets or something.
struct ScreenFadeWidget : Widget
{
private:
	enum class FadeValue
	{
		FadeIn,
		FadeOut
	};

	float fadeValue = 0.f;

	FadeValue fade = FadeValue::FadeOut;

public:
	virtual void Draw(float deltaTime) override;
	void SetToFadeIn();
	void SetToFadeOut();
};
