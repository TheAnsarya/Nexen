#pragma once

#include <array>
#include <cstdint>
#include <optional>
#include <utility>
#include "Shared/CpuType.h"
#include "Shared/EventType.h"
#include "Shared/SettingTypes.h"
#include "Debugger/DebugTypes.h"

enum class CpuStateLayout : uint8_t {
	SnesCpu,
	Spc,
	NecDsp,
	Gsu,
	Cx4,
	ArmV3,
	GbCpu,
	NesCpu,
	PceCpu,
	SmsCpu,
	GbaCpu,
	WsCpu,
	LynxCpu,
	Atari2600Cpu,
	ChannelFCpu,
	GenesisM68k,
	Unknown
};

enum class PpuStateBackend : uint8_t {
	Snes,
	Gameboy,
	Nes,
	Pce,
	Sms,
	Gba,
	Ws,
	Lynx,
	Atari2600,
	ChannelF,
	None
};

struct CpuDispatchMetadata {
	CpuType Type;
	CpuStateLayout StateLayout;
	std::optional<DebuggerFlags> DebuggerFlag;
	PpuStateBackend PpuBackend;
};

static constexpr std::array<CpuDispatchMetadata, 17> _cpuDispatchMetadata = {{
	{CpuType::Snes, CpuStateLayout::SnesCpu, DebuggerFlags::SnesDebuggerEnabled, PpuStateBackend::Snes},
	{CpuType::Spc, CpuStateLayout::Spc, DebuggerFlags::SpcDebuggerEnabled, PpuStateBackend::Snes},
	{CpuType::NecDsp, CpuStateLayout::NecDsp, DebuggerFlags::NecDspDebuggerEnabled, PpuStateBackend::Snes},
	{CpuType::Sa1, CpuStateLayout::SnesCpu, DebuggerFlags::Sa1DebuggerEnabled, PpuStateBackend::Snes},
	{CpuType::Gsu, CpuStateLayout::Gsu, DebuggerFlags::GsuDebuggerEnabled, PpuStateBackend::Snes},
	{CpuType::Cx4, CpuStateLayout::Cx4, DebuggerFlags::Cx4DebuggerEnabled, PpuStateBackend::Snes},
	{CpuType::St018, CpuStateLayout::ArmV3, DebuggerFlags::St018DebuggerEnabled, PpuStateBackend::Snes},
	{CpuType::Gameboy, CpuStateLayout::GbCpu, DebuggerFlags::GbDebuggerEnabled, PpuStateBackend::Gameboy},
	{CpuType::Nes, CpuStateLayout::NesCpu, DebuggerFlags::NesDebuggerEnabled, PpuStateBackend::Nes},
	{CpuType::Pce, CpuStateLayout::PceCpu, DebuggerFlags::PceDebuggerEnabled, PpuStateBackend::Pce},
	{CpuType::Sms, CpuStateLayout::SmsCpu, DebuggerFlags::SmsDebuggerEnabled, PpuStateBackend::Sms},
	{CpuType::Gba, CpuStateLayout::GbaCpu, DebuggerFlags::GbaDebuggerEnabled, PpuStateBackend::Gba},
	{CpuType::Ws, CpuStateLayout::WsCpu, DebuggerFlags::WsDebuggerEnabled, PpuStateBackend::Ws},
	{CpuType::Lynx, CpuStateLayout::LynxCpu, DebuggerFlags::LynxDebuggerEnabled, PpuStateBackend::Lynx},
	{CpuType::Genesis, CpuStateLayout::GenesisM68k, std::nullopt, PpuStateBackend::None},
	{CpuType::Atari2600, CpuStateLayout::Atari2600Cpu, DebuggerFlags::Atari2600DebuggerEnabled, PpuStateBackend::Atari2600},
	{CpuType::ChannelF, CpuStateLayout::ChannelFCpu, DebuggerFlags::ChannelFDebuggerEnabled, PpuStateBackend::ChannelF}
}};

[[nodiscard]] inline constexpr const CpuDispatchMetadata* TryGetCpuDispatchMetadata(CpuType cpuType) {
	for(const CpuDispatchMetadata& metadata : _cpuDispatchMetadata) {
		if(metadata.Type == cpuType) {
			return &metadata;
		}
	}

	return nullptr;
}

[[nodiscard]] inline CpuStateLayout GetCpuStateLayout(CpuType cpuType) {
	if(const CpuDispatchMetadata* metadata = TryGetCpuDispatchMetadata(cpuType)) {
		return metadata->StateLayout;
	}

	return CpuStateLayout::Unknown;
}

[[nodiscard]] inline std::optional<DebuggerFlags> GetDebuggerFlagForCpu(CpuType cpuType) {
	if(const CpuDispatchMetadata* metadata = TryGetCpuDispatchMetadata(cpuType)) {
		return metadata->DebuggerFlag;
	}

	return std::nullopt;
}

[[nodiscard]] inline PpuStateBackend GetPpuStateBackendForCpu(CpuType cpuType) {
	if(const CpuDispatchMetadata* metadata = TryGetCpuDispatchMetadata(cpuType)) {
		return metadata->PpuBackend;
	}

	return PpuStateBackend::None;
}

[[nodiscard]] inline CpuType ResolveEventCpuType(CpuType requestedCpuType, CpuType mainCpuType, bool hasRequestedCpuType) {
	return hasRequestedCpuType ? requestedCpuType : mainCpuType;
}

