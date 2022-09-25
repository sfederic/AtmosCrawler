#include "vpch.h"
#include "MeshComponent.h"
#include "Asset/FBXLoader.h"
#include "Render/RenderUtils.h"
#include "Render/ShaderSystem.h"
#include "Render/Material.h"
#include "Render/MaterialSystem.h"
#include "Render/Renderer.h"
#include "Render/TextureSystem.h"
#include "Physics/PhysicsSystem.h"
#include "VMath.h"
#include "Camera.h"
#include "Gameplay/GameUtils.h"
#include "Actors/Game/Player.h"
#include "Render/VertexShader.h"
#include "Render/PixelShader.h"

//Vertex and index buffers linked to a mesh filename to copy over to new PSOs
std::unordered_map<std::string, std::unique_ptr<MeshBuffers>> existingMeshBuffers;

void MeshComponent::ResetMeshBuffers()
{
	existingMeshBuffers.clear();
}

MeshComponent::MeshComponent()
{
	material = materialSystem.CreateMaterial("test.png", ShaderItems::Default);
}

MeshComponent::MeshComponent(const std::string filename_,
	const std::string textureFilename_,
	ShaderItem* shaderItem)
{
	meshComponentData.filename = filename_;
	material = new Material(textureFilename_, shaderItem);
}

void MeshComponent::Tick(float deltaTime)
{
	if (!isStatic)
	{
		physicsSystem.GetTransformFromPhysicsActor(this);
	}
}

void MeshComponent::Create()
{
	//Set 'this' so mesh widgets can access this component.
	meshComponentData.meshComponent = this;

	//Material's create needs to be called here to deal with serialisation
	material->Create();

	//Import mesh (bounds extents set up in here too)
	FBXLoader::Import(meshComponentData.filename.c_str(), meshDataProxy);

	//Setup bounds
	BoundingOrientedBox::CreateFromBoundingBox(boundingBox, *meshDataProxy.boundingBox);

	//Setup pipeline objects
	auto psoIt = existingMeshBuffers.find(meshComponentData.filename);
	if (psoIt == existingMeshBuffers.end())
	{
		existingMeshBuffers[meshComponentData.filename] = std::make_unique<MeshBuffers>();
		auto& meshBuffers = existingMeshBuffers[meshComponentData.filename];

		meshBuffers->vertexBuffer.data = RenderUtils::CreateVertexBuffer(meshDataProxy);
		meshBuffers->indexBuffer.data = RenderUtils::CreateIndexBuffer(meshDataProxy);

		pso.vertexBuffer = &meshBuffers->vertexBuffer;
		pso.indexBuffer = &meshBuffers->indexBuffer;
	}
	else
	{
		pso.vertexBuffer = &psoIt->second->vertexBuffer;
		pso.indexBuffer = &psoIt->second->indexBuffer;
	}
}

void MeshComponent::Destroy()
{
	//Erase physics actor
	physicsSystem.ReleasePhysicsActor(this);

	material->Destroy();
	material = nullptr;
}

static void ReassignMesh(void* data)
{
	auto meshData = (MeshComponentData*)data;
	meshData->meshComponent->Create();
}

Properties MeshComponent::GetProps()
{
	auto props = __super::GetProps();
	props.title = "MeshComponent";
	props.Add("Mesh", &meshComponentData).change = ReassignMesh;
	props.Add("Casts Shadow", &castsShadow);
	props.Add("Static", &isStatic);
	props.Add("Grid Obstacle", &gridObstacle);
	props.Merge(material->GetProps());
	return props;
}

void MeshComponent::SetRastState(const std::string newRastStateName)
{
	//If calling before Material::Create() is called, rastStateName is the one.
	material->rastStateValue.SetValue(newRastStateName);
	//This set isn't necessary with the above, but fine for after Material::Create() called.
	material->rastState = Renderer::GetRastState(newRastStateName);
}

void MeshComponent::SetBlendState(const std::string newBlendState)
{
	material->blendState = Renderer::GetBlendState(newBlendState);
}

void MeshComponent::SetTexture(const std::string newTextureName)
{
	material->texture = textureSystem.FindTexture2D(newTextureName);
	material->textureData.filename = newTextureName;
}

void MeshComponent::SetShaderFilenames(ShaderItem& shaderItem)
{
	material->shaderData.shaderItemName = shaderItem.GetName();
}

Buffer* MeshComponent::GetVertexBuffer() const 
{
	return pso.vertexBuffer;
}

Buffer* MeshComponent::GetIndexBuffer() const
{
	return pso.indexBuffer;
}
