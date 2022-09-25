#pragma once
#include <string>

struct ID3D11Resource;
struct ID3D11ShaderResourceView;

class Texture2D
{
	std::string filename;

	ID3D11Resource* data = nullptr;
	ID3D11ShaderResourceView* srv = nullptr;

	uint32_t width = 0;
	uint32_t height = 0;

public:
	Texture2D() {}
	Texture2D(std::string filename_) : filename(filename_) {}
	~Texture2D();

	std::string GetFilename() { return filename; }
	void SetFilename(std::string filename_) { filename = filename_; }

	void SetTextureData(ID3D11Resource* data_) { data = data_; }
	ID3D11Resource* GetTextureData() { return data; }

	void SetSRV(ID3D11ShaderResourceView* srv_) { srv = srv_; }
	ID3D11ShaderResourceView* GetSRV() { return srv; }

	void SetWidth(uint32_t width_) { width = width_; }
	void SetHeight(uint32_t height_) { height = height_; }
	uint32_t GetWidth() { return width; }
	uint32_t GetHeight() { return height; }
};
