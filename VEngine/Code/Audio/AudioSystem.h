#pragma once
#include <xaudio2.h>
#include <string>
#include <memory>
#include <map>
#include "AudioBase.h"
#include "AudioChannel.h"
#include "System.h"

//@Todo: Ogg Vorbis loader if file size becomes an issue https://www.gamedev.net/forums/topic/496350-xaudio2-and-ogg/

//Lot of this was inspired by https://gdcvault.com/play/1022061/How-to-Write-an-Audio
struct AudioSystem : System
{
private:
	uint64_t nextChannelID = 0;

	IXAudio2* audioEngine = nullptr; //Main XAudio2 sound engine
	IXAudio2MasteringVoice* masteringVoice = nullptr; //Main track	

	typedef std::map<std::string, std::unique_ptr<AudioBase>> AudioMap;
	AudioMap loadedAudioMap;

	typedef std::map<uint64_t, std::unique_ptr<AudioChannel>> ChannelMap;
	ChannelMap channelMap;

public:
	AudioSystem();
	void Init();
	void Tick();
	void Cleanup();
	void DeleteLoadedAudioAndChannels();
	AudioChannel* GetChannel(uint64_t channelID);
	void MuteAllAudio();
	void StopAllAudio();
	void StartAllAudio();
	void UnmuteAllAudio();
	void FadeOutAllAudio();
	void FadeInAllAudio();

	//Returns channel ID that audio is playing on so that audio components can work with that data.
	uint64_t PlayAudio(const std::string filename, bool loopAudio = false);

	void LoadAudio(const std::string filename);
	void UnloadAudio(const std::string filename);

private:
	HRESULT LoadWAV(const std::string filename, WAVEFORMATEXTENSIBLE& waveFormat, XAUDIO2_BUFFER& buffer);
	HRESULT FindChunk(HANDLE file, DWORD fourcc, DWORD* dwChunkSize, DWORD* dwChunkDataPosition);
	HRESULT ReadChunkData(HANDLE file, void* buffer, DWORD bufferSize, DWORD bufferOffset);

	AudioChannel* CreateAudioChannel();
	AudioBase* CreateAudioBase(std::string audioFilename);
};

extern AudioSystem audioSystem;
