#pragma once

#include <string>
#include <memory>
#include <DirectXMath.h>
#include "Properties.h"
#include "UID.h"
#include "VEnum.h"

using namespace DirectX;

class Texture2D;
struct Sampler;
struct RastState;
struct BlendState;
class VertexShader;
class PixelShader;
class ShaderItem;

//The data passed into a shader's constant buffer. Has to be seperate because of byte packing.
struct MaterialShaderData
{
	XMFLOAT4 ambient = XMFLOAT4(0.5f, 0.5f, 0.5f, 1.f);
	XMFLOAT4 emissive = XMFLOAT4(1.f, 1.f, 1.f, 1.f);
	XMFLOAT4 diffuse = XMFLOAT4(1.f, 1.f, 1.f, 1.f);
	XMFLOAT4 specular = XMFLOAT4(1.f, 1.f, 1.f, 1.f);
	XMFLOAT2 uvOffset = XMFLOAT2(0.f, 0.f);
	XMFLOAT2 uvScale = XMFLOAT2(1.f, 1.f);
	float uvRotation = 0.f;
	float smoothness = 0.f;
	float metallic = 0.f;
	int useTexture = true; //Keep in mind that bools in HLSL are 4 bytes
};

class Material
{
private:
	UID uid = 0;

public:
	std::shared_ptr<Texture2D> texture;
	std::shared_ptr<ShaderItem> shader;
	Sampler* sampler = nullptr;
	RastState* rastState = nullptr;
	BlendState* blendState = nullptr;

	TextureData textureData;
	ShaderData shaderData;

	XMFLOAT2 uvOffsetSpeed = XMFLOAT2(0.f, 0.f);
	float uvRotationSpeed = 0.f;

	MaterialShaderData materialShaderData;

	VEnum rastStateValue;
	VEnum blendStateValue;

public:
	Material(std::string textureFilename_, ShaderItem* shaderItem);

	virtual void Create();
	virtual void Destroy();

	virtual Properties GetProps();

	ID3D11VertexShader* GetVertexShader();
	ID3D11PixelShader* GetPixelShader();

	UID GetUID() { return uid; }
	void SetUID(UID uid_) { uid = uid_; }

	static void SetupBlendAndRastStateValues();
};
