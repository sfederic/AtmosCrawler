#include "vpch.h"
#include "AudioSystem.h"
#include <filesystem>
#include <cassert>
#include "Debug.h"
#include "Components/AudioComponent.h"

#define fourccRIFF 'FFIR'
#define fourccDATA 'atad'
#define fourccFMT ' tmf'
#define fourccWAVE 'EVAW'
#define fourccXWMA 'AMWX'
#define fourccDPDS 'sdpd'

AudioSystem audioSystem;

AudioSystem::AudioSystem() : System("AudioSystem")
{
}

void AudioSystem::Init()
{
	HR(XAudio2Create(&audioEngine));
	
#ifdef _DEBUG
	XAUDIO2_DEBUG_CONFIGURATION debug = {};
	debug.BreakMask = XAUDIO2_LOG_ERRORS;
	debug.TraceMask = XAUDIO2_LOG_ERRORS;
	audioEngine->SetDebugConfiguration(&debug);
#endif

	HR(audioEngine->CreateMasteringVoice(&masteringVoice));
}

void AudioSystem::Tick()
{
	std::vector<ChannelMap::iterator> stoppedChannels;

	for (auto it = channelMap.begin(), itEnd = channelMap.end(); it != itEnd; it++)
	{
		if (!it->second->isPlaying)
		{
			stoppedChannels.push_back(it);
		}
	}

	for (auto& it : stoppedChannels)
	{
		channelMap.erase(it);
	}
}

void AudioSystem::Cleanup()
{
	DeleteLoadedAudioAndChannels();

	masteringVoice->DestroyVoice();
	audioEngine->Release();

	//Needs to be called after XAudio2 shutdown. Only reference I could find on this is in the DirectXTK SDK samples
	//REF:https://stackoverflow.com/questions/56091616/directx-sdk-june-2010-xaudio2-crashes-on-every-app-exit
	CoUninitialize();
}

void AudioSystem::DeleteLoadedAudioAndChannels()
{
	channelMap.clear();
	loadedAudioMap.clear();
}

AudioChannel* AudioSystem::GetChannel(uint64_t channelID)
{
	return channelMap.find(channelID)->second.get();
}

void AudioSystem::MuteAllAudio()
{
	for (auto audio : AudioComponent::system.components)
	{
		audio->volume = 0.f;
	}
}

void AudioSystem::StopAllAudio()
{
	for (auto audio : AudioComponent::system.components)
	{
		channelMap[audio->channelID]->sourceVoice->Stop();
	}
}

void AudioSystem::StartAllAudio()
{
	for (auto audio : AudioComponent::system.components)
	{
		channelMap[audio->channelID]->sourceVoice->Start();
	}
}

void AudioSystem::UnmuteAllAudio()
{
	for (auto audio : AudioComponent::system.components)
	{
		audio->volume = 1.f;
	}
}

void AudioSystem::FadeOutAllAudio()
{
	for (auto audio : AudioComponent::system.components)
	{
		audio->SetToFadeOut();
	}
}

void AudioSystem::FadeInAllAudio()
{
	for (auto audio : AudioComponent::system.components)
	{
		audio->SetToFadeIn();
	}
}

uint64_t AudioSystem::PlayAudio(const std::string filename, bool loopAudio)
{
	auto audioIt = loadedAudioMap.find(filename);
	if (audioIt == loadedAudioMap.end())
	{
		LoadAudio(filename);
		audioIt = loadedAudioMap.find(filename);
	}

	AudioBase* audio = audioIt->second.get();

	auto channel = CreateAudioChannel();
	channel->isPlaying = true;

	IXAudio2SourceVoice* sourceVoice = nullptr;
	HR(audioEngine->CreateSourceVoice(&sourceVoice, (WAVEFORMATEX*)&audio->waveFormat, 0, 2.0f, channel));

	channel->sourceVoice = sourceVoice;

	if (loopAudio)
	{
		channel->isLooping = true;
		audio->buffer.LoopCount = XAUDIO2_LOOP_INFINITE;
	}

	HR(sourceVoice->SubmitSourceBuffer(&audio->buffer));
	HR(sourceVoice->Start(0));

	return nextChannelID;
}

void AudioSystem::LoadAudio(const std::string filename)
{
	std::string path = "Audio/" + filename;
	assert(std::filesystem::exists(path) && "Audio file not found.");

	auto audio = CreateAudioBase(filename);

	//Initilization of audio is bad if nothing is zeroed out, source voice fails
	HR(LoadWAV(path, audio->waveFormat, audio->buffer));
}

void AudioSystem::UnloadAudio(const std::string filename)
{
	auto audioIt = loadedAudioMap.find(filename);
	assert(audioIt != loadedAudioMap.end());

	loadedAudioMap.erase(audioIt);
}