[[nodiscard]] inline bool ShouldFallbackToMainInputDebugger(bool hasRoutedInputDebugger, bool hasMainInputDebugger) {
	return !hasRoutedInputDebugger && hasMainInputDebugger;
}

struct ProcessEventDispatchContext {
	bool DebuggerOwnsInstance = false;
	bool HasRoutedInputDebugger = false;
	bool HasMainInputDebugger = false;
	bool DebuggerBlocked = false;
	bool HasRoutedEventManager = false;
};

struct ProcessEventDispatchOutcome {
	bool ShouldDispatchScriptEvent = false;
	std::optional<CpuType> InputDebuggerCpuType = std::nullopt;
	bool ShouldSendEventViewerRefresh = false;
	bool ShouldClearFrameEvents = false;
};

enum class SleepUntilResumeDecision : uint8_t {
	Continue,
	SkipForSuspendRequest,
	SkipForExecutionAlreadyStopped,
	SkipForBreakRequestMainCpuBoundary,
	SkipForForbiddenBreakpoint
};

struct SleepUntilResumeGuardContext {
	bool HasSuspendRequest = false;
	bool ExecutionAlreadyStopped = false;
	bool HasBreakRequest = false;
	bool SourceCpuIsMainCpu = false;
	bool AllowChangeProgramCounter = false;
	bool BreakpointForbidden = false;
};

[[nodiscard]] inline SleepUntilResumeGuardContext BuildSleepUntilResumeGuardContext(bool hasSuspendRequest, bool executionAlreadyStopped, bool hasBreakRequest, bool sourceCpuIsMainCpu, bool allowChangeProgramCounter, bool breakpointForbidden) {
	SleepUntilResumeGuardContext context = {};
	context.HasSuspendRequest = hasSuspendRequest;
	context.ExecutionAlreadyStopped = executionAlreadyStopped;
	context.HasBreakRequest = hasBreakRequest;
	context.SourceCpuIsMainCpu = sourceCpuIsMainCpu;
	context.AllowChangeProgramCounter = allowChangeProgramCounter;
	context.BreakpointForbidden = breakpointForbidden;
	return context;
}

[[nodiscard]] inline SleepUntilResumeDecision EvaluateSleepUntilResumeDecision(const SleepUntilResumeGuardContext& context) {
	if (context.HasSuspendRequest) {
		return SleepUntilResumeDecision::SkipForSuspendRequest;
	}

	if (context.ExecutionAlreadyStopped) {
		return SleepUntilResumeDecision::SkipForExecutionAlreadyStopped;
	}

	if (context.HasBreakRequest && (!context.SourceCpuIsMainCpu || !context.AllowChangeProgramCounter)) {
		return SleepUntilResumeDecision::SkipForBreakRequestMainCpuBoundary;
	}

	if (context.BreakpointForbidden) {
		return SleepUntilResumeDecision::SkipForForbiddenBreakpoint;
	}

	return SleepUntilResumeDecision::Continue;
}

[[nodiscard]] inline bool ShouldEmitSleepUntilResumeBreakNotification(BreakSource source, bool hasBreakRequest) {
	return source != BreakSource::Unspecified || !hasBreakRequest;
}

[[nodiscard]] inline int32_t GetSleepUntilResumeWaitDelayMs(bool hasBreakRequest) {
	return hasBreakRequest ? 1 : 10;
}

struct SleepUntilResumePreBreakContext {
	bool ShouldEmitBreakNotification = false;
	bool SingleBreakpointPerInstruction = false;
	bool DrawPartialFrame = false;
};

struct SleepUntilResumePreBreakOutcome {
	bool ShouldIgnoreBreakpoints = false;
	bool ShouldDrawPartialFrame = false;
};

[[nodiscard]] inline SleepUntilResumePreBreakOutcome ResolveSleepUntilResumePreBreakOutcome(const SleepUntilResumePreBreakContext& context) {
	SleepUntilResumePreBreakOutcome outcome = {};
	if (!context.ShouldEmitBreakNotification) {
		return outcome;
	}

	outcome.ShouldIgnoreBreakpoints = context.SingleBreakpointPerInstruction;
	outcome.ShouldDrawPartialFrame = context.DrawPartialFrame;
	return outcome;
}


struct SleepUntilResumePreLoopContext {
	bool ShouldEmitBreakNotification = false;
};

struct SleepUntilResumePreLoopOutcome {
	bool ShouldRunPreBreakSequence = false;
	bool ShouldArmWaitForBreakResume = false;
	bool ShouldEnableScreensaver = false;
};

[[nodiscard]] inline SleepUntilResumePreLoopOutcome ResolveSleepUntilResumePreLoopOutcome(const SleepUntilResumePreLoopContext& context) {
	SleepUntilResumePreLoopOutcome outcome = {};
	outcome.ShouldRunPreBreakSequence = context.ShouldEmitBreakNotification;
	outcome.ShouldArmWaitForBreakResume = context.ShouldEmitBreakNotification;
	outcome.ShouldEnableScreensaver = context.ShouldEmitBreakNotification;
	return outcome;
}

struct SleepUntilResumeLoopContext {
	bool WaitForBreakResume = false;
	bool HasSuspendRequest = false;
	bool HasBreakRequest = false;
};

struct SleepUntilResumeLoopOutcome {
	bool ShouldContinueWaiting = false;
	int32_t WaitDelayMs = 10;
};

