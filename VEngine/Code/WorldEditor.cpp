#include "vpch.h"
#include "WorldEditor.h"
#include "Physics/Raycast.h"
#include "Input.h"
#include "Editor/Editor.h"
#include "Actors/Actor.h"
#include "Actors/IActorSystem.h"
#include "Editor/TransformGizmo.h"
#include "FileSystem.h"
#include "Camera.h"
#include "Actors/MeshActor.h"
#include "Editor/DebugMenu.h"
#include "Core.h"
#include "Asset/AssetPaths.h"
#include "Render/TextureSystem.h"
#include "Render/Material.h"
#include "Render/MaterialSystem.h"
#include "Components/MeshComponent.h"

std::set<Actor*> pickedActors;
Actor* pickedActor;
SpatialComponent* pickedComponent;
IActorSystem* spawnSystem;
std::string actorTemplateFilename;
WorldEditor::PickMode pickMode = WorldEditor::PickMode::Actor;

bool WorldEditor::texturePlacement = false;
bool WorldEditor::materialPlacement = false;

void HandleActorPicking();
void DuplicateActor();
void SaveWorld();
void DeleteActor();
void SpawnActorOnClick();
void SpawnActor(Transform& transform);

void WorldEditor::Tick()
{
	if (!debugMenu.hasMouseFocus)
	{
		SpawnActorOnClick();
		HandleActorPicking();
	}

	DuplicateActor();
	DeleteActor();

	SaveWorld();
}

void HandleActorPicking()
{
	if (transformGizmo.CheckMouseOver() || Core::gameplayOn) 
	{
		return;
	}

	if (Input::GetMouseLeftUp())
	{
		Ray screenPickRay;
		if (RaycastFromScreen(screenPickRay))
		{
			//Assign selected texture in editor to mesh on click
			if (!textureSystem.selectedTextureInEditor.empty() && WorldEditor::texturePlacement)
			{
				auto mesh = dynamic_cast<MeshComponent*>(screenPickRay.hitComponent);
				if (mesh)
				{
					mesh->material->materialShaderData.useTexture = true;
					mesh->SetTexture(VString::wstos(textureSystem.selectedTextureInEditor));
					return;
				}
			}

			//Assign selected material in editor to mesh on click
			if (!materialSystem.selectedMaterialInEditor.empty() && WorldEditor::materialPlacement)
			{
				auto mesh = dynamic_cast<MeshComponent*>(screenPickRay.hitComponent);
				if (mesh)
				{
					Material loadedMaterial = materialSystem.LoadMaterialFromFile(materialSystem.selectedMaterialInEditor);
					loadedMaterial.Create();
					*mesh->material = loadedMaterial;
					return;
				}
			}

			switch (pickMode)
			{
				case WorldEditor::PickMode::Actor:
				{
					if (Input::GetKeyHeld(Keys::Ctrl) || pickedActors.empty())
					{
						pickedActors.insert(screenPickRay.hitActor);
					}
					else
					{
						pickedActors.clear();
					}

					WorldEditor::SetPickedActor(screenPickRay.hitActor);
					break;
				}
				case WorldEditor::PickMode::Component:
				{
					WorldEditor::SetPickedComponent(screenPickRay.hitComponent);
					break;
				}
			}
		}
	}
}

void DuplicateActor()
{
	if (Input::GetKeyHeld(Keys::Ctrl))
	{
		if (Input::GetKeyDown(Keys::W))
		{
			if (pickedActor)
			{
				pickedActors.clear();

				editor->ClearProperties();

				Transform transform = pickedActor->GetTransform();
				Actor* newDuplicateActor = pickedActor->GetActorSystem()->SpawnActor(transform);

				//The props copying below will overwrite the new actor's name, so keep it here then copy it back.
				const std::string newActorOriginalName = newDuplicateActor->GetName();

				//Make a new UID for the actor
				const UID newActorOriginalUID = newDuplicateActor->GetUID();

				//Remove actor over UID and name conflicts, then back into world again later
				World::RemoveActorFromWorld(newActorOriginalName);

				//Copy values across
				auto oldProps = pickedActor->GetAllProps();
				auto newProps = newDuplicateActor->GetAllProps();
				Properties::CopyProperties(oldProps, newProps);

				newDuplicateActor->CreateAllComponents();

				newDuplicateActor->SimpleSetName(newActorOriginalName);
				newDuplicateActor->SetUID(newActorOriginalUID);

				newDuplicateActor->ResetOwnerUIDToComponents();

				World::AddActorToWorld(newDuplicateActor);

				editor->SetActorProps(newDuplicateActor);
				editor->UpdateWorldList();

				debugMenu.AddNotification(VString::wformat(
					L"Duplicated new actor [%S]",newActorOriginalName.c_str()));

				//Set new actor as picked in-editor
				pickedActor = newDuplicateActor;
			}
		}
	}
}

void SaveWorld()
{
	if (Input::GetKeyHeld(Keys::Ctrl))
	{
		if (Input::GetKeyDown(Keys::S))
		{
			FileSystem::SerialiseAllSystems();
		}
	}
}

