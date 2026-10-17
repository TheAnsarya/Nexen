#include "pch.h"
#include "Shared/RewindManager.h"
#include "Shared/MessageManager.h"
#include "Shared/Emulator.h"
#include "Shared/EmuSettings.h"
#include "Shared/Video/VideoRenderer.h"
#include "Shared/Audio/SoundMixer.h"
#include "Shared/BaseControlDevice.h"
#include "Shared/RenderedFrame.h"
#include "Shared/BaseControlManager.h"

RewindManager::RewindManager(Emulator* emu) {
	_emu = emu;
	_settings = emu->GetSettings();
}

RewindManager::~RewindManager() {
	_settings->ClearFlag(EmulationFlags::MaximumSpeed);
	_settings->ClearFlag(EmulationFlags::Rewind);
	_emu->UnregisterInputProvider(this);
	_emu->UnregisterInputRecorder(this);
}

void RewindManager::InitHistory() {
	Reset();
	_emu->RegisterInputProvider(this);
	_emu->RegisterInputRecorder(this);
	AddHistoryBlock();
}

void RewindManager::Reset() {
	_emu->UnregisterInputProvider(this);
	_emu->UnregisterInputRecorder(this);
	_settings->ClearFlag(EmulationFlags::MaximumSpeed);
	_settings->ClearFlag(EmulationFlags::Rewind);
	ClearBuffer();
}

void RewindManager::ClearBuffer() {
	_hasHistory = false;
	_history.clear();
	_historyBackup.clear();
	_framesToFastForward = 0;
	_videoHistory.clear();
	_videoHistoryBuilder.clear();
	_audioRingReadPos = 0;
	_audioRingWritePos = 0;
	_audioRingCount = 0;
	_audioHistoryBuilder.clear();
	_rewindState = RewindState::Stopped;
	_currentHistory = {};
	_totalMemoryUsage = 0;
}

void RewindManager::ProcessNotification(ConsoleNotificationType type, void* parameter) {
	if (_emu->IsRunAheadFrame()) {
		return;
	}

	if (type == ConsoleNotificationType::PpuFrameDone) [[likely]] {
		_hasHistory = _history.size() >= 2;
		if (_settings->GetPreferences().RewindBufferSize > 0) {
			switch (_rewindState) {
				case RewindState::Starting:
				case RewindState::Started:
				case RewindState::Debugging:
					_currentHistory.FrameCount--;
					break;

				case RewindState::Stopping:
					_framesToFastForward--;
					_currentHistory.FrameCount++;
					if (_framesToFastForward == 0) {
						for (int i = 0; i < BaseControlDevice::PortCount; i++) {
							size_t numberToRemove = _currentHistory.InputLogs[i].size();
							_currentHistory.InputLogs[i] = _historyBackup.front().InputLogs[i];
							for (size_t j = 0; j < numberToRemove; j++) {
								_currentHistory.InputLogs[i].pop_back();
							}
						}
						_historyBackup.clear();
						_rewindState = RewindState::Stopped;
						_settings->ClearFlag(EmulationFlags::Rewind);
						_settings->ClearFlag(EmulationFlags::MaximumSpeed);
					}
					break;

				case RewindState::Stopped: [[likely]]
					_currentHistory.FrameCount++;
					break;
			}
		} else {
			ClearBuffer();
		}
	} else if (type == ConsoleNotificationType::StateLoaded) {
		if (_rewindState == RewindState::Stopped) {
			// A save state was loaded by the user, mark as the end of the current "segment" (for history viewer)
			_currentHistory.EndOfSegment = true;
			AddHistoryBlock();
		}
	}
}

RewindStats RewindManager::GetStats() {
	RewindStats stats = {};
	stats.MemoryUsage = (uint32_t)_totalMemoryUsage;
	stats.HistorySize = (uint32_t)_history.size();
	stats.HistoryDuration = stats.HistorySize * RewindManager::BufferSize;
	return stats;
}

void RewindManager::AddHistoryBlock() {
	uint32_t maxHistorySize = _settings->GetPreferences().RewindBufferSize;
	if (maxHistorySize > 0) {
		// Use running total instead of O(n) iteration over entire history
		uint64_t maxBytes = (uint64_t)maxHistorySize << 20; // Convert MB to bytes
		while (_totalMemoryUsage > maxBytes && !_history.empty()) {
			_totalMemoryUsage -= _history.front().GetStateSize();
			_history.pop_front();

			// Remove until next full state (deltas need their base)
			while (!_history.empty() && !_history.front().IsFullState) {
				_totalMemoryUsage -= _history.front().GetStateSize();
				_history.pop_front();
			}
		}

		if (_currentHistory.FrameCount > 0) {
			_totalMemoryUsage += _currentHistory.GetStateSize();
			_history.push_back(_currentHistory);
		}
		_currentHistory = RewindData();
		_currentHistory.SaveState(_emu, _history);
		_totalMemoryUsage += _currentHistory.GetStateSize();
	}
}