[[nodiscard]] inline SleepUntilResumeLoopOutcome ResolveSleepUntilResumeLoopOutcome(const SleepUntilResumeLoopContext& context) {
	SleepUntilResumeLoopOutcome outcome = {};
	outcome.ShouldContinueWaiting = (context.WaitForBreakResume && !context.HasSuspendRequest) || context.HasBreakRequest;
	outcome.WaitDelayMs = GetSleepUntilResumeWaitDelayMs(context.HasBreakRequest);
	return outcome;
}

struct SleepUntilResumePostLoopContext {
	bool NotificationSent = false;
};

struct SleepUntilResumePostLoopOutcome {
	bool ShouldDisableScreensaver = false;
	bool ShouldSendDebuggerResumedNotification = false;
};

[[nodiscard]] inline SleepUntilResumePostLoopOutcome ResolveSleepUntilResumePostLoopOutcome(const SleepUntilResumePostLoopContext& context) {
	SleepUntilResumePostLoopOutcome outcome = {};
	outcome.ShouldDisableScreensaver = context.NotificationSent;
	outcome.ShouldSendDebuggerResumedNotification = context.NotificationSent;
	return outcome;
}

struct SleepUntilResumeBreakEventContext {
	CpuType SourceCpu = CpuType::Snes;
	BreakSource Source = BreakSource::Unspecified;
	int32_t BreakpointId = -1;
	const MemoryOperationInfo* Operation = nullptr;
};

struct SleepUntilResumeBreakEventOutcome {
	BreakEvent Event = {};
	bool HasOperation = false;
};

[[nodiscard]] inline SleepUntilResumeBreakEventOutcome ResolveSleepUntilResumeBreakEventOutcome(const SleepUntilResumeBreakEventContext& context) {
	SleepUntilResumeBreakEventOutcome outcome = {};
	outcome.Event.SourceCpu = context.SourceCpu;
	outcome.Event.Source = context.Source;
	outcome.Event.BreakpointId = context.BreakpointId;

	if (context.Operation) {
		outcome.Event.Operation = *context.Operation;
		outcome.HasOperation = true;
	}

	return outcome;
}

struct SleepUntilResumeDispatchContext {
	bool ShouldRunPreBreakSequence = false;
};

struct SleepUntilResumeDispatchOutcome {
	bool ShouldDispatchCodeBreakNotification = false;
	bool ShouldProcessCodeBreakEvent = false;
	bool ShouldMarkNotificationSent = false;
};

[[nodiscard]] inline SleepUntilResumeDispatchOutcome ResolveSleepUntilResumeDispatchOutcome(const SleepUntilResumeDispatchContext& context) {
	SleepUntilResumeDispatchOutcome outcome = {};
	outcome.ShouldDispatchCodeBreakNotification = context.ShouldRunPreBreakSequence;
	outcome.ShouldProcessCodeBreakEvent = context.ShouldRunPreBreakSequence;
	outcome.ShouldMarkNotificationSent = context.ShouldRunPreBreakSequence;
	return outcome;
}

struct SleepUntilResumeRuntimeDispatchContext {
	bool ShouldRunPreBreakSequence = false;
	CpuType SourceCpu = CpuType::Snes;
	BreakSource Source = BreakSource::Unspecified;
	int32_t BreakpointId = -1;
	const MemoryOperationInfo* Operation = nullptr;
};

struct SleepUntilResumeRuntimeDispatchOutcome {
	SleepUntilResumeBreakEventOutcome BreakEvent = {};
	SleepUntilResumeDispatchOutcome Dispatch = {};
};

[[nodiscard]] inline SleepUntilResumeRuntimeDispatchOutcome ResolveSleepUntilResumeRuntimeDispatchOutcome(const SleepUntilResumeRuntimeDispatchContext& context) {
	SleepUntilResumeRuntimeDispatchOutcome outcome = {};

	SleepUntilResumeBreakEventContext breakEventContext = {};
	breakEventContext.SourceCpu = context.SourceCpu;
	breakEventContext.Source = context.Source;
	breakEventContext.BreakpointId = context.BreakpointId;
	breakEventContext.Operation = context.Operation;
	outcome.BreakEvent = ResolveSleepUntilResumeBreakEventOutcome(breakEventContext);

	SleepUntilResumeDispatchContext dispatchContext = {};
	dispatchContext.ShouldRunPreBreakSequence = context.ShouldRunPreBreakSequence;
	outcome.Dispatch = ResolveSleepUntilResumeDispatchOutcome(dispatchContext);

	return outcome;
}

struct SleepUntilResumeRuntimeSideEffectContext {
	bool ShouldArmWaitForBreakResume = false;
	bool ShouldEnableScreensaver = false;
	bool ShouldMarkNotificationSent = false;
	bool NotificationSent = false;
};

struct SleepUntilResumeRuntimeSideEffectOutcome {
	bool ShouldSetWaitForBreakResume = false;
	bool ShouldEnableScreensaver = false;
	bool NotificationSent = false;
};

[[nodiscard]] inline SleepUntilResumeRuntimeSideEffectOutcome ResolveSleepUntilResumeRuntimeSideEffectOutcome(const SleepUntilResumeRuntimeSideEffectContext& context) {
	SleepUntilResumeRuntimeSideEffectOutcome outcome = {};
	outcome.ShouldSetWaitForBreakResume = context.ShouldArmWaitForBreakResume;
	outcome.ShouldEnableScreensaver = context.ShouldEnableScreensaver;
	outcome.NotificationSent = context.NotificationSent || context.ShouldMarkNotificationSent;
	return outcome;
}

