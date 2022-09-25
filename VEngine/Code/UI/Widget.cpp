#include "vpch.h"
#include "Widget.h"
#include <WICTextureLoader.h>
#include "Input.h"
#include "Render/Renderer.h"
#include "Editor/Editor.h"
#include "Render/SpriteSystem.h"
#include "Timer.h"

void Widget::Destroy()
{
	RemoveFromViewport();
	uiSystem.DestroyWidget(this);
	delete this;
}

void Widget::AddToViewport(float removeTimer)
{
	uiSystem.AddWidget(this);

	if (removeTimer > 0.f)
	{
		Timer::SetTimer(removeTimer, std::bind(&Widget::RemoveFromViewport, this));
	}
}

void Widget::RemoveFromViewport()
{
	uiSystem.RemoveWidget(this);
}

bool Widget::IsInViewport()
{
	for (Widget* widget : uiSystem.widgetsInViewport)
	{
		if (widget == this)
		{
			return true;
		}
	}

	return false;
}

void Widget::GetScreenSpaceCoords(int& sx, int& sy)
{
	//What you need to do here it take the actor's position after it's been multiplied 
	//by the MVP matrix on the CPU side of things, divide it by the W component 
	//and multiply it out by the viewport.
	//REF:http://www.windows-tech.info/5/a80747e145dd9062.php

	const float f1 = pos.m128_f32[0] / pos.m128_f32[3];
	const float f2 = pos.m128_f32[1] / pos.m128_f32[3];

	sx = ((f1 * 0.5f) + 0.5) * Renderer::GetViewportWidth();
	sy = ((f2 * -0.5f) + 0.5) * Renderer::GetViewportHeight();
}

void Widget::Text(const std::wstring& text, Layout layout, TextAlign align,
	D2D1_COLOR_F color, float opacity)
{
	DWRITE_TEXT_ALIGNMENT endAlignment{};

	switch (align)
	{
	case TextAlign::Center: 
		endAlignment = DWRITE_TEXT_ALIGNMENT_CENTER;
		break;

	case TextAlign::Justified:
		endAlignment = DWRITE_TEXT_ALIGNMENT_JUSTIFIED;
		break;

	case TextAlign::Leading:
		endAlignment = DWRITE_TEXT_ALIGNMENT_LEADING;
		break;

	case TextAlign::Trailing:
		endAlignment = DWRITE_TEXT_ALIGNMENT_TRAILING;
		break;
	}

	uiSystem.textFormat->SetTextAlignment(endAlignment);

	uiSystem.brushText->SetColor(color);
	uiSystem.brushText->SetOpacity(opacity);

	uiSystem.d2dRenderTarget->DrawText(text.c_str(), text.size(),
		uiSystem.textFormat, layout.rect, uiSystem.brushText);
}

bool Widget::Button(const std::wstring& text, Layout layout, float lineWidth,
	TextAlign textAlign, D2D1_COLOR_F textColor, float textOpacity)
{
	uiSystem.d2dRenderTarget->DrawRectangle(layout.rect, uiSystem.brushShapes, lineWidth);
	Text(text, layout.rect, textAlign, textColor, textOpacity);

	if (editor->viewportMouseX > layout.rect.left && editor->viewportMouseX < layout.rect.right)
	{
		if (editor->viewportMouseY > layout.rect.top && editor->viewportMouseY < layout.rect.bottom)
		{
			//Hover animation/image
			uiSystem.d2dRenderTarget->DrawRectangle(layout.rect, uiSystem.brushText, lineWidth * 2.f);

			if (Input::GetMouseLeftUp())
			{
				return true;
			}
		}
	}

	return false;
}

void Widget::Image(const std::string& filename, Layout layout)
{
	Sprite sprite = {};
	sprite.textureFilename = filename;
	sprite.dstRect = { (long)layout.rect.left, (long)layout.rect.top, (long)layout.rect.right, (long)layout.rect.bottom };
	sprite.srcRect = { 0, 0, (long)layout.rect.right, (long)layout.rect.bottom };

	spriteSystem.CreateScreenSprite(sprite);
}

void Widget::Image(const std::string& filename, int x, int y, int w, int h)
{
	Sprite sprite = {};
	sprite.textureFilename = filename;
	sprite.dstRect = { x, y, x + w, y + h };
	sprite.srcRect = { 0, 0, w, h };

	spriteSystem.CreateScreenSprite(sprite);
}

