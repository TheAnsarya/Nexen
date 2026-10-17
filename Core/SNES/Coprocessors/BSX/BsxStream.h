#pragma once
#include "pch.h"
#include "Utilities/ISerializable.h"

class SnesConsole;
class SnesMemoryManager;

/// <summary>
/// BS-X satellite data stream emulation.
/// Emulates a single channel of satellite broadcast data reception.
/// Reads from pre-recorded BS-X data files to simulate live broadcasts.
/// </summary>
/// <remarks>
/// The Satellaview received data on multiple channels with different content types:
/// - Channel 0: Time/date synchronization
/// - Other channels: Game data, audio, news, etc.
/// 
/// Data streams consist of packets with:
/// - Prefix byte (channel/type information)
/// - Data bytes (payload)
/// - Timing synchronized to broadcast schedule
/// 
/// The emulation reads from BS-X data files (.bs) that contain recordings
/// of the original satellite broadcasts. Time-based content uses either
/// real system time or a configurable custom date.
/// </remarks>
class BsxStream : public ISerializable {
private:
	/// <summary>Reference to SNES console.</summary>
	SnesConsole* _console;

	/// <summary>Reference to memory manager for timing.</summary>
	SnesMemoryManager* _memoryManager;

	/// <summary>Input stream for BS-X data file.</summary>
	ifstream _file;

	/// <summary>Current time structure for time-based content.</summary>
	tm _tm = {};

	/// <summary>Currently selected channel number.</summary>
	uint16_t _channel = 0;

	/// <summary>Current prefix byte (packet header).</summary>
	uint8_t _prefix = 0;

	/// <summary>Current data byte.</summary>
	uint8_t _data = 0;

	/// <summary>Stream status register.</summary>
	uint8_t _status = 0;

	/// <summary>Prefix latch state for read sequencing.</summary>
	bool _prefixLatch = false;

	/// <summary>Data latch state for read sequencing.</summary>
	bool _dataLatch = false;

	/// <summary>True if this is the first packet of a transmission.</summary>
	bool _firstPacket = false;

	/// <summary>Current offset in BS-X data file.</summary>
	uint32_t _fileOffset = 0;

	/// <summary>Current file index for multi-file sets.</summary>
	uint8_t _fileIndex = 0;

	/// <summary>Number of bytes in combined queue.</summary>
	uint16_t _queueLength = 0;

	/// <summary>Number of prefix bytes queued.</summary>
	uint8_t _prefixQueueLength = 0;

	/// <summary>Number of data bytes queued.</summary>
	uint8_t _dataQueueLength = 0;

	/// <summary>Active channel for data delivery.</summary>
	uint16_t _activeChannel = 0;

	/// <summary>Active file index for current stream.</summary>
	uint8_t _activeFileIndex = 0;

	/// <summary>Custom date override (-1 for real time).</summary>
	int64_t _resetDate = -1;

	/// <summary>Master clock at last reset for timing.</summary>
	uint64_t _resetMasterClock = 0;

	/// <summary>Opens the BS-X stream data file.</summary>
	void OpenStreamFile();

	/// <summary>
	/// Loads data from stream file.
	/// </summary>
	/// <returns>True if data was loaded successfully.</returns>
	bool LoadStreamFile();

	/// <summary>Initializes time structure from date settings.</summary>
	void InitTimeStruct();

	/// <summary>
	/// Gets current time value for time channel.
	/// </summary>
	/// <returns>Time byte (varies by time field being read).</returns>
	uint8_t GetTime();

public:
	/// <summary>Creates a new BS-X stream instance.</summary>
	BsxStream();

	/// <summary>
	/// Resets stream state.
	/// </summary>
	/// <param name="console">Reference to SNES console.</param>
	/// <param name="customDate">Custom date override (-1 for real time).</param>
	void Reset(SnesConsole* console, int64_t customDate);

	/// <summary>
	/// Gets currently selected channel number.
	/// </summary>
	/// <returns>Channel number (0-65535).</returns>
	uint16_t GetChannel();

	/// <summary>
	/// Checks if stream queues need to be filled.
	/// </summary>
	/// <returns>True if queues are empty and need data.</returns>
	bool NeedUpdate();

	/// <summary>
	/// Fills prefix and data queues from file.
	/// </summary>
	/// <returns>True if data was queued.</returns>
	bool FillQueues();

	/// <summary>
	/// Gets number of prefix bytes available.
	/// </summary>
	/// <returns>Count of queued prefix bytes.</returns>
	uint8_t GetPrefixCount();

	/// <summary>
	/// Gets next prefix byte from queue.
	/// </summary>
	/// <returns>Prefix byte.</returns>
	uint8_t GetPrefix();

	/// <summary>
	/// Gets next data byte from queue.
	/// </summary>
	/// <returns>Data byte.</returns>
	uint8_t GetData();

	/// <summary>
	/// Gets stream status register.
	/// </summary>
	/// <param name="reset">True to clear status after read.</param>
	/// <returns>Status byte.</returns>
	uint8_t GetStatus(bool reset);

	/// <summary>
	/// Sets low byte of channel selection.
	/// </summary>
	/// <param name="value">Low byte of channel number.</param>
	void SetChannelLow(uint8_t value);

	/// <summary>
	/// Sets high byte of channel selection.
	/// </summary>
	/// <param name="value">High byte of channel number.</param>
	void SetChannelHigh(uint8_t value);

	/// <summary>
	/// Sets prefix latch register.
	/// </summary>
	/// <param name="value">Latch control value.</param>
	void SetPrefixLatch(uint8_t value);

	/// <summary>
	/// Sets data latch register.
	/// </summary>
	/// <param name="value">Latch control value.</param>
	void SetDataLatch(uint8_t value);

	/// <summary>Serializes stream state for save states.</summary>
	void Serialize(Serializer& s) override;
};