struct SleepUntilResumeRuntimeBundleContext {
	bool ShouldRunPreBreakSequence = false;
	CpuType SourceCpu = CpuType::Snes;
	BreakSource Source = BreakSource::Unspecified;
	int32_t BreakpointId = -1;
	const MemoryOperationInfo* Operation = nullptr;
	bool ShouldArmWaitForBreakResume = false;
	bool ShouldEnableScreensaver = false;
	bool NotificationSent = false;
};

struct SleepUntilResumeRuntimeBundleOutcome {
	SleepUntilResumeRuntimeDispatchOutcome Dispatch = {};
	SleepUntilResumeRuntimeSideEffectOutcome SideEffect = {};
};

struct SleepUntilResumeRuntimeSideEffectApplicationContext {
	bool WaitForBreakResume = false;
	bool NotificationSent = false;
	SleepUntilResumeRuntimeSideEffectOutcome SideEffect = {};
};

struct SleepUntilResumeRuntimeSideEffectApplicationOutcome {
	bool WaitForBreakResume = false;
	bool NotificationSent = false;
	bool ShouldEnableScreensaver = false;
};

struct SleepUntilResumeRuntimeDispatchExecutionContext {
	SleepUntilResumeRuntimeDispatchOutcome Dispatch = {};
};

struct SleepUntilResumeRuntimeDispatchExecutionOutcome {
	bool ShouldDispatchCodeBreakNotification = false;
	bool ShouldProcessCodeBreakEvent = false;
	BreakEvent Event = {};
};

[[nodiscard]] inline SleepUntilResumeRuntimeDispatchContext BuildSleepUntilResumeRuntimeDispatchContext(const SleepUntilResumeRuntimeBundleContext& context) {
	SleepUntilResumeRuntimeDispatchContext dispatchContext = {};
	dispatchContext.ShouldRunPreBreakSequence = context.ShouldRunPreBreakSequence;
	dispatchContext.SourceCpu = context.SourceCpu;
	dispatchContext.Source = context.Source;
	dispatchContext.BreakpointId = context.BreakpointId;
	dispatchContext.Operation = context.Operation;
	return dispatchContext;
}

[[nodiscard]] inline SleepUntilResumeRuntimeSideEffectContext BuildSleepUntilResumeRuntimeSideEffectContext(const SleepUntilResumeRuntimeBundleContext& context, bool shouldMarkNotificationSent) {
	SleepUntilResumeRuntimeSideEffectContext sideEffectContext = {};
	sideEffectContext.ShouldArmWaitForBreakResume = context.ShouldArmWaitForBreakResume;
	sideEffectContext.ShouldEnableScreensaver = context.ShouldEnableScreensaver;
	sideEffectContext.ShouldMarkNotificationSent = shouldMarkNotificationSent;
	sideEffectContext.NotificationSent = context.NotificationSent;
	return sideEffectContext;
}

[[nodiscard]] inline SleepUntilResumeRuntimeBundleOutcome ResolveSleepUntilResumeRuntimeBundleOutcome(const SleepUntilResumeRuntimeBundleContext& context) {
	SleepUntilResumeRuntimeBundleOutcome outcome = {};

	SleepUntilResumeRuntimeDispatchContext dispatchContext = BuildSleepUntilResumeRuntimeDispatchContext(context);
	outcome.Dispatch = ResolveSleepUntilResumeRuntimeDispatchOutcome(dispatchContext);

	SleepUntilResumeRuntimeSideEffectContext sideEffectContext = BuildSleepUntilResumeRuntimeSideEffectContext(context, outcome.Dispatch.Dispatch.ShouldMarkNotificationSent);
	outcome.SideEffect = ResolveSleepUntilResumeRuntimeSideEffectOutcome(sideEffectContext);

	return outcome;
}

[[nodiscard]] inline SleepUntilResumeRuntimeSideEffectApplicationContext BuildSleepUntilResumeRuntimeSideEffectApplicationContext(bool waitForBreakResume, bool notificationSent, const SleepUntilResumeRuntimeBundleOutcome& runtimeBundleOutcome) {
	SleepUntilResumeRuntimeSideEffectApplicationContext context = {};
	context.WaitForBreakResume = waitForBreakResume;
	context.NotificationSent = notificationSent;
	context.SideEffect = runtimeBundleOutcome.SideEffect;
	return context;
}

[[nodiscard]] inline SleepUntilResumeRuntimeSideEffectApplicationOutcome ResolveSleepUntilResumeRuntimeSideEffectApplicationOutcome(const SleepUntilResumeRuntimeSideEffectApplicationContext& context) {
	SleepUntilResumeRuntimeSideEffectApplicationOutcome outcome = {};
	outcome.WaitForBreakResume = context.WaitForBreakResume || context.SideEffect.ShouldSetWaitForBreakResume;
	outcome.NotificationSent = context.NotificationSent || context.SideEffect.NotificationSent;
	outcome.ShouldEnableScreensaver = context.SideEffect.ShouldEnableScreensaver;
	return outcome;
}

