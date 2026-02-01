#pragma once
#include "pch.h"
#include "Shared/MessageManager.h"
#include "Shared/Interfaces/IInputProvider.h"
#include "Shared/Movies/MovieTypes.h"
#include "Shared/Movies/MovieRecorder.h"
#include "Utilities/safe_ptr.h"

class VirtualFile;
class Emulator;

/// <summary>
/// Base interface for movie playback implementations.
/// Provides input via IInputProvider during playback.
/// </summary>
class IMovie : public IInputProvider {
public:
	virtual ~IMovie() = default;

	/// <summary>
	/// Start playing movie from file.
	/// </summary>
	/// <param name="file">Movie file to play</param>
	/// <returns>True if movie loaded successfully</returns>
	virtual bool Play(VirtualFile& file) = 0;

	/// <summary>Stop movie playback</summary>
	virtual void Stop() = 0;

	/// <summary>Check if movie currently playing</summary>
	virtual bool IsPlaying() = 0;
};

/// <summary>
/// TAS movie recorder and playback manager.
/// Supports recording and playing back movies in various formats.
/// </summary>
/// <remarks>
/// Supported formats:
/// - Nexen (.msm) - Nexen native format
/// - FCEUX (.fm2, .fm3) - NES/Famicom movies
/// - BizHawk (.bk2) - Multi-system TAS format
/// - Lsnes (.lsmv) - SNES TAS format
///
/// Recording:
/// - Captures input frame-by-frame
/// - ROM info and save data included
/// - Configurable start conditions (power-on, save state)
///
/// Playback:
/// - Deterministic replay of recorded input
/// - Works as IInputProvider for emulator
/// - Auto-detects movie format from header
///
/// Features:
/// - Frame-perfect synchronization
/// - Save state support
/// - Battery/save data handling
/// - Multi-controller support
///
/// Thread safety: Uses safe_ptr for thread-safe access to player/recorder.
/// </remarks>
class MovieManager {
private:
	Emulator* _emu = nullptr;          ///< Emulator instance
	safe_ptr<IMovie> _player;          ///< Active movie player
	safe_ptr<MovieRecorder> _recorder; ///< Active movie recorder

public:
	/// <summary>Construct movie manager for emulator</summary>
	MovieManager(Emulator* emu);

	/// <summary>
	/// Start recording movie.
	/// </summary>
	/// <param name="options">Recording options (filename, format, start state, etc.)</param>
	void Record(RecordMovieOptions options);

	/// <summary>
	/// Start playing movie.
	/// </summary>
	/// <param name="file">Movie file to play</param>
	/// <param name="silent">Suppress notifications if true</param>
	void Play(VirtualFile file, bool silent = false);

	/// <summary>Stop recording or playback</summary>
	void Stop();

	/// <summary>Check if movie currently playing</summary>
	bool Playing();

	/// <summary>Check if movie currently recording</summary>
	bool Recording();
};
