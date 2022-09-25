#pragma once

#include <cstdint>
#include <string>
#include <Windows.h>

//Any utility functions or variables that are 'core' to the engine live here.
namespace Core
{
	extern bool mainLoop;
	extern bool gameplayOn;
	extern bool gameWorldPaused;

	extern bool isImGUIEnabled;

	extern double timeSinceStartup;

	extern uint32_t finalFrameCount;

	//Speed at which game runs
	extern float timeScale;

	//When starting a gameplay session, keep the first loaded world's name here to switch back
	//to this level when gameplay stops in-editor.
	extern std::string initialStartingWorldFromEditor;

	float GetDeltaTime();
	void SetTimerFrequency();
	void Init();
	void Tick();
	void StartTimer();
	void EndTimer();
	void StartGame();
	void EndGame();
	void SetGameplayState();
	void HandleWin32MessagePump(UINT message, WPARAM wparam, LPARAM lparam);
	LRESULT __stdcall WndProc(HWND window, UINT message, WPARAM wparam, LPARAM lparam);
}