[[nodiscard]] inline SleepUntilResumeRuntimeDispatchExecutionContext BuildSleepUntilResumeRuntimeDispatchExecutionContext(const SleepUntilResumeRuntimeBundleOutcome& runtimeBundleOutcome) {
	SleepUntilResumeRuntimeDispatchExecutionContext context = {};
	context.Dispatch = runtimeBundleOutcome.Dispatch;
	return context;
}

[[nodiscard]] inline SleepUntilResumeRuntimeDispatchExecutionOutcome ResolveSleepUntilResumeRuntimeDispatchExecutionOutcome(const SleepUntilResumeRuntimeDispatchExecutionContext& context) {
	SleepUntilResumeRuntimeDispatchExecutionOutcome outcome = {};
	outcome.ShouldDispatchCodeBreakNotification = context.Dispatch.Dispatch.ShouldDispatchCodeBreakNotification;
	outcome.ShouldProcessCodeBreakEvent = context.Dispatch.Dispatch.ShouldProcessCodeBreakEvent;
	outcome.Event = context.Dispatch.BreakEvent.Event;
	return outcome;
}

struct SleepUntilResumeLoopPostBundleContext {
	bool WaitForBreakResume = false;
	bool HasSuspendRequest = false;
	bool HasBreakRequest = false;
	bool NotificationSent = false;
};

struct SleepUntilResumeLoopPostBundleOutcome {
	SleepUntilResumeLoopOutcome Loop = {};
	SleepUntilResumePostLoopOutcome PostLoop = {};
};

[[nodiscard]] inline SleepUntilResumeLoopContext BuildSleepUntilResumeLoopContext(const SleepUntilResumeLoopPostBundleContext& context) {
	SleepUntilResumeLoopContext loopContext = {};
	loopContext.WaitForBreakResume = context.WaitForBreakResume;
	loopContext.HasSuspendRequest = context.HasSuspendRequest;
	loopContext.HasBreakRequest = context.HasBreakRequest;
	return loopContext;
}

[[nodiscard]] inline SleepUntilResumePostLoopContext BuildSleepUntilResumePostLoopContext(const SleepUntilResumeLoopPostBundleContext& context) {
	SleepUntilResumePostLoopContext postLoopContext = {};
	postLoopContext.NotificationSent = context.NotificationSent;
	return postLoopContext;
}

[[nodiscard]] inline SleepUntilResumeLoopPostBundleOutcome ResolveSleepUntilResumeLoopPostBundleOutcome(const SleepUntilResumeLoopPostBundleContext& context) {
	SleepUntilResumeLoopPostBundleOutcome outcome = {};

	SleepUntilResumeLoopContext loopContext = BuildSleepUntilResumeLoopContext(context);
	outcome.Loop = ResolveSleepUntilResumeLoopOutcome(loopContext);

	SleepUntilResumePostLoopContext postLoopContext = BuildSleepUntilResumePostLoopContext(context);
	outcome.PostLoop = ResolveSleepUntilResumePostLoopOutcome(postLoopContext);

	return outcome;
}

struct SleepUntilResumePreLoopBundleContext {
	bool ShouldEmitBreakNotification = false;
	bool SingleBreakpointPerInstruction = false;
	bool DrawPartialFrame = false;
};

struct SleepUntilResumePreLoopBundleOutcome {
	SleepUntilResumePreBreakOutcome PreBreak = {};
	SleepUntilResumePreLoopOutcome PreLoop = {};
	SleepUntilResumeDispatchOutcome Dispatch = {};
};

[[nodiscard]] inline SleepUntilResumePreBreakContext BuildSleepUntilResumePreBreakContext(const SleepUntilResumePreLoopBundleContext& context) {
	SleepUntilResumePreBreakContext preBreakContext = {};
	preBreakContext.ShouldEmitBreakNotification = context.ShouldEmitBreakNotification;
	preBreakContext.SingleBreakpointPerInstruction = context.SingleBreakpointPerInstruction;
	preBreakContext.DrawPartialFrame = context.DrawPartialFrame;
	return preBreakContext;
}

[[nodiscard]] inline SleepUntilResumePreLoopContext BuildSleepUntilResumePreLoopContext(const SleepUntilResumePreLoopBundleContext& context) {
	SleepUntilResumePreLoopContext preLoopContext = {};
	preLoopContext.ShouldEmitBreakNotification = context.ShouldEmitBreakNotification;
	return preLoopContext;
}

[[nodiscard]] inline SleepUntilResumeDispatchContext BuildSleepUntilResumeDispatchContext(const SleepUntilResumePreLoopOutcome& preLoopOutcome) {
	SleepUntilResumeDispatchContext dispatchContext = {};
	dispatchContext.ShouldRunPreBreakSequence = preLoopOutcome.ShouldRunPreBreakSequence;
	return dispatchContext;
}

[[nodiscard]] inline SleepUntilResumePreLoopBundleOutcome ResolveSleepUntilResumePreLoopBundleOutcome(const SleepUntilResumePreLoopBundleContext& context) {
	SleepUntilResumePreLoopBundleOutcome outcome = {};

	SleepUntilResumePreBreakContext preBreakContext = BuildSleepUntilResumePreBreakContext(context);
	outcome.PreBreak = ResolveSleepUntilResumePreBreakOutcome(preBreakContext);

	SleepUntilResumePreLoopContext preLoopContext = BuildSleepUntilResumePreLoopContext(context);
	outcome.PreLoop = ResolveSleepUntilResumePreLoopOutcome(preLoopContext);

	SleepUntilResumeDispatchContext dispatchContext = BuildSleepUntilResumeDispatchContext(outcome.PreLoop);
	outcome.Dispatch = ResolveSleepUntilResumeDispatchOutcome(dispatchContext);

	return outcome;
}

