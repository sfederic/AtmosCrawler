#include "vpch.h"
#include "Console.h"
#include <dwrite.h>
#include <d2d1.h>
#include "Input.h"
#include "UI/UISystem.h"
#include "DebugMenu.h"
#include "Render/Renderer.h"
#include "Asset/AssetSystem.h"
#include "FileSystem.h"
#include "World.h"
#include "WorldEditor.h"

std::map<std::wstring, std::pair<std::function<void()>, std::string>> Console::executeMap;

bool Console::bConsoleActive;

std::wstring consoleString;

void Console::Init()
{
	//NOTE: command strings need to be uppercase with WndProc

	//Debug Menu Commands

	executeMap.emplace(L"LS",
		std::make_pair([]() { debugMenu.consoleCommandsMenuOpen = !debugMenu.consoleCommandsMenuOpen; },
		"List all console commands"));

	executeMap.emplace(L"SNAP",
		std::make_pair([]() { debugMenu.snapMenuOpen = !debugMenu.snapMenuOpen; },
		"Show snapping menu"));

	executeMap.emplace(L"PROFILE",
		std::make_pair([]() { debugMenu.profileMenuOpen = !debugMenu.profileMenuOpen; },
		"Show profile stats"));

	executeMap.emplace(L"FPS",
		std::make_pair([]() { debugMenu.fpsMenuOpen = !debugMenu.fpsMenuOpen; },
		"Show FPS and GPU timing info"));

	executeMap.emplace(L"PROPS",
		std::make_pair([]() { debugMenu.propsMenuOpen = !debugMenu.propsMenuOpen; },
		"Show actor props"));

	executeMap.emplace(L"COMMANDS",
		std::make_pair([]() { debugMenu.commandsMenuOpen = !debugMenu.commandsMenuOpen; },
		"Shows current undo / redo commands in buffer"));

	executeMap.emplace(L"GPU",
		std::make_pair([]() { debugMenu.gpuMenuOpen = !debugMenu.gpuMenuOpen; },
		"Show GPU info"));

	executeMap.emplace(L"ACTOR",
		std::make_pair([]() { debugMenu.actorInspectMenuOpen = !debugMenu.actorInspectMenuOpen; },
		"Shows actor info while hovering over the actor with mouse"));

	executeMap.emplace(L"ACTORSYSTEM",
		std::make_pair([]() { debugMenu.actorSystemMenuOpen = !debugMenu.actorSystemMenuOpen; },
		"Show actor system stats"));

	executeMap.emplace(L"COMPONENTSYSTEM",
		std::make_pair([]() { debugMenu.componentSystemMenuOpen = !debugMenu.componentSystemMenuOpen; },
		"Show component system stats"));

	executeMap.emplace(L"STATS",
		std::make_pair([]() { debugMenu.worldStatsMenuOpen = !debugMenu.worldStatsMenuOpen; },
		"shows in - world stats (e.g.vertex count, actor count)"));

	executeMap.emplace(L"GAME",
		std::make_pair([]() { debugMenu.gameInstaceMenuOpen = !debugMenu.gameInstaceMenuOpen; },
		"Menu for manipulating game instance data"));

	executeMap.emplace(L"PARTICLE",
		std::make_pair([]() { debugMenu.particleMenuOpen = !debugMenu.particleMenuOpen; },
		"Particle"));

	executeMap.emplace(L"MEM", 
		std::make_pair([]() { debugMenu.memoriesMenuOpen = !debugMenu.memoriesMenuOpen; },
		"Show all Memory info player has"));

	executeMap.emplace(L"QUEST",
		std::make_pair([]() { debugMenu.questMenuOpen = !debugMenu.questMenuOpen; },
		"Show all in-game quests and their state"));

	executeMap.emplace(L"MEMORY",
		std::make_pair([]() { debugMenu.memoryMenuOpen = !debugMenu.memoryMenuOpen; },
		"show memory for engine systems"));

	executeMap.emplace(L"SKEL",
		std::make_pair([]() { debugMenu.skeletonViewMenuOpen = !debugMenu.skeletonViewMenuOpen; },
		"Show skeleton heirarchy on actor's meshcomponent"));

	executeMap.emplace(L"RESET",
		std::make_pair([]() { FileSystem::ReloadCurrentWorld(); },
		"Reload current world"));

	executeMap.emplace(L"CORE",
		std::make_pair([]() { debugMenu.coreMenuOpen = !debugMenu.coreMenuOpen; },
		"Show core engine variables"));

	executeMap.emplace(L"BAKE",
		std::make_pair([]() { Renderer::RenderLightProbeViews(); },
		"Work through light probes in map and get their RBG values from a cubemap rendering"));

	executeMap.emplace(L"BIN",
		std::make_pair([]() { FileSystem::WriteAllSystemsToBinary(); },
		"Save current world to binary format"));

	executeMap.emplace(L"LOADBIN",
		std::make_pair([]() { FileSystem::ReadAllSystemsFromBinary(); },
		"Load current world from existing binary file"));

	executeMap.emplace(L"BUILD MESHES",
		std::make_pair([]() { assetSystem.WriteAllMeshDataToMeshAssetFiles(); },
		"Build meshes as their engine specific file format."));

	executeMap.emplace(L"READ MESHES",
		std::make_pair([]() { assetSystem.ReadAllMeshAssetsFromFile("Meshes/animated_cube.vmesh"); },
			"test case for cube.vmesh"));

	executeMap.emplace(L"BUILD MAPS",
		std::make_pair([]() { assetSystem.BuildAllGameplayMapFiles(); },
		"Write all game save maps."));

	executeMap.emplace(L"DEFAULT",
		std::make_pair([]() { World::CreateDefaultMapActors(); },
		"Load in default actors for most worlds (Player, Grid, DirectionalLight, etc.)"));

	executeMap.emplace(L"CUTSCENE",
		std::make_pair([]() { debugMenu.cutsceneSequencerOpen = !debugMenu.cutsceneSequencerOpen; },
		"Open Cutscene Sequencer."));

	executeMap.emplace(L"TEXTURE",
		std::make_pair([]() { WorldEditor::texturePlacement = !WorldEditor::texturePlacement; },
		"Enable texture placement mode in editor"));

	executeMap.emplace(L"MATERIAL",
		std::make_pair([]() { WorldEditor::materialPlacement = !WorldEditor::materialPlacement; },
		"Enable material placement mode in editor"));

	executeMap.emplace(L"MS",
		std::make_pair([]() { Renderer::MeshIconImageCapture(); },
		"Take an editor icon image of the currently selected mesh (MS stands for Mesh Snap, like Pokemon 'Snap')"));
}