void RewindManager::PopHistory() {
	if (_history.empty() && _currentHistory.FrameCount <= 0 && !IsStepBack()) {
		StopRewinding();
	} else {
		if (_currentHistory.FrameCount <= 0 && !IsStepBack()) {
			_currentHistory = _history.back();
			_history.pop_back();
		}

		if (IsStepBack() && _currentHistory.FrameCount <= 1 && !_history.empty() && !_history.back().EndOfSegment) {
			// Go back an extra frame to ensure step back works across 30-frame chunks
			_historyBackup.push_front(_currentHistory);
			_currentHistory = _history.back();
			_history.pop_back();
		}

		_historyBackup.push_front(_currentHistory);
		_currentHistory.LoadState(_emu, _history, -1, false);

		if (!_audioHistoryBuilder.empty()) {
			// Bulk insert into audio ring buffer (replaces O(n) deque front-insert)
			uint32_t count = (uint32_t)_audioHistoryBuilder.size();
			if (count > AudioRingCapacity) {
				// More samples than ring can hold - keep only the most recent
				_audioHistoryBuilder.erase(_audioHistoryBuilder.begin(), _audioHistoryBuilder.begin() + (count - AudioRingCapacity));
				count = AudioRingCapacity;
			}

			// Write samples into ring buffer at current write position
			// These go "before" existing data (prepend), so we move writePos backward
			if (count <= _audioRingWritePos) {
				_audioRingWritePos -= count;
				memcpy(_audioRingBuffer.data() + _audioRingWritePos, _audioHistoryBuilder.data(), count * sizeof(int16_t));
			} else {
				// Wrap around: split into two copies
				uint32_t tailCount = count - _audioRingWritePos;
				memcpy(_audioRingBuffer.data() + (AudioRingCapacity - tailCount), _audioHistoryBuilder.data(), tailCount * sizeof(int16_t));
				memcpy(_audioRingBuffer.data(), _audioHistoryBuilder.data() + tailCount, _audioRingWritePos * sizeof(int16_t));
				_audioRingWritePos = AudioRingCapacity - tailCount;
			}
			_audioRingCount = std::min(_audioRingCount + count, AudioRingCapacity);
			// Read position stays at the "end" (most recent data)
			_audioRingReadPos = (_audioRingWritePos + _audioRingCount) % AudioRingCapacity;
			_audioHistoryBuilder.clear();
		}
	}
}

void RewindManager::Start(bool forDebugger) {
	if (_rewindState == RewindState::Stopped && _settings->GetPreferences().RewindBufferSize > 0) {
		if (forDebugger) {
			InternalStart(forDebugger);
		} else {
			auto lock = _emu->AcquireLock();
			InternalStart(forDebugger);
		}
	}
}

void RewindManager::InternalStart(bool forDebugger) {
	if (_history.empty() && _currentHistory.FrameCount <= 0 && !forDebugger) {
		// No data in history, can't rewind
		return;
	}

	_rewindState = forDebugger ? RewindState::Debugging : RewindState::Starting;
	_videoHistoryBuilder.clear();
	_videoHistory.clear();
	_audioHistoryBuilder.clear();
	_audioRingReadPos = 0;
	_audioRingWritePos = 0;
	_audioRingCount = 0;
	if (_audioRingBuffer.empty()) {
		_audioRingBuffer.resize(AudioRingCapacity);
	}
	_historyBackup.clear();

	PopHistory();
	_emu->GetSoundMixer()->StopAudio(true);
	_settings->SetFlag(EmulationFlags::MaximumSpeed);
	_settings->SetFlag(EmulationFlags::Rewind);
}

void RewindManager::ForceStop(bool deleteFutureData) {
	if (_rewindState != RewindState::Stopped) {
		if (deleteFutureData) {
			// Step back reached its target - delete any future "history" beyond this
			// Otherwise, subsequent step back/rewind operations won't work properly
			RewindData orgHistory = _currentHistory;
			int framesToRemove = _currentHistory.FrameCount;
			if (!_historyBackup.empty()) {
				_currentHistory = _historyBackup.front();
				_currentHistory.FrameCount -= framesToRemove;
				for (int i = 0; i < BaseControlDevice::PortCount; i++) {
					for (int j = 0; j < orgHistory.InputLogs[i].size(); j++) {
						if (!_currentHistory.InputLogs[i].empty()) {
							_currentHistory.InputLogs[i].pop_back();
						}
					}
				}
			}
			_historyBackup.clear();

			if (!_videoHistory.empty()) {
				// Update the frame on the screen to match the last frame generated during step back
				// Needed to update the screen when stepping back to the previous frame
				VideoFrame& frameData = _videoHistory.back();
				RenderedFrame oldFrame(frameData.Data.data(), frameData.Width, frameData.Height, frameData.Scale, frameData.FrameNumber, frameData.InputData);
				_emu->GetVideoRenderer()->UpdateFrame(oldFrame);
			}
		} else {
			while (_historyBackup.size() > 1) {
				_history.push_back(_historyBackup.front());
				_historyBackup.pop_front();
			}
			_currentHistory = _historyBackup.front();
			_historyBackup.clear();
		}

		_rewindState = RewindState::Stopped;
		_settings->ClearFlag(EmulationFlags::MaximumSpeed);
		_settings->ClearFlag(EmulationFlags::Rewind);
	}
}