struct SleepUntilResumePhaseContext {
	SleepUntilResumeGuardContext Guard = {};
	BreakSource Source = BreakSource::Unspecified;
	bool HasBreakRequest = false;
	bool SingleBreakpointPerInstruction = false;
	bool DrawPartialFrame = false;
	bool WaitForBreakResume = false;
	bool HasSuspendRequest = false;
	bool NotificationSent = false;
};

struct SleepUntilResumePhaseOutcome {
	SleepUntilResumeDecision Decision = SleepUntilResumeDecision::Continue;
	bool ShouldEmitBreakNotification = false;
	SleepUntilResumePreLoopBundleOutcome PreLoopBundle = {};
	SleepUntilResumeLoopOutcome Loop = {};
	SleepUntilResumePostLoopOutcome PostLoop = {};
};

struct SleepUntilResumeCoordinatorEntryContext {
	SleepUntilResumeGuardContext Guard = {};
	BreakSource Source = BreakSource::Unspecified;
	bool HasBreakRequest = false;
	bool SingleBreakpointPerInstruction = false;
	bool DrawPartialFrame = false;
};

struct SleepUntilResumeCoordinatorEntryOutcome {
	SleepUntilResumePhaseContext PhaseContext = {};
	SleepUntilResumePhaseOutcome PhaseOutcome = {};
};

struct SleepUntilResumePreBreakActionPlanContext {
	bool ShouldRunPreBreakSequence = false;
	bool ShouldIgnoreBreakpoints = false;
	bool ShouldDrawPartialFrame = false;
};

struct SleepUntilResumePreBreakActionPlanOutcome {
	bool ShouldCallOnBeforeBreak = false;
	bool ShouldCallOnBeforePause = false;
	bool ShouldIgnoreBreakpoints = false;
	bool ShouldDrawPartialFrame = false;
};

struct SleepUntilResumePreBreakExecutionContext {
	SleepUntilResumePreBreakActionPlanOutcome ActionPlan = {};
};

struct SleepUntilResumePreBreakExecutionOutcome {
	bool ShouldCallOnBeforeBreak = false;
	bool ShouldCallOnBeforePause = false;
	bool ShouldSetIgnoreBreakpoints = false;
	bool ShouldCallDrawPartialFrame = false;
	bool ShouldRunRuntimeBundle = false;
};

[[nodiscard]] inline SleepUntilResumeCoordinatorEntryContext BuildSleepUntilResumeCoordinatorEntryContext(const SleepUntilResumeGuardContext& guard, BreakSource source, bool hasBreakRequest, bool singleBreakpointPerInstruction, bool drawPartialFrame) {
	SleepUntilResumeCoordinatorEntryContext context = {};
	context.Guard = guard;
	context.Source = source;
	context.HasBreakRequest = hasBreakRequest;
	context.SingleBreakpointPerInstruction = singleBreakpointPerInstruction;
	context.DrawPartialFrame = drawPartialFrame;
	return context;
}

[[nodiscard]] inline SleepUntilResumePhaseContext BuildSleepUntilResumePhaseContext(const SleepUntilResumeGuardContext& guardContext, BreakSource source, bool hasBreakRequest, bool singleBreakpointPerInstruction, bool drawPartialFrame) {
	SleepUntilResumePhaseContext context = {};
	context.Guard = guardContext;
	context.Source = source;
	context.HasBreakRequest = hasBreakRequest;
	context.SingleBreakpointPerInstruction = singleBreakpointPerInstruction;
	context.DrawPartialFrame = drawPartialFrame;
	return context;
}

[[nodiscard]] inline SleepUntilResumePreLoopBundleContext BuildSleepUntilResumePreLoopBundleContext(const SleepUntilResumePhaseContext& context, bool shouldEmitBreakNotification) {
	SleepUntilResumePreLoopBundleContext preLoopBundleContext = {};
	preLoopBundleContext.ShouldEmitBreakNotification = shouldEmitBreakNotification;
	preLoopBundleContext.SingleBreakpointPerInstruction = context.SingleBreakpointPerInstruction;
	preLoopBundleContext.DrawPartialFrame = context.DrawPartialFrame;
	return preLoopBundleContext;
}

[[nodiscard]] inline SleepUntilResumeLoopContext BuildSleepUntilResumeLoopContext(const SleepUntilResumePhaseContext& context) {
	SleepUntilResumeLoopContext loopContext = {};
	loopContext.WaitForBreakResume = context.WaitForBreakResume;
	loopContext.HasSuspendRequest = context.HasSuspendRequest;
	loopContext.HasBreakRequest = context.HasBreakRequest;
	return loopContext;
}

[[nodiscard]] inline SleepUntilResumePostLoopContext BuildSleepUntilResumePostLoopContext(const SleepUntilResumePhaseContext& context) {
	SleepUntilResumePostLoopContext postLoopContext = {};
	postLoopContext.NotificationSent = context.NotificationSent;
	return postLoopContext;
}