void Console::ConsoleInput()
{
	if (Input::GetAnyKeyUp())
	{
		if (Input::GetKeyUp(Keys::BackSpace) && !consoleString.empty())
		{
			consoleString.pop_back();
		}
		else
		{
			std::set<Keys> upKeys = Input::GetAllUpKeys();
			for (Keys key : upKeys)
			{
				consoleString.push_back((int)key);
			}
		}
	}
}

//Make sure D2D render target calls have been made (Begin/End Draw)
void Console::Tick()
{
	if (bConsoleActive)
	{
		float width = (float)Renderer::GetViewportWidth();
		float height = (float)Renderer::GetViewportHeight();

		uiSystem.d2dRenderTarget->DrawRectangle({ 0, height - 50.f, width, height }, uiSystem.debugBrushText);

		uiSystem.textFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_JUSTIFIED);
		uiSystem.d2dRenderTarget->DrawText(consoleString.c_str(), consoleString.size(), uiSystem.textFormat,
			{ 0, height - 50.f, width, height }, uiSystem.debugBrushText);
	}
}

void Console::InputTick()
{
	if (Input::GetKeyUp(Keys::Tilde)) //~ key, like doom and unreal
	{
		bConsoleActive = !bConsoleActive;
		return;
	}

	if (bConsoleActive)
	{
		if (Input::GetKeyUp(Keys::Enter))
		{
			ExecuteString();
			bConsoleActive = false;
			return;
		}

		ConsoleInput();
	}
}

void Console::ExecuteString()
{
	auto executeIt = executeMap.find(consoleString);
	if (executeIt != executeMap.end())
	{
		executeIt->second.first();
	}
	else
	{
		debugMenu.AddNotification(L"No command found");
	}

	consoleString.clear();
}
