#pragma once
#include "Components/SpatialComponent.h"
#include "Components/ComponentSystem.h"
#include "Render/RenderTypes.h"

struct Buffer;
struct MeshDataProxy;

//@Todo: the main idea behind the Polyboard was to show an NPC's intent (both in combat and out) in the
//same way FFXII shows aggro from enemies and player NPCs while in combat. The code here is still extremely
//rough and needs work with how buffers are handled and vertices are generated smoothly.

//Taken from 'Mathematics for 3D Game Programming and Computer Graphics', Chapter 9.3.3: Polyboards.
//Polyboard is a line of 'planes' facing the camera for line-like effects (beams, lightning, etc.)
struct Polyboard : SpatialComponent
{
	COMPONENT_SYSTEM(Polyboard);

	Buffer* vertexBuffer = nullptr;
	Buffer* indexBuffer = nullptr;

	TextureData textureData;

	std::vector<Vertex> vertices;
	std::vector<MeshData::indexDataType> indices;

	XMFLOAT3 startPoint;
	XMFLOAT3 endPoint;

	float radius = 0.8f;

	Polyboard();
	virtual Properties GetProps() override;
	void GenerateVertices();
	void CalcVertices();
};