[[nodiscard]] inline SleepUntilResumePhaseOutcome ResolveSleepUntilResumePhaseOutcome(const SleepUntilResumePhaseContext& context) {
	SleepUntilResumePhaseOutcome outcome = {};
	outcome.Decision = EvaluateSleepUntilResumeDecision(context.Guard);

	if (outcome.Decision == SleepUntilResumeDecision::Continue) {
		outcome.ShouldEmitBreakNotification = ShouldEmitSleepUntilResumeBreakNotification(context.Source, context.HasBreakRequest);
		SleepUntilResumePreLoopBundleContext preLoopBundleContext = BuildSleepUntilResumePreLoopBundleContext(context, outcome.ShouldEmitBreakNotification);
		outcome.PreLoopBundle = ResolveSleepUntilResumePreLoopBundleOutcome(preLoopBundleContext);
	}

	SleepUntilResumeLoopContext loopContext = BuildSleepUntilResumeLoopContext(context);
	outcome.Loop = ResolveSleepUntilResumeLoopOutcome(loopContext);

	SleepUntilResumePostLoopContext postLoopContext = BuildSleepUntilResumePostLoopContext(context);
	outcome.PostLoop = ResolveSleepUntilResumePostLoopOutcome(postLoopContext);

	return outcome;
}

[[nodiscard]] inline SleepUntilResumeCoordinatorEntryOutcome ResolveSleepUntilResumeCoordinatorEntryOutcome(const SleepUntilResumeCoordinatorEntryContext& context) {
	SleepUntilResumeCoordinatorEntryOutcome outcome = {};
	outcome.PhaseContext = BuildSleepUntilResumePhaseContext(context.Guard, context.Source, context.HasBreakRequest, context.SingleBreakpointPerInstruction, context.DrawPartialFrame);
	outcome.PhaseOutcome = ResolveSleepUntilResumePhaseOutcome(outcome.PhaseContext);
	return outcome;
}

[[nodiscard]] inline SleepUntilResumePreBreakActionPlanOutcome ResolveSleepUntilResumePreBreakActionPlanOutcome(const SleepUntilResumePreBreakActionPlanContext& context) {
	SleepUntilResumePreBreakActionPlanOutcome outcome = {};
	outcome.ShouldCallOnBeforeBreak = context.ShouldRunPreBreakSequence;
	outcome.ShouldCallOnBeforePause = context.ShouldRunPreBreakSequence;
	outcome.ShouldIgnoreBreakpoints = context.ShouldRunPreBreakSequence && context.ShouldIgnoreBreakpoints;
	outcome.ShouldDrawPartialFrame = context.ShouldRunPreBreakSequence && context.ShouldDrawPartialFrame;
	return outcome;
}

[[nodiscard]] inline SleepUntilResumePreBreakExecutionOutcome ResolveSleepUntilResumePreBreakExecutionOutcome(const SleepUntilResumePreBreakExecutionContext& context) {
	SleepUntilResumePreBreakExecutionOutcome outcome = {};
	outcome.ShouldCallOnBeforeBreak = context.ActionPlan.ShouldCallOnBeforeBreak;
	outcome.ShouldCallOnBeforePause = context.ActionPlan.ShouldCallOnBeforePause;
	outcome.ShouldSetIgnoreBreakpoints = context.ActionPlan.ShouldCallOnBeforeBreak && context.ActionPlan.ShouldIgnoreBreakpoints;
	outcome.ShouldCallDrawPartialFrame = context.ActionPlan.ShouldCallOnBeforeBreak && context.ActionPlan.ShouldDrawPartialFrame;
	outcome.ShouldRunRuntimeBundle = context.ActionPlan.ShouldCallOnBeforeBreak;
	return outcome;
}

[[nodiscard]] inline SleepUntilResumeRuntimeBundleContext BuildSleepUntilResumeRuntimeBundleContext(const SleepUntilResumePhaseOutcome& phaseOutcome, CpuType sourceCpu, BreakSource source, int32_t breakpointId, const MemoryOperationInfo* operation, bool notificationSent) {
	SleepUntilResumeRuntimeBundleContext context = {};
	context.ShouldRunPreBreakSequence = phaseOutcome.PreLoopBundle.PreLoop.ShouldRunPreBreakSequence;
	context.SourceCpu = sourceCpu;
	context.Source = source;
	context.BreakpointId = breakpointId;
	context.Operation = operation;
	context.ShouldArmWaitForBreakResume = phaseOutcome.PreLoopBundle.PreLoop.ShouldArmWaitForBreakResume;
	context.ShouldEnableScreensaver = phaseOutcome.PreLoopBundle.PreLoop.ShouldEnableScreensaver;
	context.NotificationSent = notificationSent;
	return context;
}

[[nodiscard]] inline SleepUntilResumeLoopPostBundleContext BuildSleepUntilResumeLoopPostBundleContext(bool waitForBreakResume, bool hasSuspendRequest, bool hasBreakRequest, bool notificationSent) {
	SleepUntilResumeLoopPostBundleContext context = {};
	context.WaitForBreakResume = waitForBreakResume;
	context.HasSuspendRequest = hasSuspendRequest;
	context.HasBreakRequest = hasBreakRequest;
	context.NotificationSent = notificationSent;
	return context;
}

[[nodiscard]] inline SleepUntilResumeLoopPostBundleContext BuildSleepUntilResumeLoopPostBundleRuntimeContext(bool waitForBreakResume, int32_t suspendRequestCount, int32_t breakRequestCount) {
	return BuildSleepUntilResumeLoopPostBundleContext(waitForBreakResume, suspendRequestCount > 0, breakRequestCount > 0, false);
}