void RewindManager::Stop() {
	if (_rewindState >= RewindState::Starting) {
		auto lock = _emu->AcquireLock();
		if (_rewindState == RewindState::Started) {
			// Move back to the save state containing the frame currently shown on the screen
			if (_historyBackup.size() > 1) {
				_framesToFastForward = (uint32_t)_videoHistory.size() + _historyBackup.front().FrameCount;
				do {
					_history.push_back(_historyBackup.front());
					_framesToFastForward -= _historyBackup.front().FrameCount;
					_historyBackup.pop_front();

					_currentHistory = _historyBackup.front();
				} while (_framesToFastForward > RewindManager::BufferSize && _historyBackup.size() > 1);
			}
		} else {
			// We started rewinding, but didn't actually visually rewind anything yet
			// Move back to the save state containing the frame currently shown on the screen
			while (_historyBackup.size() > 1) {
				_history.push_back(_historyBackup.front());
				_historyBackup.pop_front();
			}
			_currentHistory = _historyBackup.front();
			_framesToFastForward = _historyBackup.front().FrameCount;
		}

		_currentHistory.LoadState(_emu, _history);
		if (_framesToFastForward > 0) {
			_rewindState = RewindState::Stopping;
			_currentHistory.FrameCount = 0;
			_settings->SetFlag(EmulationFlags::MaximumSpeed);
		} else {
			_rewindState = RewindState::Stopped;
			_historyBackup.clear();
			_settings->ClearFlag(EmulationFlags::MaximumSpeed);
			_settings->ClearFlag(EmulationFlags::Rewind);
		}

		_videoHistoryBuilder.clear();
		_videoHistory.clear();
		_audioHistoryBuilder.clear();
		_audioRingReadPos = 0;
		_audioRingWritePos = 0;
		_audioRingCount = 0;
	}
}

void RewindManager::ProcessEndOfFrame() {
	if (_rewindState >= RewindState::Starting) {
		if (_currentHistory.FrameCount <= 0 && _rewindState != RewindState::Debugging) {
			// If we're debugging, we want to keep running the emulation to the end of the next frame (even if it's incomplete)
			// Otherwise the emulation might diverge due to missing inputs.
			PopHistory();
		} else if (_currentHistory.FrameCount == 0 && _rewindState == RewindState::Debugging) {
			// Reached the end of the current 30-frame block, move to the next,
			// the step back target cycle could be at the start of the next block
			if (_historyBackup.size() > 1) {
				_history.push_back(_historyBackup.front());
				_historyBackup.pop_front();
				_currentHistory = _historyBackup.front();
			}
		}
	} else {
		// Scale rewind recording interval with emulation speed.
		// At normal speed, record every 30 frames (~0.5s at 60fps).
		// At 2x, every 60 frames; at 3x, every 90, etc.
		// This keeps approximate real-time recording frequency constant,
		// avoiding the proportional overhead increase at high speed.
		int32_t threshold = BufferSize;
		uint32_t speed = _settings->GetEmulationSpeed();
		if (speed == 0) {
			// Max speed (unlimited): record every 300 frames (~5s at 60fps)
			threshold = BufferSize * 10;
		} else if (speed > 100) {
			threshold = BufferSize * static_cast<int32_t>(speed) / 100;
		}

		if (_currentHistory.FrameCount >= threshold) {
			AddHistoryBlock();
		}
	}
}

