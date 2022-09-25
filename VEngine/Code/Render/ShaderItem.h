#pragma once

#include <string>

class VertexShader;
class PixelShader;
struct ID3D11VertexShader;
struct ID3D11PixelShader;

//Helper structure when finding shaders from Shader System
class ShaderItem
{
public:
	ShaderItem(const std::string shaderItemName_,
		const std::wstring vertexShaderFilename_,
		const std::wstring pixelShaderFilename_);

	ID3D11VertexShader* GetVertexShader();
	ID3D11PixelShader* GetPixelShader();

	std::string GetName() { return shaderItemName; }

	std::wstring GetVertexShaderFilename() { return vertexShaderFilename; }
	std::wstring GetPixelShaderFilename() { return pixelShaderFilename; }

private:
	VertexShader* vertexShader = nullptr;
	PixelShader* pixelShader = nullptr;

	std::string shaderItemName;

	std::wstring vertexShaderFilename;
	std::wstring pixelShaderFilename;
};

struct ShaderItems
{
	inline static ShaderItem* Default;
	inline static ShaderItem* DefaultClip;
	inline static ShaderItem* Unlit;
	inline static ShaderItem* Animation;
	inline static ShaderItem* Shadow;
	inline static ShaderItem* Instance;
	inline static ShaderItem* SolidColour;
	inline static ShaderItem* UI;
	inline static ShaderItem* PostProcess;
};
