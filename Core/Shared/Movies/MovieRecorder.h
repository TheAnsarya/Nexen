#pragma once
#include "pch.h"
#include <deque>
#include <unordered_map>
#include "Shared/Interfaces/IInputRecorder.h"
#include "Shared/Interfaces/INotificationListener.h"
#include "Shared/BatteryManager.h"
#include "Shared/RewindData.h"
#include "Shared/Movies/MovieTypes.h"

class ZipWriter;
class Emulator;

class MovieRecorder final : public INotificationListener, public IInputRecorder, public IBatteryRecorder, public IBatteryProvider, public std::enable_shared_from_this<MovieRecorder> {
private:
	static constexpr uint32_t MovieFormatVersion = 2;

	Emulator* _emu;
	string _filename;
	string _author;
	string _description;
	unique_ptr<ZipWriter> _writer;
	std::unordered_map<string, vector<uint8_t>> _batteryData;
	stringstream _inputData;
	string _textStateBuf;
	bool _hasSaveState = false;
	stringstream _saveStateData;
	
	// TAS features
	uint32_t _rerecordCount = 0;
	uint32_t _frameCount = 0;
	vector<string> _inputLines;  // Store input lines for rerecording support
	bool _tasMode = false;       // Enable TAS rerecording mode

	void GetGameSettings(stringstream& out);
	void WriteString(stringstream& out, string_view name, string_view value);
	void WriteInt(stringstream& out, string_view name, uint32_t value);
	void WriteBool(stringstream& out, string_view name, bool enabled);

public:
	MovieRecorder(Emulator* emu);
	virtual ~MovieRecorder();

	[[nodiscard]] bool Record(const RecordMovieOptions& options);
	[[nodiscard]] bool Stop();

	// TAS features
	void IncrementRerecordCount();
	uint32_t GetRerecordCount() const { return _rerecordCount; }
	uint32_t GetFrameCount() const { return _frameCount; }
	void SetRerecordCount(uint32_t count) { _rerecordCount = count; }
	void SetTasMode(bool enabled) { _tasMode = enabled; }
	bool IsTasMode() const { return _tasMode; }
	
	// Rerecording support - called when loading a savestate during recording
	bool HandleRerecord(uint32_t frameNumber);
	void TruncateInputToFrame(uint32_t frameNumber);

	// Inherited via IInputRecorder
	void RecordInput(const vector<shared_ptr<BaseControlDevice>>& devices) override;

	// Inherited via IBatteryRecorder
	void OnLoadBattery(const string& extension, vector<uint8_t> batteryData) override;
	vector<uint8_t> LoadBattery(const string& extension) override;

	// Inherited via INotificationListener
	void ProcessNotification(ConsoleNotificationType type, void* parameter) override;

	[[nodiscard]] bool CreateMovie(const string& movieFile, deque<RewindData>& data, uint32_t startPosition, uint32_t endPosition, bool hasBattery);
};