HRESULT AudioSystem::FindChunk(HANDLE file, DWORD fourcc, DWORD* dwChunkSize, DWORD* dwChunkDataPosition)
{
	if (SetFilePointer(file, 0, NULL, FILE_BEGIN) == INVALID_SET_FILE_POINTER) {
		return HRESULT_FROM_WIN32(GetLastError());
	}

	DWORD dwChunkType;
	DWORD dwChunkDataSize;
	DWORD dwRIFFDataSize = 0;
	DWORD dwFileType;
	DWORD bytesRead = 0;
	DWORD dwOffset = 0;

	HRESULT hr = S_OK;
	while (hr == S_OK)
	{
		DWORD dwRead;
		if (0 == ReadFile(file, &dwChunkType, sizeof(DWORD), &dwRead, NULL)) {
			hr = HRESULT_FROM_WIN32(GetLastError());
		}

		if (0 == ReadFile(file, &dwChunkDataSize, sizeof(DWORD), &dwRead, NULL)) {
			hr = HRESULT_FROM_WIN32(GetLastError());
		}

		switch (dwChunkType)
		{
		case fourccRIFF:
			dwRIFFDataSize = dwChunkDataSize;
			dwChunkDataSize = 4;
			if (0 == ReadFile(file, &dwFileType, sizeof(DWORD), &dwRead, NULL))
				hr = HRESULT_FROM_WIN32(GetLastError());
			break;

		default:
			if (INVALID_SET_FILE_POINTER == SetFilePointer(file, dwChunkDataSize, NULL, FILE_CURRENT))
				return HRESULT_FROM_WIN32(GetLastError());
		}

		dwOffset += sizeof(DWORD) * 2;

		if (dwChunkType == fourcc)
		{
			*dwChunkSize = dwChunkDataSize;
			*dwChunkDataPosition = dwOffset;
			return S_OK;
		}

		dwOffset += dwChunkDataSize;

		if (bytesRead >= dwRIFFDataSize) return S_FALSE;
	}

	return S_OK;
}

HRESULT AudioSystem::ReadChunkData(HANDLE file, void* buffer, DWORD bufferSize, DWORD bufferOffset)
{
	HRESULT hr = S_OK;
	if (INVALID_SET_FILE_POINTER == SetFilePointer(file, bufferOffset, NULL, FILE_BEGIN)) {
		return HRESULT_FROM_WIN32(GetLastError());
	}

	DWORD dwRead;
	if (0 == ReadFile(file, buffer, bufferSize, &dwRead, NULL)) {
		hr = HRESULT_FROM_WIN32(GetLastError());
	}

	return hr;
}

AudioChannel* AudioSystem::CreateAudioChannel()
{
	channelMap.insert(std::make_pair(++nextChannelID, std::make_unique<AudioChannel>()));
	return channelMap[nextChannelID].get();
}

AudioBase* AudioSystem::CreateAudioBase(std::string audioFilename)
{
	loadedAudioMap.insert(std::make_pair(audioFilename, std::make_unique<AudioBase>()));
	return loadedAudioMap[audioFilename].get();
}

//REF::https://docs.microsoft.com/en-us/windows/win32/xaudio2/how-to--load-audio-data-files-in-xaudio2
HRESULT AudioSystem::LoadWAV(const std::string filename, WAVEFORMATEXTENSIBLE& waveFormat, XAUDIO2_BUFFER& buffer)
{
	HANDLE file = CreateFileA(filename.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);

	if (INVALID_HANDLE_VALUE == file) 
	{
		return HRESULT_FROM_WIN32(GetLastError());
	}

	if (INVALID_SET_FILE_POINTER == SetFilePointer(file, 0, NULL, FILE_BEGIN)) 
	{
		return HRESULT_FROM_WIN32(GetLastError());
	}

	DWORD chunkSize;
	DWORD chunkPosition;
	HR(FindChunk(file, fourccRIFF, &chunkSize, &chunkPosition));

	DWORD fileType;
	HR(ReadChunkData(file, &fileType, sizeof(DWORD), chunkPosition));
	if (fileType != fourccWAVE) 
	{
		return S_FALSE;
	}

	HR(FindChunk(file, fourccFMT, &chunkSize, &chunkPosition));
	HR(ReadChunkData(file, &waveFormat, chunkSize, chunkPosition));

	FindChunk(file, fourccDATA, &chunkSize, &chunkPosition);
	BYTE* pDataBuffer = (BYTE*)malloc(chunkSize);
	ReadChunkData(file, pDataBuffer, chunkSize, chunkPosition);

	buffer.AudioBytes = chunkSize;
	buffer.pAudioData = pDataBuffer;
	buffer.Flags = XAUDIO2_END_OF_STREAM;

	return S_OK;
}
