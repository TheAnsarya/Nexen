#pragma once
#include "pch.h"

enum class RecordMovieFrom
{
	StartWithoutSaveData = 0,
	StartWithSaveData,
	CurrentState
};

struct RecordMovieOptions
{
	char Filename[2000] = {};
	char Author[250] = {};
	char Description[10000] = {};

	RecordMovieFrom RecordFrom = RecordMovieFrom::StartWithoutSaveData;
};

// TAS state information for the UI
struct TasState
{
	uint32_t FrameCount = 0;
	uint32_t RerecordCount = 0;
	uint32_t LagFrameCount = 0;
	bool IsRecording = false;
	bool IsPlaying = false;
	bool IsPaused = false;
};

namespace MovieKeys
{
	constexpr const char* MesenVersion = "MesenVersion";
	constexpr const char* MovieFormatVersion = "MovieFormatVersion";
	constexpr const char* GameFile = "GameFile";
	constexpr const char* Sha1 = "SHA1";
	constexpr const char* PatchFile = "PatchFile";
	constexpr const char* PatchFileSha1 = "PatchFileSHA1";
	constexpr const char* PatchedRomSha1 = "PatchedRomSHA1";
	constexpr const char* RerecordCount = "RerecordCount";
};