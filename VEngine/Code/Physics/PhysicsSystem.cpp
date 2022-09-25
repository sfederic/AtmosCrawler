#include "vpch.h"
#include "PhysicsSystem.h"
#include <cassert>
#include <PxPhysicsAPI.h>
#include <characterkinematic/PxControllerManager.h>
#include "Components/CharacterControllerComponent.h"
#include "Components/MeshComponent.h"
#include "Components/DestructibleMeshComponent.h"
#include <PxRigidActor.h>
#include "World.h"
#include "Asset/FBXLoader.h"

PhysicsSystem physicsSystem;

PxDefaultAllocator allocator;

//Need to link against PhysXExtensions_static_64.lib for this one. Also it needs to be from the debug folder.
PxDefaultErrorCallback errorCallback;

PxFoundation* foundation = nullptr;
PxPhysics* physics = nullptr;
PxCooking* cooking = nullptr;
PxDefaultCpuDispatcher* dispatcher = nullptr;
PxScene* scene = nullptr;
PxMaterial* material = nullptr;
PxMaterial* destructibleMaterial = nullptr;
PxPvd* pvd = nullptr;
PxControllerManager* controllerManager = nullptr;

void PhysicsSystem::Init()
{
	foundation = PxCreateFoundation(PX_PHYSICS_VERSION, allocator, errorCallback);
	assert(foundation);

	//nvidia physx debugger setup (pass pvd into PxCreatePhysics())
	//pvd = PxCreatePvd(*foundation);
	//PxPvdTransport* transport = PxDefaultPvdSocketTransportCreate("127.0.0.1", 5425, 10);
	//pvd->connect(*transport, PxPvdInstrumentationFlag::eALL);

	physics = PxCreatePhysics(PX_PHYSICS_VERSION, *foundation, PxTolerancesScale(), true);
	assert(physics);

	dispatcher = PxDefaultCpuDispatcherCreate(2);
	assert(dispatcher);

	//Create cooking objects
	cooking = PxCreateCooking(PX_PHYSICS_VERSION, *foundation, PxCookingParams(PxTolerancesScale()));
	assert(cooking);

	//Create scene
	PxSceneDesc sceneDesc(physics->getTolerancesScale());
	sceneDesc.gravity = PxVec3(0.0f, -9.81f, 0.0f);
	sceneDesc.cpuDispatcher = dispatcher;
	sceneDesc.filterShader = PxDefaultSimulationFilterShader;
	scene = physics->createScene(sceneDesc);

	//Default material
	material = physics->createMaterial(0.5f, 0.5f, 0.f);

	//Destructible material
	destructibleMaterial = physics->createMaterial(0.f, 0.f, 0.f);
	
	//Player capsule controller
	controllerManager = PxCreateControllerManager(*scene);
	assert(controllerManager);
}

//Setup physics actors on gameplay start
void PhysicsSystem::Start()
{
	//@Todo: it feels like physics will break if the mesh isn't the root of the actor
	auto actors = World::GetAllActorsInWorld();
	for (auto actor : actors)
	{
		auto meshes = actor->GetComponentsOfType<MeshComponent>();
		for (auto mesh : meshes)
		{
			auto dMesh = dynamic_cast<DestructibleMeshComponent*>(mesh);
			if (dMesh)
			{
				physicsSystem.CreatePhysicsForDestructibleMesh(dMesh, actor);

				for (auto cell : dMesh->meshCells)
				{
					dMesh->RemoveChild(cell);
				}
			}
			else if(!mesh->skipPhysicsCreation)
			{
				//@Todo: I don't like doing this and reseting physicssystem on gameplay end. Feels expensive,
				//but it enables roughly changing actors between static and dynamic when editing.
				if (mesh->isStatic)
				{
					physicsSystem.CreatePhysicsActor(mesh, PhysicsType::Static, actor);
				}
				else
				{
					physicsSystem.CreatePhysicsActor(mesh, PhysicsType::Dynamic, actor);
				}
			}
		}
	}
}

void PhysicsSystem::Tick(float deltaTime)
{
	//PxScene::simulate() complains if deltaTime is 0 or negative
	if (deltaTime <= 0.f) return;

	scene->simulate(deltaTime);
	scene->fetchResults(true);
}

void PhysicsSystem::Cleanup()
{
	scene->release();
	dispatcher->release();
	physics->release();

	//debugger shutdown
	if (pvd)
	{
		PxPvdTransport* transport = pvd->getTransport();
		pvd->release();	
		pvd = nullptr;
		transport->release();
	}

	foundation->release();
}

void PhysicsSystem::Reset()
{
	for (auto& rigidActorIt : rigidActorMap)
	{
		rigidActorIt.second->release();
	}

	rigidActorMap.clear();
}

