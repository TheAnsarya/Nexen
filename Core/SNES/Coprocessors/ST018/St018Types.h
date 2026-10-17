#pragma once
#include "pch.h"
#include "Shared/BaseState.h"
#include "SNES/Coprocessors/ST018/ArmV3Types.h"

/// <summary>
/// State of the ST018 coprocessor communication interface.
/// The ST018 contains an ARM v3 CPU and communicates with the SNES via
/// a simple bidirectional data register protocol.
/// Used in Hayazashi Nidan Morita Shogi 2 for advanced AI calculations.
/// </summary>
struct St018State : BaseState {
	/// <summary>Data is available for SNES to read from ARM.</summary>
	bool HasDataForSnes;

	/// <summary>Data byte waiting to be read by SNES.</summary>
	uint8_t DataSnes;

	/// <summary>Data is available for ARM to read from SNES.</summary>
	bool HasDataForArm;

	/// <summary>Data byte waiting to be read by ARM.</summary>
	uint8_t DataArm;

	/// <summary>ARM CPU reset signal is active.</summary>
	bool ArmReset;

	/// <summary>Acknowledgement flag for handshaking.</summary>
	bool Ack;
};