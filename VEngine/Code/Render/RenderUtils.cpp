#include "vpch.h"
#include "RenderUtils.h"
#include "RenderTypes.h"
#include "Texture2D.h"
#include "PipelineObjects.h"
#include "Debug.h"
#include <WICTextureLoader.h>
#include "TextureSystem.h"
#include <filesystem>
#include "VString.h"
#include "Asset/AssetPaths.h"

namespace RenderUtils
{
	ID3D11Device* device;
	ID3D11DeviceContext* context;
	Sampler* defaultSampler;

	ID3D11Buffer* CreateDefaultBuffer(uint32_t byteWidth, uint32_t bindFlags, const void* initData)
	{
		ID3D11Buffer* buffer;

		D3D11_BUFFER_DESC desc = {};
		desc.ByteWidth = byteWidth;
		desc.BindFlags = bindFlags;
		desc.Usage = D3D11_USAGE_DEFAULT;

		D3D11_SUBRESOURCE_DATA data = {};
		data.pSysMem = initData;

		HR(device->CreateBuffer(&desc, &data, &buffer));

		return buffer;
	}

	ID3D11Buffer* CreateDynamicBuffer(uint32_t byteWidth, uint32_t bindFlags, const void* initData)
	{
		ID3D11Buffer* buffer;

		D3D11_BUFFER_DESC desc = {};
		desc.ByteWidth = byteWidth;
		desc.BindFlags = bindFlags;
		desc.Usage = D3D11_USAGE_DYNAMIC;
		desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

		D3D11_SUBRESOURCE_DATA data = {};
		data.pSysMem = initData;

		HR(device->CreateBuffer(&desc, &data, &buffer));
		assert(buffer);

		return buffer;
	}

	ID3D11Buffer* CreateVertexBuffer(MeshDataProxy& meshData)
	{
		return CreateDefaultBuffer(meshData.GetVerticesByteWidth(),
			D3D11_BIND_VERTEX_BUFFER, meshData.vertices->data());
	}

	ID3D11Buffer* CreateIndexBuffer(MeshDataProxy& meshData)
	{
		return CreateDefaultBuffer(meshData.GetIndicesByteWidth(),
			D3D11_BIND_INDEX_BUFFER, meshData.indices->data());
	}

	ID3D11ShaderResourceView* CreateSRVForMeshInstance(ID3D11Buffer* structuredBuffer, uint32_t numBufferElements)
	{
		D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
		srvDesc.Format = DXGI_FORMAT_UNKNOWN;
		srvDesc.ViewDimension = D3D11_SRV_DIMENSION_BUFFEREX;
		srvDesc.BufferEx.NumElements = numBufferElements;

		ID3D11ShaderResourceView* srv = nullptr;
		HR(device->CreateShaderResourceView(structuredBuffer, &srvDesc, &srv));

		return srv;
	}

	ID3D11Buffer* CreateStructuredBuffer(uint32_t byteWidth, uint32_t byteStride, const void* initData)
	{
		ID3D11Buffer* buffer;

		D3D11_BUFFER_DESC desc = {};
		desc.ByteWidth = byteWidth;
		desc.Usage = D3D11_USAGE_DYNAMIC;
		desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
		desc.StructureByteStride = byteStride;
		desc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
		desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

		D3D11_SUBRESOURCE_DATA data = {};
		data.pSysMem = initData;

		HR(device->CreateBuffer(&desc, &data, &buffer));

		return buffer;
	}

	Sampler* CreateSampler()
	{
		D3D11_SAMPLER_DESC sampDesc = {};
		sampDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
		sampDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
		sampDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
		sampDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;

		ID3D11SamplerState* samplerState;
		HR(device->CreateSamplerState(&sampDesc, &samplerState));

		auto sampler = new Sampler(samplerState);
		return sampler;
	}

	Sampler* GetDefaultSampler()
	{
		assert(defaultSampler);
		return defaultSampler;
	}

	void CreateTexture(Texture2D* texture)
	{
		std::wstring path;

		if (texture->GetFilename().empty())
		{
			//Set default texture if filename empty
			texture->SetFilename("test.png");
			path = VString::stows(AssetBaseFolders::texture + texture->GetFilename());
		}
		else
		{
			path = VString::stows(AssetBaseFolders::texture + texture->GetFilename());
		}

		assert(std::filesystem::exists(path) && "Texture file doesn't exist");

		ID3D11Resource* resource = nullptr;
		ID3D11ShaderResourceView* srv = nullptr;
		HR(CreateWICTextureFromFile(device, path.c_str(), &resource, &srv));
		assert(resource);
		assert(srv);

		texture->SetTextureData(resource);
		texture->SetSRV(srv);

		//CreateWICTextureFromFile() doesn't like ID3D11Texture2D, so casting down here
		//to get the texture Desc.
		ID3D11Texture2D* textureResource = nullptr;
		HR(resource->QueryInterface(&textureResource));

		D3D11_TEXTURE2D_DESC texDesc = {};
		textureResource->GetDesc(&texDesc);

		texture->SetWidth(texDesc.Width);
		texture->SetHeight(texDesc.Height);
	}
}