void PhysicsSystem::ReleasePhysicsActor(MeshComponent* mesh)
{
	auto rigid = rigidActorMap[mesh->uid];
	if (rigid)
	{
		rigid->release();
	}
	rigidActorMap.erase(mesh->uid);
}

void PhysicsSystem::CreatePhysicsActor(MeshComponent* mesh, PhysicsType type, Actor* actor)
{
	PxTransform pxTransform = {};
	Transform transform = mesh->transform;
	ActorToPhysxTransform(transform, pxTransform);

	PxRigidActor* rigidActor = nullptr;
	switch (type)
	{
	case PhysicsType::Static:
		rigidActor = physics->createRigidStatic(pxTransform);
		break;

	case PhysicsType::Dynamic:
		rigidActor = physics->createRigidDynamic(pxTransform);
		break;
	}

	//Set actor as user data
	assert(rigidActor);
	rigidActor->userData = actor;

	XMVECTOR extentsVector = XMLoadFloat3(&mesh->boundingBox.Extents) * mesh->GetScaleV();
	XMFLOAT3 extents;
	XMStoreFloat3(&extents, extentsVector);
	NormaliseExtents(extents.x, extents.y, extents.z);
	PxShape* box = physics->createShape(PxBoxGeometry(extents.x, extents.y, extents.z), *material);

	rigidActor->attachShape(*box);
	scene->addActor(*rigidActor);

	rigidActorMap.insert(std::make_pair(mesh->uid, rigidActor));
}

void PhysicsSystem::CreatePhysicsForDestructibleMesh(DestructibleMeshComponent* mesh, Actor* actor)
{
	for (auto cell : mesh->meshCells)
	{
		CreateConvexPhysicsMesh(cell, actor);
	}
}

void PhysicsSystem::CreateCharacterController(CharacterControllerComponent* characterControllerComponent)
{
	PxCapsuleControllerDesc desc = {};
	desc.height = characterControllerComponent->height;
	desc.radius = characterControllerComponent->radius;
	desc.stepOffset = 0.3f;
	desc.volumeGrowth = 1.9f;
	desc.slopeLimit = cosf(XMConvertToRadians(15.f));
	desc.nonWalkableMode = PxControllerNonWalkableMode::ePREVENT_CLIMBING_AND_FORCE_SLIDING;
	desc.contactOffset = 0.5f;
	desc.upDirection = PxVec3(0.f, 1.f, 0.f);
	desc.material = material;

	auto pos = characterControllerComponent->GetPosition();
	desc.position.x = pos.x;
	desc.position.y = pos.y;
	desc.position.z = pos.z;

	PxController* controller = controllerManager->createController(desc);
	assert(controller);

	characterControllerComponent->controller = controller;
}

//PhysX says cooking is fairly intensive with larger meshes
//https://gameworksdocs.nvidia.com/PhysX/4.1/documentation/physxguide/Manual/Geometry.html#convex-mesh-cooking
//@Todo: look into cooking it outside of runtime

//@Todo: destructible meshes using convex hulls are exploding out based on how close they're starting too each
//other. Find a way to make this look better, or tighten the convex for destructible cells.
void PhysicsSystem::CreateConvexPhysicsMesh(MeshComponent* mesh, Actor* actor)
{
	PxConvexMeshDesc convexDesc;
	convexDesc.points.count = mesh->meshDataProxy.vertices->size();
	convexDesc.points.stride = sizeof(PxVec3);
	convexDesc.points.data = mesh->meshDataProxy.vertices->data();
	convexDesc.flags = PxConvexFlag::eCOMPUTE_CONVEX;

	PxDefaultMemoryOutputStream buf;
	PxConvexMeshCookingResult::Enum result;
	if (!cooking->cookConvexMesh(convexDesc, buf, &result))
	{
		throw std::exception("PhysX cooking blew up.");
	}

	PxDefaultMemoryInputData input(buf.getData(), buf.getSize());
	PxConvexMesh* convexMesh = physics->createConvexMesh(input);

	PxTransform pxTransform = {};
	Transform transform = mesh->transform;
	ActorToPhysxTransform(transform, pxTransform);

	PxRigidActor* rigidActor = nullptr;
	rigidActor = physics->createRigidDynamic(pxTransform);
	assert(rigidActor);

	rigidActor->userData = actor;

	PxConvexMeshGeometry convexGeom(convexMesh);
	//This flag here is important. Convex hulls are too loose otherwise for DestructibleMesh cells.
	convexGeom.meshFlags = PxConvexMeshGeometryFlag::eTIGHT_BOUNDS;
	PxRigidActorExt::createExclusiveShape(*rigidActor, convexGeom, *destructibleMaterial);

	scene->addActor(*rigidActor);
	rigidActorMap.insert(std::make_pair(mesh->uid, rigidActor));
}

