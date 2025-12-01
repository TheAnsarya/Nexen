#include "pch.h"
#include <algorithm>
#include "Utilities/FolderUtilities.h"
#include "Utilities/VirtualFile.h"
#include "Utilities/ZipReader.h"
#include "Shared/Emulator.h"
#include "Shared/Movies/MovieManager.h"
#include "Shared/Movies/MesenMovie.h"
#include "Shared/Movies/MovieRecorder.h"

MovieManager::MovieManager(Emulator* emu)
{
	_emu = emu;
}

void MovieManager::Record(RecordMovieOptions options)
{
	//Stop any active recording/playback before starting playback for this movie
	Stop();

	shared_ptr<MovieRecorder> recorder(new MovieRecorder(_emu));
	if(recorder->Record(options)) {
		_recorder.reset(recorder);
	}
}

void MovieManager::Play(VirtualFile file, bool forTest)
{
	vector<uint8_t> fileData;
	if(file.IsValid() && file.ReadFile(fileData)) {
		shared_ptr<IMovie> player;
		if(memcmp(fileData.data(), "PK", 2) == 0) {
			//Mesen movie
			ZipReader reader;
			reader.LoadArchive(fileData);

			vector<string> files = reader.GetFileList();
			if(std::find(files.begin(), files.end(), "GameSettings.txt") != files.end()) {
				player.reset(new MesenMovie(_emu, forTest));
			}
		}

		//Stop any active recording/playback before starting playback for this movie
		Stop();

		if(player && player->Play(file)) {
			_player.reset(player);
			if(!forTest) {
				MessageManager::DisplayMessage("Movies", "MoviePlaying", file.GetFileName());
			}
		}
	}
}

void MovieManager::Stop()
{
	shared_ptr<IMovie> player = _player.lock();
	if(player) {
		player->Stop();
	}
	_player.reset();
	_recorder.reset();
}

bool MovieManager::Playing()
{
	return _player != nullptr;
}

bool MovieManager::Recording()
{
	return _recorder != nullptr;
}

void MovieManager::IncrementRerecordCount()
{
	MovieRecorder* recorder = _recorder.get();
	if(recorder) {
		recorder->IncrementRerecordCount();
	}
}

TasState MovieManager::GetTasState()
{
	TasState state = {};
	state.IsRecording = Recording();
	state.IsPlaying = Playing();
	
	MovieRecorder* recorder = _recorder.get();
	if(recorder) {
		state.FrameCount = recorder->GetFrameCount();
		state.RerecordCount = recorder->GetRerecordCount();
	}
	
	return state;
}

bool MovieManager::HandleRerecord(uint32_t frameNumber)
{
	MovieRecorder* recorder = _recorder.get();
	if(recorder && recorder->IsTasMode()) {
		return recorder->HandleRerecord(frameNumber);
	}
	return false;
}

bool MovieManager::IsTasMode()
{
	MovieRecorder* recorder = _recorder.get();
	return recorder ? recorder->IsTasMode() : false;
}

void MovieManager::SetTasMode(bool enabled)
{
	MovieRecorder* recorder = _recorder.get();
	if(recorder) {
		recorder->SetTasMode(enabled);
	}
}