void DeleteActor()
{
	if (Input::GetKeyUp(Keys::Delete) || 
		(Input::GetKeyHeld(Keys::Ctrl) && Input::GetKeyDown(Keys::D)))
	{
		switch (pickMode)
		{
			case WorldEditor::PickMode::Actor:
			{
				if (pickedActor)
				{
					editor->RemoveActorFromWorldList();

					if (pickedActors.size() > 1)
					{
						//Destroy all multiple picked actors
						for (auto actor : pickedActors)
						{
							actor->Destroy();
						}
					}
					else
					{
						debugMenu.AddNotification(VString::wformat(
							L"Destroyed actor [%S]", pickedActor->GetName().c_str()));
						pickedActor->Destroy();
					}
				}

				break;
			}
			case WorldEditor::PickMode::Component:
			{
				if (pickedComponent)
				{
					pickedComponent->Remove();
				}

				break;
			}
		}

		pickedActors.clear();
		pickedActor = nullptr;
		pickedComponent = nullptr;

		editor->ClearProperties();
	}
}

//Spawn actor on middle mouse click in viewport
void SpawnActorOnClick()
{
	if (Input::GetMouseMiddleUp())
	{
		if (spawnSystem)
		{
			Ray ray;
			if (RaycastFromScreen(ray)) //Spawn actor at ray hit point
			{
				XMVECTOR dist = ray.direction * ray.hitDistance;
				XMVECTOR rayEnd = ray.origin + dist;

				Transform transform;

				//Round the position up for spawning on the grid in increments
				rayEnd = XMVectorRound(rayEnd);
				XMStoreFloat3(&transform.position, rayEnd);

				SpawnActor(transform);
			}
			else //Spawn actor a bit in front of the camera based on the click
			{
				XMVECTOR spawnPos = XMLoadFloat3(&activeCamera->transform.position);
				XMFLOAT3 forward = activeCamera->GetForwardVector();
				XMVECTOR forwardVec = XMLoadFloat3(&forward);
				spawnPos += forwardVec * 10.0f;

				XMVECTOR dist = ray.direction * 10.f;
				XMVECTOR rayEnd = ray.origin + dist;

				Transform transform;

				//Round the position up for spawning on the grid in increments
				rayEnd = XMVectorRound(rayEnd);
				XMStoreFloat3(&transform.position, rayEnd);

				SpawnActor(transform);
			}

			editor->UpdateWorldList();
		}
	}
}

void SpawnActor(Transform& transform)
{
	Actor* actor = nullptr;

	if (!actorTemplateFilename.empty()) //Spawn actor through template
	{
		std::string path = AssetBaseFolders::actorTemplate + actorTemplateFilename;
		Deserialiser d(path, OpenMode::In);

		actor = spawnSystem->SpawnActor(transform);

		auto actorProps = actor->GetProps();

		//@Todo: there needs to be an actortemplate specific Deserialse() to be able to create components.
		//Somethng like, 'when L"next" is hit, lookup Component System by name and spawn'.
		d.Deserialise(actorProps);

		actor->Create();
		actor->ResetOwnerUIDToComponents();

		for (auto& componentPair : actor->componentMap)
		{
			auto componentProps = componentPair.second->GetProps();
			d.Deserialise(componentProps);
			componentPair.second->Create();
		}

		//Set the transform, props will have the original transform data and will be
		//different from the click position in world.
		actor->SetTransform(transform);

		std::string newActorName = spawnSystem->GetName() + std::to_string(spawnSystem->GetNumActors() - 1);
		actor->SetName(newActorName);

		debugMenu.AddNotification(VString::wformat(
			L"Spawned actor [%S] from template", actor->GetName().c_str()));
	}
	else //Spawn MeshActor (usually)
	{
		actor = spawnSystem->SpawnActor(transform);
		debugMenu.AddNotification(VString::wformat(
			L"Spawned actor [%S] from MeshActor system", actor->GetName().c_str()));
	}

	pickedActor = actor;
	editor->SetActorProps(pickedActor);
}

void WorldEditor::DeselectPickedActor()
{
	pickedActor = nullptr;
	editor->ClearProperties();
}

void WorldEditor::DeselectAll()
{
	DeselectPickedActor();
	pickedComponent = nullptr;
}

void WorldEditor::SetPickedActor(Actor* actor)
{
	assert(actor);
	pickedActor = actor;

	pickedActors.insert(actor);

	editor->SetActorProps(pickedActor);
	editor->SelectActorInWorldList();
}

void WorldEditor::SetPickedComponent(SpatialComponent* spatialComponent)
{
	assert(spatialComponent);
	pickedComponent = spatialComponent;
}

Actor* WorldEditor::GetPickedActor()
{
	return pickedActor;
}

std::set<Actor*>& WorldEditor::GetPickedActors()
{
	return pickedActors;
}

WorldEditor::PickMode WorldEditor::GetPickMode()
{
	return pickMode;
}

SpatialComponent* WorldEditor::GetPickedComponent()
{
	return pickedComponent;
}

void WorldEditor::SetPickMode(PickMode newPickMode)
{
	pickMode = newPickMode;
}

void WorldEditor::SetSpawnSystem(IActorSystem* newSpawnSystem)
{
	assert(newSpawnSystem);
	spawnSystem = newSpawnSystem;
}

IActorSystem* WorldEditor::GetSpawnSystem()
{
	return spawnSystem;
}

void WorldEditor::AddPickedActor(Actor* actor)
{
	assert(actor);
	pickedActors.insert(actor);
}

void WorldEditor::ClearPickedActors()
{
	pickedActors.clear();
}

std::string WorldEditor::GetActorTempateFilename()
{
	return actorTemplateFilename;
}

void WorldEditor::SetActorTemplateFilename(std::string netActorTempateFilename)
{
	actorTemplateFilename = netActorTempateFilename;
}

