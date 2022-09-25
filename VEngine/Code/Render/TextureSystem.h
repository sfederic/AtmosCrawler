#pragma once
#include <unordered_map>
#include <string>
#include <memory>
#include "Render/Texture2D.h"
#include "System.h"

struct TextureSystem : System
{
	std::unordered_map<std::string, std::shared_ptr<Texture2D>> texture2DMap;

public:
	std::wstring selectedTextureInEditor;

	TextureSystem() : System("TextureSystem") {}
	void CreateAllTextures();
	void Cleanup();

	std::shared_ptr<Texture2D> FindTexture2D(std::string textureFilename);
};

extern TextureSystem textureSystem;