void PhysicsSystem::CreateConvexPhysicsMeshFromCollisionMesh(MeshComponent* mesh,
	Actor* actor, const std::string filename)
{
	auto collisionMesh = new MeshComponent();
	collisionMesh->transform = actor->GetTransform();
	//Set the UID to the actual mesh so that the physics actor is connected to the mesh, not the collision mesh.
	collisionMesh->uid = mesh->uid;

	FBXLoader::Import(filename, collisionMesh->meshDataProxy);

	CreateConvexPhysicsMesh(collisionMesh, actor);

	delete collisionMesh;
}

void PhysicsSystem::ActorToPhysxTransform(const Transform& actorTransform, PxTransform& pxTransform)
{
	pxTransform.p = PxVec3(actorTransform.position.x,
		actorTransform.position.y, actorTransform.position.z);

	pxTransform.q = PxQuat(actorTransform.rotation.x, actorTransform.rotation.y,
		actorTransform.rotation.z, actorTransform.rotation.w);
}

void PhysicsSystem::PhysxToActorTransform(Transform& actorTransform, const PxTransform& pxTransform)
{
	actorTransform.position = XMFLOAT3(pxTransform.p.x, pxTransform.p.y, pxTransform.p.z);
	actorTransform.rotation = XMFLOAT4(pxTransform.q.x, pxTransform.q.y, pxTransform.q.z, pxTransform.q.w);
}

void PhysicsSystem::GetTransformFromPhysicsActor(MeshComponent* mesh)
{
	auto rigid = rigidActorMap[mesh->uid];

	PxTransform pxTransform = rigid->getGlobalPose();
	Transform transform = mesh->transform;
	PhysxToActorTransform(transform, pxTransform);

	mesh->transform = transform;
	mesh->UpdateTransform();
}

//Extents can be 0 or less than because of the planes and walls, Physx wants extents above 0.
void PhysicsSystem::NormaliseExtents(float& x, float& y, float& z)
{
	if (x <= 0.f) x = 0.1f;
	if (y <= 0.f) y = 0.1f;
	if (z <= 0.f) z = 0.1f;
}

bool Physics::Raycast(XMFLOAT3 origin, XMFLOAT3 dir, float range, RaycastHit& hit)
{
	PxRaycastBuffer hitBuffer;
	PxVec3 pxOrigin(origin.x, origin.y, origin.z);
	PxVec3 pxDir(dir.x, dir.y, dir.z);

	if (scene->raycast(pxOrigin, pxDir, range, hitBuffer))
	{
		PxRaycastHit& block = hitBuffer.block;

		hit.hitActor = (Actor*)block.actor->userData;
		hit.distance = block.distance;
		hit.normal = XMFLOAT3(block.normal.x, block.normal.y, block.normal.z);
		hit.posiiton = XMFLOAT3(block.position.x, block.position.y, block.position.z);
		hit.uv = XMFLOAT2(block.u, block.v);

		return true;
	}

	return false;
}

PxVec3 Physics::Float3ToPxVec3(XMFLOAT3 float3)
{
	return PxVec3(float3.x, float3.y, float3.z);
}

XMFLOAT3 Physics::PxVec3ToFloat3(PxVec3 pxVec3)
{
	return XMFLOAT3(pxVec3.x, pxVec3.y, pxVec3.z);
}

bool Physics::BoxCast(XMFLOAT3 extents, XMFLOAT3 origin, XMFLOAT3 direction, float distance, RaycastHit& hit)
{
	PxVec3 pxExtents = Physics::Float3ToPxVec3(extents);
	PxVec3 pxDirection = Physics::Float3ToPxVec3(direction);

	//Use identity quaternion for sweep, won't need more than that for now.
	PxTransform pose(Physics::Float3ToPxVec3(origin));

	PxSweepBuffer sweepBuffer;
	if (scene->sweep(PxBoxGeometry(pxExtents), PxTransform(), pxDirection, distance, sweepBuffer))
	{
		PxSweepHit& block = sweepBuffer.block;
		
		hit.hitActor = (Actor*)block.actor->userData;
		hit.distance = block.distance;
		hit.normal = XMFLOAT3(block.normal.x, block.normal.y, block.normal.z);
		hit.posiiton = XMFLOAT3(block.position.x, block.position.y, block.position.z);
		hit.uv = XMFLOAT2(0.f, 0.f); //No uv coords for sweeps

		return true;
	}

	return false;
}
