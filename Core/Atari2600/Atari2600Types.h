#pragma once
#include "pch.h"
#include "Shared/BaseState.h"
#include "Debugger/DebugTypes.h"

namespace Atari2600Constants {
	static constexpr uint32_t ScreenWidth = 160;
	static constexpr uint32_t ScreenHeight = 192;
	static constexpr uint32_t CpuCyclesPerFrame = 19912;
}

/// <summary>
/// Atari 2600 6507 addressing modes.
/// The 6507 is a 6502 with only 13 address lines (8KB address space).
/// Same instruction set as 6502 but limited address range.
/// </summary>
enum class Atari2600AddrMode : uint8_t {
	None,   ///< Not used / illegal
	Acc,    ///< Accumulator (A)
	Imp,    ///< Implied
	Imm,    ///< Immediate (#$nn)
	Rel,    ///< Relative (branch target)
	Zero,   ///< Zero page ($nn)
	Abs,    ///< Absolute ($nnnn)
	ZeroX,  ///< Zero page,X ($nn,X)
	ZeroY,  ///< Zero page,Y ($nn,Y)
	Ind,    ///< Indirect (JMP ($nnnn))
	IndX,   ///< (Indirect,X) (($nn,X))
	IndY,   ///< (Indirect),Y (($nn),Y)
	IndYW,  ///< (Indirect),Y with extra cycle on page cross
	AbsX,   ///< Absolute,X ($nnnn,X)
	AbsXW,  ///< Absolute,X with extra cycle on page cross
	AbsY,   ///< Absolute,Y ($nnnn,Y)
	AbsYW   ///< Absolute,Y with extra cycle on page cross
};

/// <summary>
/// Atari 2600 6507 CPU register state.
/// The 6507 is a cut-down 6502 with 13 address pins (A0-A12)
/// giving a 8KB ($0000-$1FFF) address space.
/// </summary>
struct Atari2600CpuState : BaseState {
	uint64_t CycleCount = 0; ///< Total CPU cycles executed
	uint16_t PC = 0;         ///< Program counter (only low 13 bits used on bus)
	uint8_t SP = 0;          ///< Stack pointer
	uint8_t A = 0;           ///< Accumulator
	uint8_t X = 0;           ///< X index register
	uint8_t Y = 0;           ///< Y index register
	uint8_t PS = 0;          ///< Processor status (N V - B D I Z C)
	uint8_t IRQFlag = 0;     ///< IRQ pending flag (6507 has no IRQ pin, but BRK sets this)
	bool NmiFlag = false;    ///< NMI pending flag (6507 has no NMI pin — always false)
};

struct Atari2600RiotState {
	uint8_t PortA = 0;
	uint8_t PortB = 0;
	uint8_t PortADirection = 0;
	uint8_t PortBDirection = 0;
	uint8_t PortAInput = 0;
	uint8_t PortBInput = 0;
	uint16_t Timer = 0;
	uint16_t TimerDivider = 1;
	uint16_t TimerDividerCounter = 1;
	bool TimerUnderflow = false;
	bool InterruptFlag = false;
	uint32_t InterruptEdgeCount = 0;
	uint64_t CpuCycles = 0;
};

