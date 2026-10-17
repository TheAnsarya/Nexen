#pragma once
#include "pch.h"
#include "Utilities/ISerializable.h"
#include "Utilities/Serializer.h"
#include "ChannelF/ChannelFBiosDatabase.h"

/// <summary>
/// Deterministic Channel F core scaffold used during early bring-up.
///
/// This class is intentionally minimal and side-effect free:
/// - Provides deterministic RunFrame() behavior for test/benchmark harnesses.
/// - Exposes variant selection via BIOS hashes (Fairchild/Luxor System II).
/// - Implements Serialize() to validate save-state plumbing before full core integration.
/// </summary>
class ChannelFCoreScaffold final : public ISerializable {
public:
	static constexpr uint32_t CpuClockHz = 1789773;
	static constexpr double Fps = 60.0;
	static constexpr uint32_t CyclesPerFrame = 29829;

private:
	ChannelFBiosVariant _variant = ChannelFBiosVariant::Unknown;
	uint64_t _masterClock = 0;
	uint32_t _frameCount = 0;
	uint8_t _panelButtons = 0;
	uint8_t _rightController = 0;
	uint8_t _leftController = 0;
	uint32_t _deterministicState = 0x6d2b79f5;

public:
	explicit ChannelFCoreScaffold(ChannelFBiosVariant variant = ChannelFBiosVariant::Unknown);

	void Reset();
	void RunFrame();
	void SetPanelButtons(uint8_t panelButtons);
	void SetRightController(uint8_t value);
	void SetLeftController(uint8_t value);
	void DetectVariantFromHashes(const string& biosSha1, const string& biosMd5);

	[[nodiscard]] ChannelFBiosVariant GetVariant() const { return _variant; }
	[[nodiscard]] uint64_t GetMasterClock() const { return _masterClock; }
	[[nodiscard]] uint32_t GetFrameCount() const { return _frameCount; }
	[[nodiscard]] uint32_t GetDeterministicState() const { return _deterministicState; }

	void Serialize(Serializer& s) override;
};
