#pragma once
#include "pch.h"
#include <memory>
#include <array>
#include "PCE/PceTypes.h"
#include "PCE/PceConstants.h"
#include "PCE/PceVdc.h"
#include "Utilities/ISerializable.h"

class PceVce;
class PceConsole;
class Emulator;

/// <summary>
/// PC Engine Video Priority Controller (VPC) - HuC6202.
/// Manages layer priority and composition for SuperGrafx dual-VDC setups.
/// </summary>
/// <remarks>
/// The VPC controls how two VDC outputs are composited:
/// - Window regions with configurable priority settings
/// - Sprite/background priority per window
/// - Transparent pixel handling
///
/// **Standard PC Engine:** Single VDC, VPC passes through directly.
/// **SuperGrafx:** Dual VDCs with priority composition.
///
/// **Priority Windows:**
/// - 2 configurable rectangular windows
/// - Each window can set BG/sprite priority per VDC
/// - Flexible layer mixing options
/// </remarks>
class PceVpc final : public ISerializable {
public:
	/// <summary>Flag indicating pixel is from sprite layer.</summary>
	static constexpr uint16_t SpritePixelFlag = 0x8000;

	/// <summary>Flag indicating pixel is transparent.</summary>
	static constexpr uint16_t TransparentPixelFlag = 0x4000;

private:
	/// <summary>Primary VDC (always present).</summary>
	PceVdc* _vdc1 = nullptr;

	/// <summary>Secondary VDC (SuperGrafx only).</summary>
	PceVdc* _vdc2 = nullptr;

	/// <summary>Video Color Encoder for palette.</summary>
	PceVce* _vce = nullptr;

	/// <summary>Emulator instance.</summary>
	Emulator* _emu = nullptr;

	/// <summary>Console instance.</summary>
	PceConsole* _console = nullptr;

	/// <summary>Double-buffered frame output.</summary>
	std::array<std::unique_ptr<uint16_t[]>, 2> _outBuffer;

	/// <summary>Current output buffer.</summary>
	uint16_t* _currentOutBuffer = nullptr;

	/// <summary>Horizontal start position.</summary>
	uint16_t _xStart = 0;

	/// <summary>Frame skip timer for performance.</summary>
	Timer _frameSkipTimer;

	/// <summary>Skip rendering this frame.</summary>
	bool _skipRender = false;

	/// <summary>VPC register state.</summary>
	PceVpcState _state = {};

	/// <summary>Sets priority configuration for a window.</summary>
	void SetPriorityConfig(PceVpcPixelWindow wnd, uint8_t value);

	/// <summary>Updates IRQ line state.</summary>
	void UpdateIrqState();

public:
	/// <summary>
	/// Constructs VPC with emulator and VCE references.
	/// </summary>
	PceVpc(Emulator* emu, PceConsole* console, PceVce* vce);
	~PceVpc();

	/// <summary>
	/// Connects VDC(s) to the VPC.
	/// </summary>
	/// <param name="vdc1">Primary VDC.</param>
	/// <param name="vdc2">Secondary VDC (SuperGrafx) or nullptr.</param>
	void ConnectVdc(PceVdc* vdc1, PceVdc* vdc2);

	/// <summary>Reads from VPC register.</summary>
	uint8_t Read(uint16_t addr);

	/// <summary>Writes to VPC register.</summary>
	void Write(uint16_t addr, uint8_t value);

	/// <summary>SuperGrafx ST (shadow) VDC write.</summary>
	void StVdcWrite(uint16_t addr, uint8_t value);

	/// <summary>Executes one VDC cycle (standard PC Engine).</summary>
	__forceinline void Exec() { _vdc1->Exec(); }

	/// <summary>Executes one cycle for both VDCs (SuperGrafx).</summary>
	__forceinline void ExecSuperGrafx() {
		_vdc2->Exec();
		_vdc1->Exec();
	}

	/// <summary>Draws and composites current scanline.</summary>
	void DrawScanline();

	/// <summary>Processes start of frame.</summary>
	void ProcessStartFrame();

	/// <summary>Processes start of scanline for a VDC.</summary>
	void ProcessScanlineStart(PceVdc* vdc, uint16_t scanline);

	/// <summary>Processes current scanline.</summary>
	void ProcessScanline();

	/// <summary>Processes end of scanline for a VDC.</summary>
	void ProcessScanlineEnd(PceVdc* vdc, uint16_t scanline, uint16_t* rowBuffer);

	/// <summary>Sends completed frame to output.</summary>
	void SendFrame(PceVdc* vdc);

	/// <summary>Debug: Force sends current frame.</summary>
	void DebugSendFrame();

	/// <summary>Sets IRQ from VDC.</summary>
	void SetIrq(PceVdc* vdc);

	/// <summary>Clears IRQ from VDC.</summary>
	void ClearIrq(PceVdc* vdc);

	/// <summary>Checks if frame skip is enabled.</summary>
	[[nodiscard]] bool IsSkipRenderEnabled() { return _skipRender; }

	/// <summary>Gets current VPC state.</summary>
	[[nodiscard]] PceVpcState GetState() { return _state; }

	/// <summary>Gets current screen buffer.</summary>
	uint16_t* GetScreenBuffer() { return _currentOutBuffer; }

	/// <summary>Gets previous screen buffer for double-buffering.</summary>
	uint16_t* GetPreviousScreenBuffer() { return _currentOutBuffer == _outBuffer[0].get() ? _outBuffer[1].get() : _outBuffer[0].get(); }

	/// <summary>Serializes VPC state for save states.</summary>
	void Serialize(Serializer& s) override;
};