void RewindManager::ProcessFrame(RenderedFrame& frame, bool forRewind) {
	if (_rewindState == RewindState::Starting || _rewindState == RewindState::Started) {
		if (!forRewind) {
			// Ignore any frames that occur between start of rewind process & first rewinded frame completed
			// These are caused by the fact that VideoDecoder is asynchronous - a previous (extra) frame can end up
			// in the rewind queue, which causes display glitches
			return;
		}

		// Reuse VideoFrame buffer via CopyFrom (avoids per-frame heap allocation)
		_videoHistoryBuilder.emplace_back();
		_videoHistoryBuilder.back().CopyFrom(frame);

		if (_videoHistoryBuilder.size() == (size_t)_historyBackup.front().FrameCount) {
			for (int i = (int)_videoHistoryBuilder.size() - 1; i >= 0; i--) {
				_videoHistory.push_front(std::move(_videoHistoryBuilder[i]));
			}
			_videoHistoryBuilder.clear();
		}

		if (_rewindState == RewindState::Started || _videoHistory.size() >= RewindManager::BufferSize) {
			_rewindState = RewindState::Started;
			_settings->ClearFlag(EmulationFlags::MaximumSpeed);
			if (!_videoHistory.empty()) {
				VideoFrame& frameData = _videoHistory.back();
				RenderedFrame oldFrame(frameData.Data.data(), frameData.Width, frameData.Height, frameData.Scale, frameData.FrameNumber, frameData.InputData);
				_emu->GetVideoRenderer()->UpdateFrame(oldFrame);
				_videoHistory.pop_back();
			}
		}
	} else if (_rewindState == RewindState::Stopping) {
		// Display nothing while resyncing
	} else if (_rewindState == RewindState::Debugging) {
		// Keep the last frame to be able to display it once step back reaches its target
		_videoHistory.clear();
		_videoHistory.emplace_back();
		_videoHistory.back().CopyFrom(frame);
	} else {
		_emu->GetVideoRenderer()->UpdateFrame(frame);
	}
}

bool RewindManager::ProcessAudio(int16_t* soundBuffer, uint32_t sampleCount) {
	if (_rewindState == RewindState::Starting || _rewindState == RewindState::Started) {
		_audioHistoryBuilder.insert(_audioHistoryBuilder.end(), soundBuffer, soundBuffer + sampleCount * 2);

		uint32_t totalSamples = sampleCount * 2;
		if (_rewindState == RewindState::Started && _audioRingCount > totalSamples) {
			// Bulk read from ring buffer (replaces per-sample deque pop_back)
			// Read from the "back" (most recent) — decrement readPos
			for (uint32_t i = 0; i < totalSamples; i++) {
				_audioRingReadPos = (_audioRingReadPos == 0) ? AudioRingCapacity - 1 : _audioRingReadPos - 1;
				soundBuffer[i] = _audioRingBuffer[_audioRingReadPos];
			}
			_audioRingCount -= totalSamples;

			return true;
		} else {
			// Mute while we prepare to rewind
			return false;
		}
	} else if (_rewindState == RewindState::Stopping || _rewindState == RewindState::Debugging) {
		// Mute while we resync
		return false;
	} else {
		return true;
	}
}

void RewindManager::RecordInput(const vector<shared_ptr<BaseControlDevice>>& devices) {
	if (_settings->GetPreferences().RewindBufferSize > 0 && _rewindState == RewindState::Stopped) {
		for (const shared_ptr<BaseControlDevice>& device : devices) {
			_currentHistory.InputLogs[device->GetPort()].push_back(device->GetRawState());
		}
	}
}

bool RewindManager::SetInput(BaseControlDevice* device) {
	uint8_t port = device->GetPort();
	if (IsRewinding()) {
		if (!_currentHistory.InputLogs[port].empty()) {
			ControlDeviceState state = _currentHistory.InputLogs[port].front();
			_currentHistory.InputLogs[port].pop_front();
			device->SetRawState(state);
			return true;
		} else {
			return false;
		}
	} else {
		return false;
	}
}

void RewindManager::StartRewinding(bool forDebugger) {
	Start(forDebugger);
}

void RewindManager::StopRewinding(bool forDebugger, bool deleteFutureData) {
	if (forDebugger) {
		ForceStop(deleteFutureData);
	} else {
		Stop();
	}
}

bool RewindManager::IsRewinding() {
	return _rewindState != RewindState::Stopped;
}

bool RewindManager::IsStepBack() {
	return _rewindState == RewindState::Debugging;
}

void RewindManager::RewindSeconds(uint32_t seconds) {
	if (_rewindState == RewindState::Stopped) {
		uint32_t removeCount = (seconds * 60 / RewindManager::BufferSize) + 1;
		auto lock = _emu->AcquireLock();

		for (uint32_t i = 0; i < removeCount; i++) {
			if (!_history.empty()) {
				_currentHistory = _history.back();
				_history.pop_back();
			} else {
				break;
			}
		}
		_currentHistory.LoadState(_emu, _history);
	}
}

bool RewindManager::HasHistory() {
	return _hasHistory;
}

deque<RewindData> RewindManager::GetHistory() {
	deque<RewindData> history = _history;
	history.push_back(_currentHistory);
	return history;
}

void RewindManager::SendFrame(RenderedFrame& frame, bool forRewind) {
	ProcessFrame(frame, forRewind);
}

bool RewindManager::SendAudio(int16_t* soundBuffer, uint32_t sampleCount) {
	return ProcessAudio(soundBuffer, sampleCount);
}