struct Atari2600TiaState {
	uint32_t FrameCount = 0;
	uint32_t Scanline = 0;
	uint32_t ColorClock = 0;
	bool WsyncHold = false;
	uint32_t WsyncCount = 0;
	bool HmovePending = false;
	bool HmoveDelayToNextScanline = false;
	uint32_t HmoveStrobeCount = 0;
	uint32_t HmoveApplyCount = 0;
	uint8_t ColorBackground = 0;
	uint8_t ColorPlayfield = 0;
	uint8_t ColorPlayer0 = 0;
	uint8_t ColorPlayer1 = 0;
	uint8_t Playfield0 = 0;
	uint8_t Playfield1 = 0;
	uint8_t Playfield2 = 0;
	bool PlayfieldReflect = false;
	bool PlayfieldScoreMode = false;
	bool PlayfieldPriority = false;
	uint8_t BallSize = 1;
	uint8_t Nusiz0 = 0;
	uint8_t Nusiz1 = 0;
	uint8_t Player0Graphics = 0;
	uint8_t Player1Graphics = 0;
	bool Player0Reflect = false;
	bool Player1Reflect = false;
	bool Missile0ResetToPlayer = false;
	bool Missile1ResetToPlayer = false;
	bool Missile0Enabled = false;
	bool Missile1Enabled = false;
	bool BallEnabled = false;
	bool VdelPlayer0 = false;
	bool VdelPlayer1 = false;
	bool VdelBall = false;
	uint8_t DelayedPlayer0Graphics = 0;
	uint8_t DelayedPlayer1Graphics = 0;
	bool DelayedBallEnabled = false;
	uint8_t Player0X = 24;
	uint8_t Player1X = 96;
	uint8_t Missile0X = 32;
	uint8_t Missile1X = 104;
	uint8_t BallX = 80;
	int8_t MotionPlayer0 = 0;
	int8_t MotionPlayer1 = 0;
	int8_t MotionMissile0 = 0;
	int8_t MotionMissile1 = 0;
	int8_t MotionBall = 0;
	uint8_t CollisionCxm0p = 0;
	uint8_t CollisionCxm1p = 0;
	uint8_t CollisionCxp0fb = 0;
	uint8_t CollisionCxp1fb = 0;
	uint8_t CollisionCxm0fb = 0;
	uint8_t CollisionCxm1fb = 0;
	uint8_t CollisionCxblpf = 0;
	uint8_t CollisionCxppmm = 0;
	uint32_t RenderRevision = 0;
	uint8_t AudioControl0 = 0;
	uint8_t AudioControl1 = 0;
	uint8_t AudioFrequency0 = 0;
	uint8_t AudioFrequency1 = 0;
	uint8_t AudioVolume0 = 0;
	uint8_t AudioVolume1 = 0;
	uint16_t AudioCounter0 = 1;
	uint16_t AudioCounter1 = 1;
	uint8_t AudioPhase0 = 0;
	uint8_t AudioPhase1 = 0;
	uint8_t LastMixedSample = 0;
	uint64_t AudioMixAccumulator = 0;
	uint64_t AudioSampleCount = 0;
	uint32_t AudioRevision = 0;
	uint64_t TotalColorClocks = 0;
};

struct Atari2600ScanlineRenderState {
	uint8_t ColorBackground = 0;
	uint8_t ColorPlayfield = 0;
	uint8_t ColorPlayer0 = 0;
	uint8_t ColorPlayer1 = 0;
	uint8_t Playfield0 = 0;
	uint8_t Playfield1 = 0;
	uint8_t Playfield2 = 0;
	bool PlayfieldReflect = false;
	bool PlayfieldScoreMode = false;
	bool PlayfieldPriority = false;
	uint8_t BallSize = 1;
	uint8_t Nusiz0 = 0;
	uint8_t Nusiz1 = 0;
	uint8_t Player0Graphics = 0;
	uint8_t Player1Graphics = 0;
	bool Player0Reflect = false;
	bool Player1Reflect = false;
	bool Missile0Enabled = false;
	bool Missile1Enabled = false;
	bool BallEnabled = false;
	uint8_t Player0X = 24;
	uint8_t Player1X = 96;
	uint8_t Missile0X = 32;
	uint8_t Missile1X = 104;
	uint8_t BallX = 80;
};

struct Atari2600FrameStepSummary {
	uint32_t FrameCount = 0;
	uint32_t CpuCyclesThisFrame = 0;
	uint32_t ScanlineAtFrameEnd = 0;
	uint32_t ColorClockAtFrameEnd = 0;
};

struct Atari2600State : BaseState {
	Atari2600CpuState Cpu = {};
	Atari2600TiaState Tia = {};
	Atari2600RiotState Riot = {};
};