bool Widget::ImageButton(const std::string& filename, Layout layout)
{
	Sprite sprite = {};
	sprite.textureFilename = filename;
	sprite.dstRect = { (long)layout.rect.left, (long)layout.rect.top, (long)layout.rect.right, (long)layout.rect.bottom };
	sprite.srcRect = { 0, 0, (long)layout.rect.right, (long)layout.rect.bottom };

	spriteSystem.CreateScreenSprite(sprite);

	if (editor->viewportMouseX > layout.rect.left && editor->viewportMouseX < layout.rect.right)
	{
		if (editor->viewportMouseY > layout.rect.top && editor->viewportMouseY < layout.rect.bottom)
		{
			//Hover animation/image
			uiSystem.d2dRenderTarget->DrawRectangle(layout.rect, uiSystem.brushText, 2.f);

			if (Input::GetMouseLeftUp())
			{
				return true;
			}
		}
	}

	return false;
}

void Widget::Rect(Layout layout)
{
	uiSystem.d2dRenderTarget->DrawRectangle(layout.rect, uiSystem.brushShapes);
}

void Widget::FillRect(Layout layout, D2D1_COLOR_F color, float opacity)
{
	uiSystem.brushShapesAlpha->SetColor(color);
	uiSystem.brushShapesAlpha->SetOpacity(opacity);

	uiSystem.d2dRenderTarget->FillRectangle(layout.rect, uiSystem.brushShapesAlpha);
}

Layout Widget::AlignLayout(float w, float h, Align align)
{
	float vw = Renderer::GetViewportWidth();
	float vh = Renderer::GetViewportHeight();

	switch (align)
	{
	case Align::Center: 
		vw /= 2.f;
		vh /= 2.f;
		break;

	case Align::Right:
		vw *= 0.75;
		vh /= 2.f;
		break;

	case Align::Left:
		vw *= 0.25f;
		vh /= 2.f;
		break;

	case Align::Top:
		vw /= 2.f;
		vh *= 0.25f;
		break;

	case Align::Bottom:
		vw /= 2.f;
		vh *= 0.75f;
		break;

	case Align::TopLeft:
		vw *= 0.25f;
		vh *= 0.25f;
		break;

	case Align::TopRight:
		vw *= 0.75f;
		vh *= 0.25f;
		break;

	case Align::BottomLeft:
		vw *= 0.25f;
		vh *= 0.75f;
		break;

	case Align::BottomRight:
		vw *= 0.75f;
		vh *= 0.75f;
		break;
	}

	D2D1_RECT_F rect = { vw - w, vh - h, vw + w, vh + h };

	if (rect.left < 0.f) rect.left = 0.f;
	if (rect.top < 0.f) rect.top = 0.f;
	if (rect.right > Renderer::GetViewportWidth()) rect.right = vw;
	if (rect.bottom > Renderer::GetViewportHeight()) rect.bottom = vh;

	Layout layout = {};
	layout.rect = rect;

	return layout;
}

Layout Widget::PercentAlignLayout(float left, float top, float right, float bottom)
{
	float vw = Renderer::GetViewportWidth();
	float vh = Renderer::GetViewportHeight();

	float endLeft = vw * left;
	float endTop = vh * top;
	float endRight = vw * right;
	float endBottom = vh * bottom;
	D2D1_RECT_F rect = { endLeft, endTop, endRight, endBottom };

	if (rect.left < 0.f) rect.left = 0.f;
	if (rect.top < 0.f) rect.top = 0.f;
	if (rect.right > Renderer::GetViewportWidth()) rect.right = vw;
	if (rect.bottom > Renderer::GetViewportHeight()) rect.bottom = vh;

	return rect;
}

Layout Widget::CenterLayoutOnScreenSpaceCoords(float w, float h)
{
	int sx = 0, sy = 0;
	GetScreenSpaceCoords(sx, sy);

	D2D1_RECT_F rect = {sx - w, sy - h, sx + w, sy + h};

	float vw = Renderer::GetViewportWidth();
	float vh = Renderer::GetViewportHeight();

	if (rect.left < 0.f) rect.left = 0.f;
	if (rect.top < 0.f) rect.top = 0.f;
	if (rect.right > vw) rect.right = vw;
	if (rect.bottom > vh) rect.bottom = vh;

	return Layout(rect);
}