[[nodiscard]] inline SleepUntilResumeLoopPostBundleContext BuildSleepUntilResumePostLoopBundleRuntimeContext(bool notificationSent) {
	return BuildSleepUntilResumeLoopPostBundleContext(false, false, false, notificationSent);
}

[[nodiscard]] inline SleepUntilResumePreBreakActionPlanContext BuildSleepUntilResumePreBreakActionPlanContext(const SleepUntilResumePhaseOutcome& phaseOutcome) {
	SleepUntilResumePreBreakActionPlanContext context = {};
	context.ShouldRunPreBreakSequence = phaseOutcome.PreLoopBundle.PreLoop.ShouldRunPreBreakSequence;
	context.ShouldIgnoreBreakpoints = phaseOutcome.PreLoopBundle.PreBreak.ShouldIgnoreBreakpoints;
	context.ShouldDrawPartialFrame = phaseOutcome.PreLoopBundle.PreBreak.ShouldDrawPartialFrame;
	return context;
}

[[nodiscard]] inline bool ShouldDispatchScriptEvent(bool debuggerOwnsInstance) {
	return debuggerOwnsInstance;
}

[[nodiscard]] inline std::optional<CpuType> ResolveInputDebuggerCpuType(CpuType routedCpuType, CpuType mainCpuType, bool hasRoutedInputDebugger, bool hasMainInputDebugger) {
	if (hasRoutedInputDebugger) {
		return routedCpuType;
	}

	if (ShouldFallbackToMainInputDebugger(hasRoutedInputDebugger, hasMainInputDebugger)) {
		return mainCpuType;
	}

	return std::nullopt;
}

[[nodiscard]] inline ProcessEventDispatchOutcome ResolveProcessEventDispatchOutcome(EventType type, CpuType routedCpuType, CpuType mainCpuType, const ProcessEventDispatchContext& context) {
	ProcessEventDispatchOutcome outcome = {};
	outcome.ShouldDispatchScriptEvent = ShouldDispatchScriptEvent(context.DebuggerOwnsInstance);

	switch (type) {
		case EventType::InputPolled:
			outcome.InputDebuggerCpuType = ResolveInputDebuggerCpuType(routedCpuType, mainCpuType, context.HasRoutedInputDebugger, context.HasMainInputDebugger);
			break;
		case EventType::StartFrame:
			outcome.ShouldSendEventViewerRefresh = !context.DebuggerBlocked;
			outcome.ShouldClearFrameEvents = context.HasRoutedEventManager;
			break;
		default:
			break;
	}

	return outcome;
}

[[nodiscard]] inline int32_t GetPauseScanlineForCpu(CpuType cpuType) {
	static constexpr std::array<std::pair<CpuType, int32_t>, 10> kPauseScanlineByCpu = {{
		{CpuType::Snes, 240},
		{CpuType::Gameboy, 144},
		{CpuType::Nes, 241},
		{CpuType::Pce, 243},
		{CpuType::Sms, 240},
		{CpuType::Gba, 160},
		{CpuType::Ws, 145},
		{CpuType::Lynx, 102},
		{CpuType::Atari2600, 262},
		{CpuType::ChannelF, 64}
	}};

	for (const auto& entry : kPauseScanlineByCpu) {
		if (entry.first == cpuType) {
			return entry.second;
		}
	}

	return 0;
}

[[nodiscard]] inline bool IsBreakOptionEnabledForSource(BreakSource src, const DebugConfig& cfg) {
	switch (src) {
		case BreakSource::GbDisableLcdOutsideVblank:
			return cfg.GbBreakOnDisableLcdOutsideVblank;
		case BreakSource::GbInvalidVramAccess:
			return cfg.GbBreakOnInvalidVramAccess;
		case BreakSource::GbInvalidOamAccess:
			return cfg.GbBreakOnInvalidOamAccess;
		case BreakSource::NesBreakOnDecayedOamRead:
			return cfg.NesBreakOnDecayedOamRead;
		case BreakSource::NesBreakOnPpuScrollGlitch:
			return cfg.NesBreakOnPpuScrollGlitch;
		case BreakSource::NesBusConflict:
			return cfg.NesBreakOnBusConflict;
		case BreakSource::NesBreakOnCpuCrash:
			return cfg.NesBreakOnCpuCrash;
		case BreakSource::NesBreakOnExtOutputMode:
			return cfg.NesBreakOnExtOutputMode;
		case BreakSource::NesInvalidVramAccess:
			return cfg.NesBreakOnInvalidVramAccess;
		case BreakSource::NesInvalidOamWrite:
			return cfg.NesBreakOnInvalidOamWrite;
		case BreakSource::NesDmaInputRead:
			return cfg.NesBreakOnDmaInputRead;
		case BreakSource::PceBreakOnInvalidVramAddress:
			return cfg.PceBreakOnInvalidVramAddress;
		case BreakSource::GbaInvalidOpCode:
			return cfg.GbaBreakOnInvalidOpCode;
		case BreakSource::GbaUnalignedMemoryAccess:
			return cfg.GbaBreakOnUnalignedMemAccess;
		case BreakSource::SnesInvalidPpuAccess:
			return cfg.SnesBreakOnInvalidPpuAccess;
		case BreakSource::SnesReadDuringAutoJoy:
			return cfg.SnesBreakOnReadDuringAutoJoy;
		default:
			break;
	}

	return true;
}
