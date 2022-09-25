#pragma once

#include <string>
#include <set>
#include "Properties.h"

class SalvageMission;

//Instance holding data over the entirety of the game.
//GameInstane is also used as a global save file of sorts, seperate from .vmaps.
struct GameInstance
{
	//this is to set in the editor to know whether to use map files from WorldMaps/ vs GameSaves/
	inline static bool useGameSaves = false;

	inline static std::string startingMap = "test_worldmap.vmap";
	inline static std::string previousMapMovedFrom = startingMap;

	//Used when continuing from game save files
	inline static std::string mapToLoadOnContinue;

	inline static std::set<std::string> playerPhotoTagsCaptured;

	static Properties GetInstanceSaveData();
};
