#include "pch.h"
#include "Debugger/DebuggerDispatchUtils.h"

namespace {
	SleepUntilResumePhaseContext CreateSleepUntilResumeContinuePhaseContext(BreakSource source, bool hasBreakRequest) {
		SleepUntilResumePhaseContext context = {};
		context.Guard.HasSuspendRequest = false;
		context.Guard.ExecutionAlreadyStopped = false;
		context.Guard.HasBreakRequest = hasBreakRequest;
		context.Guard.SourceCpuIsMainCpu = true;
		context.Guard.AllowChangeProgramCounter = true;
		context.Guard.BreakpointForbidden = false;
		context.Source = source;
		context.HasBreakRequest = hasBreakRequest;
		context.SingleBreakpointPerInstruction = true;
		context.DrawPartialFrame = true;
		return context;
	}

	SleepUntilResumeRuntimeBundleContext CreateRuntimeBundleContext(const SleepUntilResumePhaseOutcome& phaseOutcome, CpuType sourceCpu, BreakSource source, int32_t breakpointId, const MemoryOperationInfo* operation, bool notificationSent) {
		return BuildSleepUntilResumeRuntimeBundleContext(phaseOutcome, sourceCpu, source, breakpointId, operation, notificationSent);
	}
}

TEST(DebuggerDispatchUtilsTests, PauseScanlineMappingMatchesExpectedSystems) {
	EXPECT_EQ(GetPauseScanlineForCpu(CpuType::Snes), 240);
	EXPECT_EQ(GetPauseScanlineForCpu(CpuType::Gameboy), 144);
	EXPECT_EQ(GetPauseScanlineForCpu(CpuType::Nes), 241);
	EXPECT_EQ(GetPauseScanlineForCpu(CpuType::Pce), 243);
	EXPECT_EQ(GetPauseScanlineForCpu(CpuType::Sms), 240);
	EXPECT_EQ(GetPauseScanlineForCpu(CpuType::Gba), 160);
	EXPECT_EQ(GetPauseScanlineForCpu(CpuType::Ws), 145);
	EXPECT_EQ(GetPauseScanlineForCpu(CpuType::Lynx), 102);
	EXPECT_EQ(GetPauseScanlineForCpu(CpuType::Atari2600), 262);
	EXPECT_EQ(GetPauseScanlineForCpu(CpuType::ChannelF), 64);
}

TEST(DebuggerDispatchUtilsTests, PauseScanlineMappingReturnsZeroForUnsupportedCpuTypes) {
	EXPECT_EQ(GetPauseScanlineForCpu(CpuType::Spc), 0);
	EXPECT_EQ(GetPauseScanlineForCpu(CpuType::NecDsp), 0);
	EXPECT_EQ(GetPauseScanlineForCpu(CpuType::Sa1), 0);
	EXPECT_EQ(GetPauseScanlineForCpu(CpuType::Gsu), 0);
	EXPECT_EQ(GetPauseScanlineForCpu(CpuType::Cx4), 0);
	EXPECT_EQ(GetPauseScanlineForCpu(CpuType::St018), 0);
	EXPECT_EQ(GetPauseScanlineForCpu(CpuType::Genesis), 0);
}

TEST(DebuggerDispatchUtilsTests, CpuStateLayoutMappingMatchesExpectedCpuFamilies) {
	EXPECT_EQ(GetCpuStateLayout(CpuType::Snes), CpuStateLayout::SnesCpu);
	EXPECT_EQ(GetCpuStateLayout(CpuType::Sa1), CpuStateLayout::SnesCpu);
	EXPECT_EQ(GetCpuStateLayout(CpuType::Spc), CpuStateLayout::Spc);
	EXPECT_EQ(GetCpuStateLayout(CpuType::NecDsp), CpuStateLayout::NecDsp);
	EXPECT_EQ(GetCpuStateLayout(CpuType::Gsu), CpuStateLayout::Gsu);
	EXPECT_EQ(GetCpuStateLayout(CpuType::Cx4), CpuStateLayout::Cx4);
	EXPECT_EQ(GetCpuStateLayout(CpuType::St018), CpuStateLayout::ArmV3);
	EXPECT_EQ(GetCpuStateLayout(CpuType::Gameboy), CpuStateLayout::GbCpu);
	EXPECT_EQ(GetCpuStateLayout(CpuType::Nes), CpuStateLayout::NesCpu);
	EXPECT_EQ(GetCpuStateLayout(CpuType::Pce), CpuStateLayout::PceCpu);
	EXPECT_EQ(GetCpuStateLayout(CpuType::Sms), CpuStateLayout::SmsCpu);
	EXPECT_EQ(GetCpuStateLayout(CpuType::Gba), CpuStateLayout::GbaCpu);
	EXPECT_EQ(GetCpuStateLayout(CpuType::Ws), CpuStateLayout::WsCpu);
	EXPECT_EQ(GetCpuStateLayout(CpuType::Lynx), CpuStateLayout::LynxCpu);
	EXPECT_EQ(GetCpuStateLayout(CpuType::Atari2600), CpuStateLayout::Atari2600Cpu);
	EXPECT_EQ(GetCpuStateLayout(CpuType::ChannelF), CpuStateLayout::ChannelFCpu);
	EXPECT_EQ(GetCpuStateLayout(CpuType::Genesis), CpuStateLayout::GenesisM68k);
}

TEST(DebuggerDispatchUtilsTests, DebuggerFlagMappingMatchesExpectedCpuTargets) {
	EXPECT_EQ(GetDebuggerFlagForCpu(CpuType::Snes), DebuggerFlags::SnesDebuggerEnabled);
	EXPECT_EQ(GetDebuggerFlagForCpu(CpuType::Spc), DebuggerFlags::SpcDebuggerEnabled);
	EXPECT_EQ(GetDebuggerFlagForCpu(CpuType::NecDsp), DebuggerFlags::NecDspDebuggerEnabled);
	EXPECT_EQ(GetDebuggerFlagForCpu(CpuType::Sa1), DebuggerFlags::Sa1DebuggerEnabled);
	EXPECT_EQ(GetDebuggerFlagForCpu(CpuType::Gsu), DebuggerFlags::GsuDebuggerEnabled);
	EXPECT_EQ(GetDebuggerFlagForCpu(CpuType::Cx4), DebuggerFlags::Cx4DebuggerEnabled);
	EXPECT_EQ(GetDebuggerFlagForCpu(CpuType::St018), DebuggerFlags::St018DebuggerEnabled);
	EXPECT_EQ(GetDebuggerFlagForCpu(CpuType::Gameboy), DebuggerFlags::GbDebuggerEnabled);
	EXPECT_EQ(GetDebuggerFlagForCpu(CpuType::Nes), DebuggerFlags::NesDebuggerEnabled);
	EXPECT_EQ(GetDebuggerFlagForCpu(CpuType::Pce), DebuggerFlags::PceDebuggerEnabled);
	EXPECT_EQ(GetDebuggerFlagForCpu(CpuType::Sms), DebuggerFlags::SmsDebuggerEnabled);
	EXPECT_EQ(GetDebuggerFlagForCpu(CpuType::Gba), DebuggerFlags::GbaDebuggerEnabled);
	EXPECT_EQ(GetDebuggerFlagForCpu(CpuType::Ws), DebuggerFlags::WsDebuggerEnabled);
	EXPECT_EQ(GetDebuggerFlagForCpu(CpuType::Lynx), DebuggerFlags::LynxDebuggerEnabled);
	EXPECT_EQ(GetDebuggerFlagForCpu(CpuType::Atari2600), DebuggerFlags::Atari2600DebuggerEnabled);
	EXPECT_EQ(GetDebuggerFlagForCpu(CpuType::ChannelF), DebuggerFlags::ChannelFDebuggerEnabled);
	EXPECT_EQ(GetDebuggerFlagForCpu(CpuType::Genesis), std::nullopt);
}

TEST(DebuggerDispatchUtilsTests, BreakSourceMappingUsesExpectedConfigFlags) {
	DebugConfig cfg = {};
	cfg.GbBreakOnDisableLcdOutsideVblank = true;
	cfg.NesBreakOnBusConflict = true;
	cfg.GbaBreakOnUnalignedMemAccess = true;
	cfg.SnesBreakOnReadDuringAutoJoy = true;

	EXPECT_TRUE(IsBreakOptionEnabledForSource(BreakSource::GbDisableLcdOutsideVblank, cfg));
	EXPECT_TRUE(IsBreakOptionEnabledForSource(BreakSource::NesBusConflict, cfg));
	EXPECT_TRUE(IsBreakOptionEnabledForSource(BreakSource::GbaUnalignedMemoryAccess, cfg));
	EXPECT_TRUE(IsBreakOptionEnabledForSource(BreakSource::SnesReadDuringAutoJoy, cfg));

	EXPECT_FALSE(IsBreakOptionEnabledForSource(BreakSource::NesInvalidVramAccess, cfg));
}

TEST(DebuggerDispatchUtilsTests, BreakSourceMappingDefaultsToTrueForUnmappedSources) {
	DebugConfig cfg = {};

	EXPECT_TRUE(IsBreakOptionEnabledForSource(BreakSource::Breakpoint, cfg));
	EXPECT_TRUE(IsBreakOptionEnabledForSource(BreakSource::CpuStep, cfg));
	EXPECT_TRUE(IsBreakOptionEnabledForSource(BreakSource::BreakOnBrk, cfg));
}

TEST(DebuggerDispatchUtilsTests, PpuBackendMappingRoutesGroupedCpuTypesToExpectedBackends) {
	EXPECT_EQ(GetPpuStateBackendForCpu(CpuType::Snes), PpuStateBackend::Snes);
	EXPECT_EQ(GetPpuStateBackendForCpu(CpuType::Spc), PpuStateBackend::Snes);
	EXPECT_EQ(GetPpuStateBackendForCpu(CpuType::NecDsp), PpuStateBackend::Snes);
	EXPECT_EQ(GetPpuStateBackendForCpu(CpuType::Sa1), PpuStateBackend::Snes);
	EXPECT_EQ(GetPpuStateBackendForCpu(CpuType::Gsu), PpuStateBackend::Snes);
	EXPECT_EQ(GetPpuStateBackendForCpu(CpuType::Cx4), PpuStateBackend::Snes);
	EXPECT_EQ(GetPpuStateBackendForCpu(CpuType::St018), PpuStateBackend::Snes);
	EXPECT_EQ(GetPpuStateBackendForCpu(CpuType::Gameboy), PpuStateBackend::Gameboy);
	EXPECT_EQ(GetPpuStateBackendForCpu(CpuType::Nes), PpuStateBackend::Nes);
	EXPECT_EQ(GetPpuStateBackendForCpu(CpuType::Pce), PpuStateBackend::Pce);
	EXPECT_EQ(GetPpuStateBackendForCpu(CpuType::Sms), PpuStateBackend::Sms);
	EXPECT_EQ(GetPpuStateBackendForCpu(CpuType::Gba), PpuStateBackend::Gba);
	EXPECT_EQ(GetPpuStateBackendForCpu(CpuType::Ws), PpuStateBackend::Ws);
	EXPECT_EQ(GetPpuStateBackendForCpu(CpuType::Lynx), PpuStateBackend::Lynx);
	EXPECT_EQ(GetPpuStateBackendForCpu(CpuType::Atari2600), PpuStateBackend::Atari2600);
	EXPECT_EQ(GetPpuStateBackendForCpu(CpuType::ChannelF), PpuStateBackend::ChannelF);
}

TEST(DebuggerDispatchUtilsTests, PpuBackendMappingReturnsNoneForUnsupportedCpuTypes) {
	EXPECT_EQ(GetPpuStateBackendForCpu(CpuType::Genesis), PpuStateBackend::None);
}

TEST(DebuggerDispatchUtilsTests, CpuDispatchMappingsRemainInParityAcrossLayoutFlagsAndPpuBackends) {
	struct ExpectedCpuDispatch {
		CpuType Type;
		CpuStateLayout Layout;
		std::optional<DebuggerFlags> Flag;
		PpuStateBackend Backend;
	};

	static constexpr std::array<ExpectedCpuDispatch, 17> kExpectedMappings = {{
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

	for(const ExpectedCpuDispatch& expected : kExpectedMappings) {
		EXPECT_EQ(GetCpuStateLayout(expected.Type), expected.Layout);
		EXPECT_EQ(GetDebuggerFlagForCpu(expected.Type), expected.Flag);
		EXPECT_EQ(GetPpuStateBackendForCpu(expected.Type), expected.Backend);
	}
}

TEST(DebuggerDispatchUtilsTests, EventCpuRoutingUsesRequestedCpuWhenAvailable) {
	EXPECT_EQ(ResolveEventCpuType(CpuType::Nes, CpuType::Snes, true), CpuType::Nes);
	EXPECT_EQ(ResolveEventCpuType(CpuType::Gba, CpuType::Snes, true), CpuType::Gba);
}

TEST(DebuggerDispatchUtilsTests, EventCpuRoutingFallsBackToMainCpuWhenRequestedCpuUnavailable) {
	EXPECT_EQ(ResolveEventCpuType(CpuType::Nes, CpuType::Snes, false), CpuType::Snes);
	EXPECT_EQ(ResolveEventCpuType(CpuType::ChannelF, CpuType::Gameboy, false), CpuType::Gameboy);
}

TEST(DebuggerDispatchUtilsTests, InputDebuggerFallbackDecisionRequiresMissingRoutedAndPresentMain) {
	EXPECT_TRUE(ShouldFallbackToMainInputDebugger(false, true));
	EXPECT_FALSE(ShouldFallbackToMainInputDebugger(true, true));
	EXPECT_FALSE(ShouldFallbackToMainInputDebugger(false, false));
}

TEST(DebuggerDispatchUtilsTests, ScriptDispatchRequiresDebuggerOwnership) {
	EXPECT_TRUE(ShouldDispatchScriptEvent(true));
	EXPECT_FALSE(ShouldDispatchScriptEvent(false));
}

TEST(DebuggerDispatchUtilsTests, InputDebuggerCpuResolutionPrefersRoutedThenMainThenNone) {
	EXPECT_EQ(ResolveInputDebuggerCpuType(CpuType::Nes, CpuType::Snes, true, true), CpuType::Nes);
	EXPECT_EQ(ResolveInputDebuggerCpuType(CpuType::Nes, CpuType::Snes, false, true), CpuType::Snes);
	EXPECT_FALSE(ResolveInputDebuggerCpuType(CpuType::Nes, CpuType::Snes, false, false).has_value());
}

TEST(DebuggerDispatchUtilsTests, ProcessEventDispatchOutcomeComposesInputFallbackAndScriptOwnership) {
	ProcessEventDispatchContext context = {};
	context.DebuggerOwnsInstance = false;
	context.HasRoutedInputDebugger = false;
	context.HasMainInputDebugger = true;

	ProcessEventDispatchOutcome outcome = ResolveProcessEventDispatchOutcome(EventType::InputPolled, CpuType::Nes, CpuType::Snes, context);
	EXPECT_FALSE(outcome.ShouldDispatchScriptEvent);
	ASSERT_TRUE(outcome.InputDebuggerCpuType.has_value());
	EXPECT_EQ(outcome.InputDebuggerCpuType.value(), CpuType::Snes);
	EXPECT_FALSE(outcome.ShouldSendEventViewerRefresh);
	EXPECT_FALSE(outcome.ShouldClearFrameEvents);
}

TEST(DebuggerDispatchUtilsTests, ProcessEventDispatchOutcomeComposesStartFrameRefreshAndEventManagerFlags) {
	ProcessEventDispatchContext activeContext = {};
	activeContext.DebuggerOwnsInstance = true;
	activeContext.DebuggerBlocked = false;
	activeContext.HasRoutedEventManager = true;

	ProcessEventDispatchOutcome activeOutcome = ResolveProcessEventDispatchOutcome(EventType::StartFrame, CpuType::Nes, CpuType::Snes, activeContext);
	EXPECT_TRUE(activeOutcome.ShouldDispatchScriptEvent);
	EXPECT_TRUE(activeOutcome.ShouldSendEventViewerRefresh);
	EXPECT_TRUE(activeOutcome.ShouldClearFrameEvents);

	ProcessEventDispatchContext blockedContext = {};
	blockedContext.DebuggerOwnsInstance = true;
	blockedContext.DebuggerBlocked = true;
	blockedContext.HasRoutedEventManager = false;

	ProcessEventDispatchOutcome blockedOutcome = ResolveProcessEventDispatchOutcome(EventType::StartFrame, CpuType::Nes, CpuType::Snes, blockedContext);
	EXPECT_TRUE(blockedOutcome.ShouldDispatchScriptEvent);
	EXPECT_FALSE(blockedOutcome.ShouldSendEventViewerRefresh);
	EXPECT_FALSE(blockedOutcome.ShouldClearFrameEvents);
}

TEST(DebuggerDispatchUtilsTests, SleepUntilResumeDecisionSkipsForSuspendRequestFirst) {
	SleepUntilResumeGuardContext context = {};
	context.HasSuspendRequest = true;
	context.ExecutionAlreadyStopped = true;
	context.HasBreakRequest = true;
	context.SourceCpuIsMainCpu = false;
	context.AllowChangeProgramCounter = false;
	context.BreakpointForbidden = true;

	EXPECT_EQ(EvaluateSleepUntilResumeDecision(context), SleepUntilResumeDecision::SkipForSuspendRequest);
}

TEST(DebuggerDispatchUtilsTests, SleepUntilResumeDecisionSkipsForExecutionAlreadyStoppedWhenNoSuspend) {
	SleepUntilResumeGuardContext context = {};
	context.ExecutionAlreadyStopped = true;

	EXPECT_EQ(EvaluateSleepUntilResumeDecision(context), SleepUntilResumeDecision::SkipForExecutionAlreadyStopped);
}

TEST(DebuggerDispatchUtilsTests, SleepUntilResumeDecisionSkipsForBreakRequestMainCpuBoundary) {
	SleepUntilResumeGuardContext nonMainCpuContext = {};
	nonMainCpuContext.HasBreakRequest = true;
	nonMainCpuContext.SourceCpuIsMainCpu = false;
	nonMainCpuContext.AllowChangeProgramCounter = true;
	EXPECT_EQ(EvaluateSleepUntilResumeDecision(nonMainCpuContext), SleepUntilResumeDecision::SkipForBreakRequestMainCpuBoundary);

	SleepUntilResumeGuardContext blockedProgramCounterContext = {};
	blockedProgramCounterContext.HasBreakRequest = true;
	blockedProgramCounterContext.SourceCpuIsMainCpu = true;
	blockedProgramCounterContext.AllowChangeProgramCounter = false;
	EXPECT_EQ(EvaluateSleepUntilResumeDecision(blockedProgramCounterContext), SleepUntilResumeDecision::SkipForBreakRequestMainCpuBoundary);
}

TEST(DebuggerDispatchUtilsTests, SleepUntilResumeDecisionSkipsForForbiddenBreakpointWhenOtherGuardsPass) {
	SleepUntilResumeGuardContext context = {};
	context.BreakpointForbidden = true;

	EXPECT_EQ(EvaluateSleepUntilResumeDecision(context), SleepUntilResumeDecision::SkipForForbiddenBreakpoint);
}

TEST(DebuggerDispatchUtilsTests, SleepUntilResumeDecisionContinuesWhenNoGuardTriggers) {
	SleepUntilResumeGuardContext context = {};
	context.HasSuspendRequest = false;
	context.ExecutionAlreadyStopped = false;
	context.HasBreakRequest = false;
	context.SourceCpuIsMainCpu = true;
	context.AllowChangeProgramCounter = true;
	context.BreakpointForbidden = false;

	EXPECT_EQ(EvaluateSleepUntilResumeDecision(context), SleepUntilResumeDecision::Continue);
}

TEST(DebuggerDispatchUtilsTests, SleepUntilResumeGuardContextBuilderMapsAllRuntimeGuardFields) {
	SleepUntilResumeGuardContext context = BuildSleepUntilResumeGuardContext(true, false, true, true, false, true);
	EXPECT_TRUE(context.HasSuspendRequest);
	EXPECT_FALSE(context.ExecutionAlreadyStopped);
	EXPECT_TRUE(context.HasBreakRequest);
	EXPECT_TRUE(context.SourceCpuIsMainCpu);
	EXPECT_FALSE(context.AllowChangeProgramCounter);
	EXPECT_TRUE(context.BreakpointForbidden);
}

TEST(DebuggerDispatchUtilsTests, SleepUntilResumeGuardContextBuilderSupportsContinuationBoundaryCombination) {
	SleepUntilResumeGuardContext context = BuildSleepUntilResumeGuardContext(false, false, false, true, true, false);
	EXPECT_FALSE(context.HasSuspendRequest);
	EXPECT_FALSE(context.ExecutionAlreadyStopped);
	EXPECT_FALSE(context.HasBreakRequest);
	EXPECT_TRUE(context.SourceCpuIsMainCpu);
	EXPECT_TRUE(context.AllowChangeProgramCounter);
	EXPECT_FALSE(context.BreakpointForbidden);
	EXPECT_EQ(EvaluateSleepUntilResumeDecision(context), SleepUntilResumeDecision::Continue);
}

TEST(DebuggerDispatchUtilsTests, SleepUntilResumePhaseContextBuilderMapsGuardSourceBreakAndConfigFields) {
	SleepUntilResumeGuardContext guardContext = {};
	guardContext.HasSuspendRequest = true;
	guardContext.ExecutionAlreadyStopped = false;
	guardContext.HasBreakRequest = true;
	guardContext.SourceCpuIsMainCpu = true;
	guardContext.AllowChangeProgramCounter = false;
	guardContext.BreakpointForbidden = true;

	SleepUntilResumePhaseContext context = BuildSleepUntilResumePhaseContext(guardContext, BreakSource::Breakpoint, true, true, false);
	EXPECT_TRUE(context.Guard.HasSuspendRequest);
	EXPECT_FALSE(context.Guard.ExecutionAlreadyStopped);
	EXPECT_TRUE(context.Guard.HasBreakRequest);
	EXPECT_TRUE(context.Guard.SourceCpuIsMainCpu);
	EXPECT_FALSE(context.Guard.AllowChangeProgramCounter);
	EXPECT_TRUE(context.Guard.BreakpointForbidden);
	EXPECT_EQ(context.Source, BreakSource::Breakpoint);
	EXPECT_TRUE(context.HasBreakRequest);
	EXPECT_TRUE(context.SingleBreakpointPerInstruction);
	EXPECT_FALSE(context.DrawPartialFrame);
}

TEST(DebuggerDispatchUtilsTests, SleepUntilResumePhaseContextBuilderSupportsNonEmittedConfigurationInputs) {
	SleepUntilResumeGuardContext guardContext = {};
	guardContext.HasSuspendRequest = false;
	guardContext.ExecutionAlreadyStopped = false;
	guardContext.HasBreakRequest = true;
	guardContext.SourceCpuIsMainCpu = true;
	guardContext.AllowChangeProgramCounter = true;
	guardContext.BreakpointForbidden = false;

	SleepUntilResumePhaseContext context = BuildSleepUntilResumePhaseContext(guardContext, BreakSource::Unspecified, true, false, true);
	EXPECT_EQ(context.Source, BreakSource::Unspecified);
	EXPECT_TRUE(context.HasBreakRequest);
	EXPECT_FALSE(context.SingleBreakpointPerInstruction);
	EXPECT_TRUE(context.DrawPartialFrame);
	EXPECT_FALSE(context.WaitForBreakResume);
	EXPECT_FALSE(context.HasSuspendRequest);
	EXPECT_FALSE(context.NotificationSent);
}

TEST(DebuggerDispatchUtilsTests, SleepUntilResumeLoopPostContextBuilderMapsAllLoopAndPostFields) {
	SleepUntilResumeLoopPostBundleContext context = BuildSleepUntilResumeLoopPostBundleContext(true, false, true, true);
	EXPECT_TRUE(context.WaitForBreakResume);
	EXPECT_FALSE(context.HasSuspendRequest);
	EXPECT_TRUE(context.HasBreakRequest);
	EXPECT_TRUE(context.NotificationSent);
}

TEST(DebuggerDispatchUtilsTests, SleepUntilResumeLoopPostContextBuilderSupportsNonEmittedIdleCombination) {
	SleepUntilResumeLoopPostBundleContext context = BuildSleepUntilResumeLoopPostBundleContext(false, false, false, false);
	EXPECT_FALSE(context.WaitForBreakResume);
	EXPECT_FALSE(context.HasSuspendRequest);
	EXPECT_FALSE(context.HasBreakRequest);
	EXPECT_FALSE(context.NotificationSent);

	SleepUntilResumeLoopPostBundleOutcome outcome = ResolveSleepUntilResumeLoopPostBundleOutcome(context);
	EXPECT_FALSE(outcome.Loop.ShouldContinueWaiting);
	EXPECT_EQ(outcome.Loop.WaitDelayMs, 10);
	EXPECT_FALSE(outcome.PostLoop.ShouldDisableScreensaver);
	EXPECT_FALSE(outcome.PostLoop.ShouldSendDebuggerResumedNotification);
}

TEST(DebuggerDispatchUtilsTests, SleepUntilResumeLoopPostBundleRuntimeContextBuilderMapsCounterStateToBooleans) {
	SleepUntilResumeLoopPostBundleContext context = BuildSleepUntilResumeLoopPostBundleRuntimeContext(true, 2, 0);
	EXPECT_TRUE(context.WaitForBreakResume);
	EXPECT_TRUE(context.HasSuspendRequest);
	EXPECT_FALSE(context.HasBreakRequest);
	EXPECT_FALSE(context.NotificationSent);
}

TEST(DebuggerDispatchUtilsTests, SleepUntilResumePostLoopBundleRuntimeContextBuilderMapsNotificationState) {
	SleepUntilResumeLoopPostBundleContext context = BuildSleepUntilResumePostLoopBundleRuntimeContext(true);
	EXPECT_FALSE(context.WaitForBreakResume);
	EXPECT_FALSE(context.HasSuspendRequest);
	EXPECT_FALSE(context.HasBreakRequest);
	EXPECT_TRUE(context.NotificationSent);
}

TEST(DebuggerDispatchUtilsTests, SleepUntilResumeCoordinatorEntryOutcomeResolvesContinueWithEmittedPolicyInputs) {
	SleepUntilResumeCoordinatorEntryContext context = {};
	context.Guard = BuildSleepUntilResumeGuardContext(false, false, true, true, true, false);
	context.Source = BreakSource::Breakpoint;
	context.HasBreakRequest = true;
	context.SingleBreakpointPerInstruction = true;
	context.DrawPartialFrame = true;

	SleepUntilResumeCoordinatorEntryOutcome outcome = ResolveSleepUntilResumeCoordinatorEntryOutcome(context);
	EXPECT_EQ(outcome.PhaseOutcome.Decision, SleepUntilResumeDecision::Continue);
	EXPECT_TRUE(outcome.PhaseOutcome.ShouldEmitBreakNotification);
	EXPECT_TRUE(outcome.PhaseOutcome.PreLoopBundle.PreLoop.ShouldRunPreBreakSequence);
	EXPECT_TRUE(outcome.PhaseContext.SingleBreakpointPerInstruction);
	EXPECT_TRUE(outcome.PhaseContext.DrawPartialFrame);
}

TEST(DebuggerDispatchUtilsTests, SleepUntilResumeCoordinatorEntryContextBuilderMapsGuardAndRuntimePolicyFields) {
	SleepUntilResumeGuardContext guardContext = BuildSleepUntilResumeGuardContext(false, true, true, false, true, false);
	SleepUntilResumeCoordinatorEntryContext context = BuildSleepUntilResumeCoordinatorEntryContext(guardContext, BreakSource::Breakpoint, true, true, false);

	EXPECT_TRUE(context.Guard.ExecutionAlreadyStopped);
	EXPECT_EQ(context.Source, BreakSource::Breakpoint);
	EXPECT_TRUE(context.HasBreakRequest);
	EXPECT_TRUE(context.SingleBreakpointPerInstruction);
	EXPECT_FALSE(context.DrawPartialFrame);
}

TEST(DebuggerDispatchUtilsTests, SleepUntilResumeCoordinatorEntryOutcomePreservesSkipDecisionWithoutEmittedPath) {
	SleepUntilResumeCoordinatorEntryContext context = {};
	context.Guard = BuildSleepUntilResumeGuardContext(true, false, true, true, true, false);
	context.Source = BreakSource::Unspecified;
	context.HasBreakRequest = true;
	context.SingleBreakpointPerInstruction = false;
	context.DrawPartialFrame = false;

	SleepUntilResumeCoordinatorEntryOutcome outcome = ResolveSleepUntilResumeCoordinatorEntryOutcome(context);
	EXPECT_EQ(outcome.PhaseOutcome.Decision, SleepUntilResumeDecision::SkipForSuspendRequest);
	EXPECT_FALSE(outcome.PhaseOutcome.ShouldEmitBreakNotification);
	EXPECT_FALSE(outcome.PhaseOutcome.PreLoopBundle.PreLoop.ShouldRunPreBreakSequence);
	EXPECT_FALSE(outcome.PhaseContext.SingleBreakpointPerInstruction);
	EXPECT_FALSE(outcome.PhaseContext.DrawPartialFrame);
}

TEST(DebuggerDispatchUtilsTests, SleepUntilResumePreBreakActionPlanContextBuilderMapsPreLoopAndPreBreakFieldsForEmittedFlow) {
	SleepUntilResumePhaseContext phaseContext = CreateSleepUntilResumeContinuePhaseContext(BreakSource::Breakpoint, true);
	SleepUntilResumePhaseOutcome phaseOutcome = ResolveSleepUntilResumePhaseOutcome(phaseContext);

	SleepUntilResumePreBreakActionPlanContext context = BuildSleepUntilResumePreBreakActionPlanContext(phaseOutcome);
	EXPECT_TRUE(context.ShouldRunPreBreakSequence);
	EXPECT_TRUE(context.ShouldIgnoreBreakpoints);
	EXPECT_TRUE(context.ShouldDrawPartialFrame);
}

TEST(DebuggerDispatchUtilsTests, SleepUntilResumePreBreakActionPlanContextBuilderDisablesPreBreakFieldsForNonEmittedFlow) {
	SleepUntilResumePhaseContext phaseContext = CreateSleepUntilResumeContinuePhaseContext(BreakSource::Unspecified, true);
	SleepUntilResumePhaseOutcome phaseOutcome = ResolveSleepUntilResumePhaseOutcome(phaseContext);

	SleepUntilResumePreBreakActionPlanContext context = BuildSleepUntilResumePreBreakActionPlanContext(phaseOutcome);
	EXPECT_FALSE(context.ShouldRunPreBreakSequence);
	EXPECT_FALSE(context.ShouldIgnoreBreakpoints);
	EXPECT_FALSE(context.ShouldDrawPartialFrame);
}

TEST(DebuggerDispatchUtilsTests, SleepUntilResumePreBreakActionPlanOutcomeDisablesActionsWhenSequenceDoesNotRun) {
	SleepUntilResumePreBreakActionPlanContext context = {};
	context.ShouldRunPreBreakSequence = false;
	context.ShouldIgnoreBreakpoints = true;
	context.ShouldDrawPartialFrame = true;

	SleepUntilResumePreBreakActionPlanOutcome outcome = ResolveSleepUntilResumePreBreakActionPlanOutcome(context);
	EXPECT_FALSE(outcome.ShouldCallOnBeforeBreak);
	EXPECT_FALSE(outcome.ShouldCallOnBeforePause);
	EXPECT_FALSE(outcome.ShouldIgnoreBreakpoints);
	EXPECT_FALSE(outcome.ShouldDrawPartialFrame);
}

TEST(DebuggerDispatchUtilsTests, SleepUntilResumePreBreakActionPlanOutcomePropagatesActionsWhenSequenceRuns) {
	SleepUntilResumePreBreakActionPlanContext context = {};
	context.ShouldRunPreBreakSequence = true;
	context.ShouldIgnoreBreakpoints = true;
	context.ShouldDrawPartialFrame = false;

	SleepUntilResumePreBreakActionPlanOutcome outcome = ResolveSleepUntilResumePreBreakActionPlanOutcome(context);
	EXPECT_TRUE(outcome.ShouldCallOnBeforeBreak);
	EXPECT_TRUE(outcome.ShouldCallOnBeforePause);
	EXPECT_TRUE(outcome.ShouldIgnoreBreakpoints);
	EXPECT_FALSE(outcome.ShouldDrawPartialFrame);
}

TEST(DebuggerDispatchUtilsTests, SleepUntilResumePreBreakExecutionOutcomeEnablesRuntimeFlowForBreakSequence) {
	SleepUntilResumePreBreakExecutionContext context = {};
	context.ActionPlan.ShouldCallOnBeforeBreak = true;
	context.ActionPlan.ShouldCallOnBeforePause = true;
	context.ActionPlan.ShouldIgnoreBreakpoints = true;
	context.ActionPlan.ShouldDrawPartialFrame = true;

	SleepUntilResumePreBreakExecutionOutcome outcome = ResolveSleepUntilResumePreBreakExecutionOutcome(context);
	EXPECT_TRUE(outcome.ShouldCallOnBeforeBreak);
	EXPECT_TRUE(outcome.ShouldCallOnBeforePause);
	EXPECT_TRUE(outcome.ShouldSetIgnoreBreakpoints);
	EXPECT_TRUE(outcome.ShouldCallDrawPartialFrame);
	EXPECT_TRUE(outcome.ShouldRunRuntimeBundle);
}

TEST(DebuggerDispatchUtilsTests, SleepUntilResumePreBreakExecutionOutcomeDisablesRuntimeFlowWithoutBreakCallback) {
	SleepUntilResumePreBreakExecutionContext context = {};
	context.ActionPlan.ShouldCallOnBeforeBreak = false;
	context.ActionPlan.ShouldCallOnBeforePause = false;
	context.ActionPlan.ShouldIgnoreBreakpoints = true;
	context.ActionPlan.ShouldDrawPartialFrame = true;

	SleepUntilResumePreBreakExecutionOutcome outcome = ResolveSleepUntilResumePreBreakExecutionOutcome(context);
	EXPECT_FALSE(outcome.ShouldCallOnBeforeBreak);
	EXPECT_FALSE(outcome.ShouldCallOnBeforePause);
	EXPECT_FALSE(outcome.ShouldSetIgnoreBreakpoints);
	EXPECT_FALSE(outcome.ShouldCallDrawPartialFrame);
	EXPECT_FALSE(outcome.ShouldRunRuntimeBundle);
}

TEST(DebuggerDispatchUtilsTests, SleepUntilResumeBreakNotificationPolicyUsesSourceAndBreakRequestState) {
	EXPECT_TRUE(ShouldEmitSleepUntilResumeBreakNotification(BreakSource::Breakpoint, false));
	EXPECT_TRUE(ShouldEmitSleepUntilResumeBreakNotification(BreakSource::Breakpoint, true));
	EXPECT_TRUE(ShouldEmitSleepUntilResumeBreakNotification(BreakSource::CpuStep, true));

	EXPECT_TRUE(ShouldEmitSleepUntilResumeBreakNotification(BreakSource::Unspecified, false));
	EXPECT_FALSE(ShouldEmitSleepUntilResumeBreakNotification(BreakSource::Unspecified, true));
}

TEST(DebuggerDispatchUtilsTests, SleepUntilResumeWaitDelayPolicyUsesExpectedMilliseconds) {
	EXPECT_EQ(GetSleepUntilResumeWaitDelayMs(true), 1);
	EXPECT_EQ(GetSleepUntilResumeWaitDelayMs(false), 10);
}

TEST(DebuggerDispatchUtilsTests, SleepUntilResumePreBreakOutcomeIsDisabledWhenNotificationNotEmitted) {
	SleepUntilResumePreBreakContext context = {};
	context.ShouldEmitBreakNotification = false;
	context.SingleBreakpointPerInstruction = true;
	context.DrawPartialFrame = true;

	SleepUntilResumePreBreakOutcome outcome = ResolveSleepUntilResumePreBreakOutcome(context);
	EXPECT_FALSE(outcome.ShouldIgnoreBreakpoints);
	EXPECT_FALSE(outcome.ShouldDrawPartialFrame);
}

TEST(DebuggerDispatchUtilsTests, SleepUntilResumePreBreakOutcomeTracksConfiguredSideEffectsWhenNotificationEmitted) {
	SleepUntilResumePreBreakContext fullContext = {};
	fullContext.ShouldEmitBreakNotification = true;
	fullContext.SingleBreakpointPerInstruction = true;
	fullContext.DrawPartialFrame = true;

	SleepUntilResumePreBreakOutcome fullOutcome = ResolveSleepUntilResumePreBreakOutcome(fullContext);
	EXPECT_TRUE(fullOutcome.ShouldIgnoreBreakpoints);
	EXPECT_TRUE(fullOutcome.ShouldDrawPartialFrame);

	SleepUntilResumePreBreakContext partialContext = {};
	partialContext.ShouldEmitBreakNotification = true;
	partialContext.SingleBreakpointPerInstruction = false;
	partialContext.DrawPartialFrame = true;

	SleepUntilResumePreBreakOutcome partialOutcome = ResolveSleepUntilResumePreBreakOutcome(partialContext);
	EXPECT_FALSE(partialOutcome.ShouldIgnoreBreakpoints);
	EXPECT_TRUE(partialOutcome.ShouldDrawPartialFrame);
}

TEST(DebuggerDispatchUtilsTests, SleepUntilResumePreLoopOutcomeDisablesSequenceWhenNotificationNotEmitted) {
	SleepUntilResumePreLoopContext context = {};
	context.ShouldEmitBreakNotification = false;

	SleepUntilResumePreLoopOutcome outcome = ResolveSleepUntilResumePreLoopOutcome(context);
	EXPECT_FALSE(outcome.ShouldRunPreBreakSequence);
	EXPECT_FALSE(outcome.ShouldArmWaitForBreakResume);
	EXPECT_FALSE(outcome.ShouldEnableScreensaver);
}

TEST(DebuggerDispatchUtilsTests, SleepUntilResumePreLoopOutcomeEnablesSequenceWhenNotificationEmitted) {
	SleepUntilResumePreLoopContext context = {};
	context.ShouldEmitBreakNotification = true;

	SleepUntilResumePreLoopOutcome outcome = ResolveSleepUntilResumePreLoopOutcome(context);
	EXPECT_TRUE(outcome.ShouldRunPreBreakSequence);
	EXPECT_TRUE(outcome.ShouldArmWaitForBreakResume);
	EXPECT_TRUE(outcome.ShouldEnableScreensaver);
}

TEST(DebuggerDispatchUtilsTests, SleepUntilResumeLoopOutcomeContinuesForWaitOrBreakRequestStates) {
	SleepUntilResumeLoopContext waitContext = {};
	waitContext.WaitForBreakResume = true;
	waitContext.HasSuspendRequest = false;
	waitContext.HasBreakRequest = false;

	SleepUntilResumeLoopOutcome waitOutcome = ResolveSleepUntilResumeLoopOutcome(waitContext);
	EXPECT_TRUE(waitOutcome.ShouldContinueWaiting);
	EXPECT_EQ(waitOutcome.WaitDelayMs, 10);

	SleepUntilResumeLoopContext breakContext = {};
	breakContext.WaitForBreakResume = false;
	breakContext.HasSuspendRequest = true;
	breakContext.HasBreakRequest = true;

	SleepUntilResumeLoopOutcome breakOutcome = ResolveSleepUntilResumeLoopOutcome(breakContext);
	EXPECT_TRUE(breakOutcome.ShouldContinueWaiting);
	EXPECT_EQ(breakOutcome.WaitDelayMs, 1);
}

TEST(DebuggerDispatchUtilsTests, SleepUntilResumeLoopOutcomeStopsWhenNoWaitAndNoBreakRequest) {
	SleepUntilResumeLoopContext context = {};
	context.WaitForBreakResume = false;
	context.HasSuspendRequest = false;
	context.HasBreakRequest = false;

	SleepUntilResumeLoopOutcome outcome = ResolveSleepUntilResumeLoopOutcome(context);
	EXPECT_FALSE(outcome.ShouldContinueWaiting);
	EXPECT_EQ(outcome.WaitDelayMs, 10);
}

TEST(DebuggerDispatchUtilsTests, SleepUntilResumeLoopOutcomeStopsWaitPathWhenSuspendRequestedWithoutBreakRequest) {
	SleepUntilResumeLoopContext context = {};
	context.WaitForBreakResume = true;
	context.HasSuspendRequest = true;
	context.HasBreakRequest = false;

	SleepUntilResumeLoopOutcome outcome = ResolveSleepUntilResumeLoopOutcome(context);
	EXPECT_FALSE(outcome.ShouldContinueWaiting);
	EXPECT_EQ(outcome.WaitDelayMs, 10);
}

TEST(DebuggerDispatchUtilsTests, SleepUntilResumePostLoopOutcomeDisablesSideEffectsWhenNoNotificationWasSent) {
	SleepUntilResumePostLoopContext context = {};
	context.NotificationSent = false;

	SleepUntilResumePostLoopOutcome outcome = ResolveSleepUntilResumePostLoopOutcome(context);
	EXPECT_FALSE(outcome.ShouldDisableScreensaver);
	EXPECT_FALSE(outcome.ShouldSendDebuggerResumedNotification);
}

TEST(DebuggerDispatchUtilsTests, SleepUntilResumePostLoopOutcomeEnablesSideEffectsWhenNotificationWasSent) {
	SleepUntilResumePostLoopContext context = {};
	context.NotificationSent = true;

	SleepUntilResumePostLoopOutcome outcome = ResolveSleepUntilResumePostLoopOutcome(context);
	EXPECT_TRUE(outcome.ShouldDisableScreensaver);
	EXPECT_TRUE(outcome.ShouldSendDebuggerResumedNotification);
}

TEST(DebuggerDispatchUtilsTests, SleepUntilResumeBreakEventOutcomeIncludesCorePayloadFields) {
	SleepUntilResumeBreakEventContext context = {};
	context.SourceCpu = CpuType::Nes;
	context.Source = BreakSource::Breakpoint;
	context.BreakpointId = 77;

	SleepUntilResumeBreakEventOutcome outcome = ResolveSleepUntilResumeBreakEventOutcome(context);
	EXPECT_EQ(outcome.Event.SourceCpu, CpuType::Nes);
	EXPECT_EQ(outcome.Event.Source, BreakSource::Breakpoint);
	EXPECT_EQ(outcome.Event.BreakpointId, 77);
	EXPECT_FALSE(outcome.HasOperation);
}

TEST(DebuggerDispatchUtilsTests, SleepUntilResumeBreakEventOutcomeCopiesOperationWhenProvided) {
	MemoryOperationInfo operation = {};
	operation.Address = 0x1234;
	operation.Value = 0x56;
	operation.Type = (MemoryOperationType)2;
	operation.MemType = (MemoryType)3;

	SleepUntilResumeBreakEventContext context = {};
	context.SourceCpu = CpuType::Snes;
	context.Source = BreakSource::CpuStep;
	context.BreakpointId = -1;
	context.Operation = &operation;

	SleepUntilResumeBreakEventOutcome outcome = ResolveSleepUntilResumeBreakEventOutcome(context);
	EXPECT_TRUE(outcome.HasOperation);
	EXPECT_EQ(outcome.Event.Operation.Address, 0x1234u);
	EXPECT_EQ(outcome.Event.Operation.Value, 0x56);
	EXPECT_EQ((int)outcome.Event.Operation.Type, 2);
	EXPECT_EQ((int)outcome.Event.Operation.MemType, 3);
}

TEST(DebuggerDispatchUtilsTests, SleepUntilResumeDispatchOutcomeDisablesDispatchAndSentStateWhenSequenceIsDisabled) {
	SleepUntilResumeDispatchContext context = {};
	context.ShouldRunPreBreakSequence = false;

	SleepUntilResumeDispatchOutcome outcome = ResolveSleepUntilResumeDispatchOutcome(context);
	EXPECT_FALSE(outcome.ShouldDispatchCodeBreakNotification);
	EXPECT_FALSE(outcome.ShouldProcessCodeBreakEvent);
	EXPECT_FALSE(outcome.ShouldMarkNotificationSent);
}

TEST(DebuggerDispatchUtilsTests, SleepUntilResumeDispatchOutcomeEnablesDispatchAndSentStateWhenSequenceIsEnabled) {
	SleepUntilResumeDispatchContext context = {};
	context.ShouldRunPreBreakSequence = true;

	SleepUntilResumeDispatchOutcome outcome = ResolveSleepUntilResumeDispatchOutcome(context);
	EXPECT_TRUE(outcome.ShouldDispatchCodeBreakNotification);
	EXPECT_TRUE(outcome.ShouldProcessCodeBreakEvent);
	EXPECT_TRUE(outcome.ShouldMarkNotificationSent);
}

TEST(DebuggerDispatchUtilsTests, SleepUntilResumePreLoopBundleOutcomeDisablesPreLoopAndDispatchPoliciesWhenNotificationNotEmitted) {
	SleepUntilResumePreLoopBundleContext context = {};
	context.ShouldEmitBreakNotification = false;
	context.SingleBreakpointPerInstruction = true;
	context.DrawPartialFrame = true;

	SleepUntilResumePreLoopBundleOutcome outcome = ResolveSleepUntilResumePreLoopBundleOutcome(context);
	EXPECT_FALSE(outcome.PreBreak.ShouldIgnoreBreakpoints);
	EXPECT_FALSE(outcome.PreBreak.ShouldDrawPartialFrame);
	EXPECT_FALSE(outcome.PreLoop.ShouldRunPreBreakSequence);
	EXPECT_FALSE(outcome.PreLoop.ShouldArmWaitForBreakResume);
	EXPECT_FALSE(outcome.PreLoop.ShouldEnableScreensaver);
	EXPECT_FALSE(outcome.Dispatch.ShouldDispatchCodeBreakNotification);
	EXPECT_FALSE(outcome.Dispatch.ShouldProcessCodeBreakEvent);
	EXPECT_FALSE(outcome.Dispatch.ShouldMarkNotificationSent);
}

TEST(DebuggerDispatchUtilsTests, SleepUntilResumePreLoopBundleOutcomeEnablesPreLoopAndDispatchPoliciesWhenNotificationEmitted) {
	SleepUntilResumePreLoopBundleContext context = {};
	context.ShouldEmitBreakNotification = true;
	context.SingleBreakpointPerInstruction = true;
	context.DrawPartialFrame = true;

	SleepUntilResumePreLoopBundleOutcome outcome = ResolveSleepUntilResumePreLoopBundleOutcome(context);
	EXPECT_TRUE(outcome.PreBreak.ShouldIgnoreBreakpoints);
	EXPECT_TRUE(outcome.PreBreak.ShouldDrawPartialFrame);
	EXPECT_TRUE(outcome.PreLoop.ShouldRunPreBreakSequence);
	EXPECT_TRUE(outcome.PreLoop.ShouldArmWaitForBreakResume);
	EXPECT_TRUE(outcome.PreLoop.ShouldEnableScreensaver);
	EXPECT_TRUE(outcome.Dispatch.ShouldDispatchCodeBreakNotification);
	EXPECT_TRUE(outcome.Dispatch.ShouldProcessCodeBreakEvent);
	EXPECT_TRUE(outcome.Dispatch.ShouldMarkNotificationSent);
}

TEST(DebuggerDispatchUtilsTests, SleepUntilResumePhaseOutcomeCarriesGuardSkipDecisionWithoutPreLoopSideEffects) {
	SleepUntilResumePhaseContext context = {};
	context.Guard.HasSuspendRequest = true;
	context.Source = BreakSource::Breakpoint;
	context.HasBreakRequest = false;
	context.SingleBreakpointPerInstruction = true;
	context.DrawPartialFrame = true;

	SleepUntilResumePhaseOutcome outcome = ResolveSleepUntilResumePhaseOutcome(context);
	EXPECT_EQ(outcome.Decision, SleepUntilResumeDecision::SkipForSuspendRequest);
	EXPECT_FALSE(outcome.ShouldEmitBreakNotification);
	EXPECT_FALSE(outcome.PreLoopBundle.PreLoop.ShouldRunPreBreakSequence);
	EXPECT_FALSE(outcome.PreLoopBundle.Dispatch.ShouldDispatchCodeBreakNotification);
}

TEST(DebuggerDispatchUtilsTests, SleepUntilResumePhaseOutcomeComposesPreLoopBundleWhenNotificationIsEmitted) {
	SleepUntilResumePhaseContext context = {};
	context.Guard.HasSuspendRequest = false;
	context.Guard.ExecutionAlreadyStopped = false;
	context.Guard.HasBreakRequest = false;
	context.Guard.SourceCpuIsMainCpu = true;
	context.Guard.AllowChangeProgramCounter = true;
	context.Guard.BreakpointForbidden = false;
	context.Source = BreakSource::Breakpoint;
	context.HasBreakRequest = true;
	context.SingleBreakpointPerInstruction = true;
	context.DrawPartialFrame = true;

	SleepUntilResumePhaseOutcome outcome = ResolveSleepUntilResumePhaseOutcome(context);
	EXPECT_EQ(outcome.Decision, SleepUntilResumeDecision::Continue);
	EXPECT_TRUE(outcome.ShouldEmitBreakNotification);
	EXPECT_TRUE(outcome.PreLoopBundle.PreBreak.ShouldIgnoreBreakpoints);
	EXPECT_TRUE(outcome.PreLoopBundle.PreBreak.ShouldDrawPartialFrame);
	EXPECT_TRUE(outcome.PreLoopBundle.PreLoop.ShouldRunPreBreakSequence);
	EXPECT_TRUE(outcome.PreLoopBundle.Dispatch.ShouldProcessCodeBreakEvent);
}

TEST(DebuggerDispatchUtilsTests, SleepUntilResumePhaseOutcomeDisablesPreLoopBundleWhenNotificationNotEmitted) {
	SleepUntilResumePhaseContext context = {};
	context.Guard.HasSuspendRequest = false;
	context.Guard.ExecutionAlreadyStopped = false;
	context.Guard.HasBreakRequest = false;
	context.Guard.SourceCpuIsMainCpu = true;
	context.Guard.AllowChangeProgramCounter = true;
	context.Guard.BreakpointForbidden = false;
	context.Source = BreakSource::Unspecified;
	context.HasBreakRequest = true;
	context.SingleBreakpointPerInstruction = true;
	context.DrawPartialFrame = true;

	SleepUntilResumePhaseOutcome outcome = ResolveSleepUntilResumePhaseOutcome(context);
	EXPECT_EQ(outcome.Decision, SleepUntilResumeDecision::Continue);
	EXPECT_FALSE(outcome.ShouldEmitBreakNotification);
	EXPECT_FALSE(outcome.PreLoopBundle.PreBreak.ShouldIgnoreBreakpoints);
	EXPECT_FALSE(outcome.PreLoopBundle.PreBreak.ShouldDrawPartialFrame);
	EXPECT_FALSE(outcome.PreLoopBundle.PreLoop.ShouldRunPreBreakSequence);
	EXPECT_FALSE(outcome.PreLoopBundle.Dispatch.ShouldMarkNotificationSent);
}

TEST(DebuggerDispatchUtilsTests, SleepUntilResumePhaseOutcomeComposesLoopAndPostLoopPolicies) {
	SleepUntilResumePhaseContext context = {};
	context.WaitForBreakResume = false;
	context.HasSuspendRequest = true;
	context.HasBreakRequest = true;
	context.NotificationSent = true;

	SleepUntilResumePhaseOutcome outcome = ResolveSleepUntilResumePhaseOutcome(context);
	EXPECT_TRUE(outcome.Loop.ShouldContinueWaiting);
	EXPECT_EQ(outcome.Loop.WaitDelayMs, 1);
	EXPECT_TRUE(outcome.PostLoop.ShouldDisableScreensaver);
	EXPECT_TRUE(outcome.PostLoop.ShouldSendDebuggerResumedNotification);
}

TEST(DebuggerDispatchUtilsTests, SleepUntilResumeRuntimeDispatchOutcomeDisablesDispatchSequenceWhenNotRunningPreBreakSequence) {
	SleepUntilResumeRuntimeDispatchContext context = {};
	context.ShouldRunPreBreakSequence = false;
	context.SourceCpu = CpuType::Nes;
	context.Source = BreakSource::Breakpoint;
	context.BreakpointId = 42;

	SleepUntilResumeRuntimeDispatchOutcome outcome = ResolveSleepUntilResumeRuntimeDispatchOutcome(context);
	EXPECT_EQ(outcome.BreakEvent.Event.SourceCpu, CpuType::Nes);
	EXPECT_EQ(outcome.BreakEvent.Event.Source, BreakSource::Breakpoint);
	EXPECT_EQ(outcome.BreakEvent.Event.BreakpointId, 42);
	EXPECT_FALSE(outcome.Dispatch.ShouldDispatchCodeBreakNotification);
	EXPECT_FALSE(outcome.Dispatch.ShouldProcessCodeBreakEvent);
	EXPECT_FALSE(outcome.Dispatch.ShouldMarkNotificationSent);
}

TEST(DebuggerDispatchUtilsTests, SleepUntilResumeRuntimeDispatchOutcomeEnablesDispatchSequenceAndCopiesOperationWhenPreBreakSequenceRuns) {
	MemoryOperationInfo operation = {};
	operation.Address = 0x4567;
	operation.Value = 0x89;
	operation.Type = (MemoryOperationType)2;
	operation.MemType = (MemoryType)3;

	SleepUntilResumeRuntimeDispatchContext context = {};
	context.ShouldRunPreBreakSequence = true;
	context.SourceCpu = CpuType::Snes;
	context.Source = BreakSource::CpuStep;
	context.BreakpointId = 7;
	context.Operation = &operation;

	SleepUntilResumeRuntimeDispatchOutcome outcome = ResolveSleepUntilResumeRuntimeDispatchOutcome(context);
	EXPECT_TRUE(outcome.BreakEvent.HasOperation);
	EXPECT_EQ(outcome.BreakEvent.Event.Operation.Address, 0x4567u);
	EXPECT_EQ(outcome.BreakEvent.Event.Operation.Value, 0x89);
	EXPECT_TRUE(outcome.Dispatch.ShouldDispatchCodeBreakNotification);
	EXPECT_TRUE(outcome.Dispatch.ShouldProcessCodeBreakEvent);
	EXPECT_TRUE(outcome.Dispatch.ShouldMarkNotificationSent);
}

TEST(DebuggerDispatchUtilsTests, SleepUntilResumeRuntimeSideEffectOutcomeKeepsNotificationStateWhenDispatchDoesNotMarkSent) {
	SleepUntilResumeRuntimeSideEffectContext context = {};
	context.ShouldArmWaitForBreakResume = false;
	context.ShouldEnableScreensaver = false;
	context.ShouldMarkNotificationSent = false;
	context.NotificationSent = false;

	SleepUntilResumeRuntimeSideEffectOutcome outcome = ResolveSleepUntilResumeRuntimeSideEffectOutcome(context);
	EXPECT_FALSE(outcome.ShouldSetWaitForBreakResume);
	EXPECT_FALSE(outcome.ShouldEnableScreensaver);
	EXPECT_FALSE(outcome.NotificationSent);
}

TEST(DebuggerDispatchUtilsTests, SleepUntilResumeRuntimeSideEffectOutcomeAppliesWaitAndScreenAndPromotesNotificationSent) {
	SleepUntilResumeRuntimeSideEffectContext context = {};
	context.ShouldArmWaitForBreakResume = true;
	context.ShouldEnableScreensaver = true;
	context.ShouldMarkNotificationSent = true;
	context.NotificationSent = false;

	SleepUntilResumeRuntimeSideEffectOutcome outcome = ResolveSleepUntilResumeRuntimeSideEffectOutcome(context);
	EXPECT_TRUE(outcome.ShouldSetWaitForBreakResume);
	EXPECT_TRUE(outcome.ShouldEnableScreensaver);
	EXPECT_TRUE(outcome.NotificationSent);
}

TEST(DebuggerDispatchUtilsTests, SleepUntilResumeRuntimeDispatchContextBuilderMapsRuntimeBundlePayloadFields) {
	MemoryOperationInfo operation = {};
	operation.Address = 0x1357;

	SleepUntilResumeRuntimeBundleContext bundleContext = {};
	bundleContext.ShouldRunPreBreakSequence = true;
	bundleContext.SourceCpu = CpuType::Nes;
	bundleContext.Source = BreakSource::Breakpoint;
	bundleContext.BreakpointId = 55;
	bundleContext.Operation = &operation;

	SleepUntilResumeRuntimeDispatchContext dispatchContext = BuildSleepUntilResumeRuntimeDispatchContext(bundleContext);
	EXPECT_TRUE(dispatchContext.ShouldRunPreBreakSequence);
	EXPECT_EQ(dispatchContext.SourceCpu, CpuType::Nes);
	EXPECT_EQ(dispatchContext.Source, BreakSource::Breakpoint);
	EXPECT_EQ(dispatchContext.BreakpointId, 55);
	EXPECT_EQ(dispatchContext.Operation, &operation);
}

TEST(DebuggerDispatchUtilsTests, SleepUntilResumeRuntimeSideEffectContextBuilderMapsRuntimeBundleAndMarkSentInputs) {
	SleepUntilResumeRuntimeBundleContext bundleContext = {};
	bundleContext.ShouldArmWaitForBreakResume = true;
	bundleContext.ShouldEnableScreensaver = false;
	bundleContext.NotificationSent = true;

	SleepUntilResumeRuntimeSideEffectContext sideEffectContext = BuildSleepUntilResumeRuntimeSideEffectContext(bundleContext, true);
	EXPECT_TRUE(sideEffectContext.ShouldArmWaitForBreakResume);
	EXPECT_FALSE(sideEffectContext.ShouldEnableScreensaver);
	EXPECT_TRUE(sideEffectContext.ShouldMarkNotificationSent);
	EXPECT_TRUE(sideEffectContext.NotificationSent);
}

TEST(DebuggerDispatchUtilsTests, SleepUntilResumeRuntimeSideEffectApplicationContextBuilderMapsRuntimeStateAndSideEffects) {
	SleepUntilResumeRuntimeBundleOutcome runtimeBundleOutcome = {};
	runtimeBundleOutcome.SideEffect.ShouldSetWaitForBreakResume = true;
	runtimeBundleOutcome.SideEffect.ShouldEnableScreensaver = false;
	runtimeBundleOutcome.SideEffect.NotificationSent = true;

	SleepUntilResumeRuntimeSideEffectApplicationContext context = BuildSleepUntilResumeRuntimeSideEffectApplicationContext(false, false, runtimeBundleOutcome);
	EXPECT_FALSE(context.WaitForBreakResume);
	EXPECT_FALSE(context.NotificationSent);
	EXPECT_TRUE(context.SideEffect.ShouldSetWaitForBreakResume);
	EXPECT_FALSE(context.SideEffect.ShouldEnableScreensaver);
	EXPECT_TRUE(context.SideEffect.NotificationSent);
}

TEST(DebuggerDispatchUtilsTests, SleepUntilResumeRuntimeSideEffectApplicationOutcomePromotesWaitAndNotificationState) {
	SleepUntilResumeRuntimeSideEffectApplicationContext context = {};
	context.WaitForBreakResume = false;
	context.NotificationSent = false;
	context.SideEffect.ShouldSetWaitForBreakResume = true;
	context.SideEffect.ShouldEnableScreensaver = true;
	context.SideEffect.NotificationSent = true;

	SleepUntilResumeRuntimeSideEffectApplicationOutcome outcome = ResolveSleepUntilResumeRuntimeSideEffectApplicationOutcome(context);
	EXPECT_TRUE(outcome.WaitForBreakResume);
	EXPECT_TRUE(outcome.NotificationSent);
	EXPECT_TRUE(outcome.ShouldEnableScreensaver);
}

TEST(DebuggerDispatchUtilsTests, SleepUntilResumeRuntimeSideEffectApplicationOutcomePreservesExistingStateWhenNoPromotionRequested) {
	SleepUntilResumeRuntimeSideEffectApplicationContext context = {};
	context.WaitForBreakResume = true;
	context.NotificationSent = true;
	context.SideEffect.ShouldSetWaitForBreakResume = false;
	context.SideEffect.ShouldEnableScreensaver = false;
	context.SideEffect.NotificationSent = false;

	SleepUntilResumeRuntimeSideEffectApplicationOutcome outcome = ResolveSleepUntilResumeRuntimeSideEffectApplicationOutcome(context);
	EXPECT_TRUE(outcome.WaitForBreakResume);
	EXPECT_TRUE(outcome.NotificationSent);
	EXPECT_FALSE(outcome.ShouldEnableScreensaver);
}

TEST(DebuggerDispatchUtilsTests, SleepUntilResumeRuntimeDispatchExecutionContextBuilderMapsRuntimeDispatchPayload) {
	SleepUntilResumeRuntimeBundleOutcome runtimeBundleOutcome = {};
	runtimeBundleOutcome.Dispatch.Dispatch.ShouldDispatchCodeBreakNotification = true;
	runtimeBundleOutcome.Dispatch.Dispatch.ShouldProcessCodeBreakEvent = false;
	runtimeBundleOutcome.Dispatch.BreakEvent.Event.SourceCpu = CpuType::Nes;
	runtimeBundleOutcome.Dispatch.BreakEvent.Event.Source = BreakSource::Breakpoint;
	runtimeBundleOutcome.Dispatch.BreakEvent.Event.BreakpointId = 17;

	SleepUntilResumeRuntimeDispatchExecutionContext context = BuildSleepUntilResumeRuntimeDispatchExecutionContext(runtimeBundleOutcome);
	EXPECT_TRUE(context.Dispatch.Dispatch.ShouldDispatchCodeBreakNotification);
	EXPECT_FALSE(context.Dispatch.Dispatch.ShouldProcessCodeBreakEvent);
	EXPECT_EQ(context.Dispatch.BreakEvent.Event.SourceCpu, CpuType::Nes);
	EXPECT_EQ(context.Dispatch.BreakEvent.Event.Source, BreakSource::Breakpoint);
	EXPECT_EQ(context.Dispatch.BreakEvent.Event.BreakpointId, 17);
}

TEST(DebuggerDispatchUtilsTests, SleepUntilResumeRuntimeDispatchExecutionOutcomeMapsDispatchFlagsAndEventPayload) {
	SleepUntilResumeRuntimeDispatchExecutionContext context = {};
	context.Dispatch.Dispatch.ShouldDispatchCodeBreakNotification = true;
	context.Dispatch.Dispatch.ShouldProcessCodeBreakEvent = true;
	context.Dispatch.BreakEvent.Event.SourceCpu = CpuType::Gba;
	context.Dispatch.BreakEvent.Event.Source = BreakSource::CpuStep;
	context.Dispatch.BreakEvent.Event.BreakpointId = 9;

	SleepUntilResumeRuntimeDispatchExecutionOutcome outcome = ResolveSleepUntilResumeRuntimeDispatchExecutionOutcome(context);
	EXPECT_TRUE(outcome.ShouldDispatchCodeBreakNotification);
	EXPECT_TRUE(outcome.ShouldProcessCodeBreakEvent);
	EXPECT_EQ(outcome.Event.SourceCpu, CpuType::Gba);
	EXPECT_EQ(outcome.Event.Source, BreakSource::CpuStep);
	EXPECT_EQ(outcome.Event.BreakpointId, 9);
}

TEST(DebuggerDispatchUtilsTests, SleepUntilResumeLoopContextBuilderMapsLoopPostBundleFields) {
	SleepUntilResumeLoopPostBundleContext bundleContext = {};
	bundleContext.WaitForBreakResume = true;
	bundleContext.HasSuspendRequest = false;
	bundleContext.HasBreakRequest = true;
	bundleContext.NotificationSent = true;

	SleepUntilResumeLoopContext loopContext = BuildSleepUntilResumeLoopContext(bundleContext);
	EXPECT_TRUE(loopContext.WaitForBreakResume);
	EXPECT_FALSE(loopContext.HasSuspendRequest);
	EXPECT_TRUE(loopContext.HasBreakRequest);
}

TEST(DebuggerDispatchUtilsTests, SleepUntilResumePostLoopContextBuilderMapsLoopPostBundleNotificationState) {
	SleepUntilResumeLoopPostBundleContext bundleContext = {};
	bundleContext.WaitForBreakResume = false;
	bundleContext.HasSuspendRequest = true;
	bundleContext.HasBreakRequest = false;
	bundleContext.NotificationSent = true;

	SleepUntilResumePostLoopContext postLoopContext = BuildSleepUntilResumePostLoopContext(bundleContext);
	EXPECT_TRUE(postLoopContext.NotificationSent);
}

TEST(DebuggerDispatchUtilsTests, SleepUntilResumePreBreakContextBuilderMapsPreLoopBundleFields) {
	SleepUntilResumePreLoopBundleContext bundleContext = {};
	bundleContext.ShouldEmitBreakNotification = true;
	bundleContext.SingleBreakpointPerInstruction = true;
	bundleContext.DrawPartialFrame = false;

	SleepUntilResumePreBreakContext preBreakContext = BuildSleepUntilResumePreBreakContext(bundleContext);
	EXPECT_TRUE(preBreakContext.ShouldEmitBreakNotification);
	EXPECT_TRUE(preBreakContext.SingleBreakpointPerInstruction);
	EXPECT_FALSE(preBreakContext.DrawPartialFrame);
}

TEST(DebuggerDispatchUtilsTests, SleepUntilResumePreLoopContextBuilderMapsPreLoopBundleNotificationField) {
	SleepUntilResumePreLoopBundleContext bundleContext = {};
	bundleContext.ShouldEmitBreakNotification = false;
	bundleContext.SingleBreakpointPerInstruction = true;
	bundleContext.DrawPartialFrame = true;

	SleepUntilResumePreLoopContext preLoopContext = BuildSleepUntilResumePreLoopContext(bundleContext);
	EXPECT_FALSE(preLoopContext.ShouldEmitBreakNotification);
}

TEST(DebuggerDispatchUtilsTests, SleepUntilResumeDispatchContextBuilderMapsPreLoopOutcomeRunSequenceFlag) {
	SleepUntilResumePreLoopOutcome preLoopOutcome = {};
	preLoopOutcome.ShouldRunPreBreakSequence = true;
	preLoopOutcome.ShouldArmWaitForBreakResume = true;
	preLoopOutcome.ShouldEnableScreensaver = false;

	SleepUntilResumeDispatchContext dispatchContext = BuildSleepUntilResumeDispatchContext(preLoopOutcome);
	EXPECT_TRUE(dispatchContext.ShouldRunPreBreakSequence);
}

TEST(DebuggerDispatchUtilsTests, SleepUntilResumePreLoopBundleContextBuilderMapsPhaseOutcomeInputs) {
	SleepUntilResumePhaseContext phaseContext = {};
	phaseContext.SingleBreakpointPerInstruction = true;
	phaseContext.DrawPartialFrame = false;

	SleepUntilResumePreLoopBundleContext preLoopBundleContext = BuildSleepUntilResumePreLoopBundleContext(phaseContext, true);
	EXPECT_TRUE(preLoopBundleContext.ShouldEmitBreakNotification);
	EXPECT_TRUE(preLoopBundleContext.SingleBreakpointPerInstruction);
	EXPECT_FALSE(preLoopBundleContext.DrawPartialFrame);
}

TEST(DebuggerDispatchUtilsTests, SleepUntilResumeLoopContextBuilderMapsPhaseContextLoopFields) {
	SleepUntilResumePhaseContext phaseContext = {};
	phaseContext.WaitForBreakResume = true;
	phaseContext.HasSuspendRequest = false;
	phaseContext.HasBreakRequest = true;

	SleepUntilResumeLoopContext loopContext = BuildSleepUntilResumeLoopContext(phaseContext);
	EXPECT_TRUE(loopContext.WaitForBreakResume);
	EXPECT_FALSE(loopContext.HasSuspendRequest);
	EXPECT_TRUE(loopContext.HasBreakRequest);
}

TEST(DebuggerDispatchUtilsTests, SleepUntilResumePostLoopContextBuilderMapsPhaseContextNotificationState) {
	SleepUntilResumePhaseContext phaseContext = {};
	phaseContext.NotificationSent = true;

	SleepUntilResumePostLoopContext postLoopContext = BuildSleepUntilResumePostLoopContext(phaseContext);
	EXPECT_TRUE(postLoopContext.NotificationSent);
}

TEST(DebuggerDispatchUtilsTests, SleepUntilResumeRuntimeBundleContextBuilderComposesPhasePreLoopFlagsAndRuntimePayloadForEmittedFlow) {
	MemoryOperationInfo operation = {};
	operation.Address = 0x2468;

	SleepUntilResumePhaseContext phaseContext = CreateSleepUntilResumeContinuePhaseContext(BreakSource::Breakpoint, true);
	SleepUntilResumePhaseOutcome phaseOutcome = ResolveSleepUntilResumePhaseOutcome(phaseContext);

	SleepUntilResumeRuntimeBundleContext context = BuildSleepUntilResumeRuntimeBundleContext(phaseOutcome, CpuType::Nes, BreakSource::Breakpoint, 41, &operation, false);
	EXPECT_TRUE(context.ShouldRunPreBreakSequence);
	EXPECT_EQ(context.SourceCpu, CpuType::Nes);
	EXPECT_EQ(context.Source, BreakSource::Breakpoint);
	EXPECT_EQ(context.BreakpointId, 41);
	EXPECT_EQ(context.Operation, &operation);
	EXPECT_TRUE(context.ShouldArmWaitForBreakResume);
	EXPECT_TRUE(context.ShouldEnableScreensaver);
	EXPECT_FALSE(context.NotificationSent);
}

TEST(DebuggerDispatchUtilsTests, SleepUntilResumeRuntimeBundleContextBuilderDisablesPreBreakFlagsForNonEmittedFlowAndPreservesPayload) {
	SleepUntilResumePhaseContext phaseContext = CreateSleepUntilResumeContinuePhaseContext(BreakSource::Unspecified, true);
	SleepUntilResumePhaseOutcome phaseOutcome = ResolveSleepUntilResumePhaseOutcome(phaseContext);

	SleepUntilResumeRuntimeBundleContext context = BuildSleepUntilResumeRuntimeBundleContext(phaseOutcome, CpuType::Gba, BreakSource::Unspecified, -1, nullptr, true);
	EXPECT_FALSE(context.ShouldRunPreBreakSequence);
	EXPECT_EQ(context.SourceCpu, CpuType::Gba);
	EXPECT_EQ(context.Source, BreakSource::Unspecified);
	EXPECT_EQ(context.BreakpointId, -1);
	EXPECT_EQ(context.Operation, nullptr);
	EXPECT_FALSE(context.ShouldArmWaitForBreakResume);
	EXPECT_FALSE(context.ShouldEnableScreensaver);
	EXPECT_TRUE(context.NotificationSent);
}

TEST(DebuggerDispatchUtilsTests, SleepUntilResumeRuntimeBundleOutcomeDisablesDispatchAndSideEffectsWhenPreBreakSequenceDoesNotRun) {
	SleepUntilResumeRuntimeBundleContext context = {};
	context.ShouldRunPreBreakSequence = false;
	context.SourceCpu = CpuType::Nes;
	context.Source = BreakSource::Unspecified;
	context.BreakpointId = -1;
	context.ShouldArmWaitForBreakResume = false;
	context.ShouldEnableScreensaver = false;
	context.NotificationSent = false;

	SleepUntilResumeRuntimeBundleOutcome outcome = ResolveSleepUntilResumeRuntimeBundleOutcome(context);
	EXPECT_FALSE(outcome.Dispatch.Dispatch.ShouldDispatchCodeBreakNotification);
	EXPECT_FALSE(outcome.Dispatch.Dispatch.ShouldProcessCodeBreakEvent);
	EXPECT_FALSE(outcome.Dispatch.Dispatch.ShouldMarkNotificationSent);
	EXPECT_FALSE(outcome.SideEffect.ShouldSetWaitForBreakResume);
	EXPECT_FALSE(outcome.SideEffect.ShouldEnableScreensaver);
	EXPECT_FALSE(outcome.SideEffect.NotificationSent);
}

TEST(DebuggerDispatchUtilsTests, SleepUntilResumeRuntimeBundleOutcomeComposesDispatchPayloadAndSideEffectStateTransitions) {
	MemoryOperationInfo operation = {};
	operation.Address = 0x6789;
	operation.Value = 0xab;

	SleepUntilResumeRuntimeBundleContext context = {};
	context.ShouldRunPreBreakSequence = true;
	context.SourceCpu = CpuType::Snes;
	context.Source = BreakSource::Breakpoint;
	context.BreakpointId = 12;
	context.Operation = &operation;
	context.ShouldArmWaitForBreakResume = true;
	context.ShouldEnableScreensaver = true;
	context.NotificationSent = false;

	SleepUntilResumeRuntimeBundleOutcome outcome = ResolveSleepUntilResumeRuntimeBundleOutcome(context);
	EXPECT_TRUE(outcome.Dispatch.Dispatch.ShouldDispatchCodeBreakNotification);
	EXPECT_TRUE(outcome.Dispatch.Dispatch.ShouldProcessCodeBreakEvent);
	EXPECT_TRUE(outcome.Dispatch.Dispatch.ShouldMarkNotificationSent);
	EXPECT_TRUE(outcome.Dispatch.BreakEvent.HasOperation);
	EXPECT_EQ(outcome.Dispatch.BreakEvent.Event.BreakpointId, 12);
	EXPECT_EQ(outcome.Dispatch.BreakEvent.Event.Operation.Address, 0x6789u);
	EXPECT_TRUE(outcome.SideEffect.ShouldSetWaitForBreakResume);
	EXPECT_TRUE(outcome.SideEffect.ShouldEnableScreensaver);
	EXPECT_TRUE(outcome.SideEffect.NotificationSent);
}

TEST(DebuggerDispatchUtilsTests, SleepUntilResumeComposedFlowIntegrationEmittedPathTransitionsAcrossPhaseRuntimeAndPostLoop) {
	MemoryOperationInfo operation = {};
	operation.Address = 0x3456;
	operation.Value = 0x78;

	SleepUntilResumeGuardContext guardContext = {};
	guardContext.HasSuspendRequest = false;
	guardContext.ExecutionAlreadyStopped = false;
	guardContext.HasBreakRequest = true;
	guardContext.SourceCpuIsMainCpu = true;
	guardContext.AllowChangeProgramCounter = true;
	guardContext.BreakpointForbidden = false;

	SleepUntilResumePhaseContext phaseContext = BuildSleepUntilResumePhaseContext(guardContext, BreakSource::Breakpoint, true, true, true);
	SleepUntilResumePhaseOutcome phaseOutcome = ResolveSleepUntilResumePhaseOutcome(phaseContext);
	EXPECT_EQ(phaseOutcome.Decision, SleepUntilResumeDecision::Continue);
	EXPECT_TRUE(phaseOutcome.ShouldEmitBreakNotification);
	EXPECT_TRUE(phaseOutcome.PreLoopBundle.PreLoop.ShouldRunPreBreakSequence);

	SleepUntilResumeRuntimeBundleContext runtimeBundleContext = CreateRuntimeBundleContext(phaseOutcome, CpuType::Nes, BreakSource::Breakpoint, 99, &operation, false);
	SleepUntilResumeRuntimeBundleOutcome runtimeBundleOutcome = ResolveSleepUntilResumeRuntimeBundleOutcome(runtimeBundleContext);
	EXPECT_TRUE(runtimeBundleOutcome.Dispatch.Dispatch.ShouldDispatchCodeBreakNotification);
	EXPECT_TRUE(runtimeBundleOutcome.Dispatch.BreakEvent.HasOperation);
	EXPECT_TRUE(runtimeBundleOutcome.SideEffect.ShouldSetWaitForBreakResume);
	EXPECT_TRUE(runtimeBundleOutcome.SideEffect.ShouldEnableScreensaver);
	EXPECT_TRUE(runtimeBundleOutcome.SideEffect.NotificationSent);

	SleepUntilResumeLoopContext loopContext = {};
	loopContext.WaitForBreakResume = runtimeBundleOutcome.SideEffect.ShouldSetWaitForBreakResume;
	loopContext.HasSuspendRequest = false;
	loopContext.HasBreakRequest = true;
	SleepUntilResumeLoopOutcome loopOutcome = ResolveSleepUntilResumeLoopOutcome(loopContext);
	EXPECT_TRUE(loopOutcome.ShouldContinueWaiting);
	EXPECT_EQ(loopOutcome.WaitDelayMs, 1);

	SleepUntilResumePostLoopContext postLoopContext = {};
	postLoopContext.NotificationSent = runtimeBundleOutcome.SideEffect.NotificationSent;
	SleepUntilResumePostLoopOutcome postLoopOutcome = ResolveSleepUntilResumePostLoopOutcome(postLoopContext);
	EXPECT_TRUE(postLoopOutcome.ShouldDisableScreensaver);
	EXPECT_TRUE(postLoopOutcome.ShouldSendDebuggerResumedNotification);
}

TEST(DebuggerDispatchUtilsTests, SleepUntilResumeComposedFlowIntegrationNonEmittedPathSkipsRuntimeDispatchAndPostLoopSideEffects) {
	SleepUntilResumePhaseContext phaseContext = CreateSleepUntilResumeContinuePhaseContext(BreakSource::Unspecified, true);
	SleepUntilResumePhaseOutcome phaseOutcome = ResolveSleepUntilResumePhaseOutcome(phaseContext);
	EXPECT_EQ(phaseOutcome.Decision, SleepUntilResumeDecision::Continue);
	EXPECT_FALSE(phaseOutcome.ShouldEmitBreakNotification);
	EXPECT_FALSE(phaseOutcome.PreLoopBundle.PreLoop.ShouldRunPreBreakSequence);

	SleepUntilResumeRuntimeBundleContext runtimeBundleContext = CreateRuntimeBundleContext(phaseOutcome, CpuType::Nes, BreakSource::Unspecified, -1, nullptr, false);
	SleepUntilResumeRuntimeBundleOutcome runtimeBundleOutcome = ResolveSleepUntilResumeRuntimeBundleOutcome(runtimeBundleContext);
	EXPECT_FALSE(runtimeBundleOutcome.Dispatch.Dispatch.ShouldDispatchCodeBreakNotification);
	EXPECT_FALSE(runtimeBundleOutcome.Dispatch.Dispatch.ShouldProcessCodeBreakEvent);
	EXPECT_FALSE(runtimeBundleOutcome.SideEffect.ShouldSetWaitForBreakResume);
	EXPECT_FALSE(runtimeBundleOutcome.SideEffect.ShouldEnableScreensaver);
	EXPECT_FALSE(runtimeBundleOutcome.SideEffect.NotificationSent);

	SleepUntilResumeLoopContext loopContext = {};
	loopContext.WaitForBreakResume = runtimeBundleOutcome.SideEffect.ShouldSetWaitForBreakResume;
	loopContext.HasSuspendRequest = false;
	loopContext.HasBreakRequest = true;
	SleepUntilResumeLoopOutcome loopOutcome = ResolveSleepUntilResumeLoopOutcome(loopContext);
	EXPECT_TRUE(loopOutcome.ShouldContinueWaiting);
	EXPECT_EQ(loopOutcome.WaitDelayMs, 1);

	SleepUntilResumePostLoopContext postLoopContext = {};
	postLoopContext.NotificationSent = runtimeBundleOutcome.SideEffect.NotificationSent;
	SleepUntilResumePostLoopOutcome postLoopOutcome = ResolveSleepUntilResumePostLoopOutcome(postLoopContext);
	EXPECT_FALSE(postLoopOutcome.ShouldDisableScreensaver);
	EXPECT_FALSE(postLoopOutcome.ShouldSendDebuggerResumedNotification);
}

TEST(DebuggerDispatchUtilsTests, SleepUntilResumeLoopPostBundleOutcomeComposesWaitingLoopWithBreakRequestDelayPolicy) {
	SleepUntilResumeLoopPostBundleContext context = {};
	context.WaitForBreakResume = true;
	context.HasSuspendRequest = false;
	context.HasBreakRequest = true;
	context.NotificationSent = false;

	SleepUntilResumeLoopPostBundleOutcome outcome = ResolveSleepUntilResumeLoopPostBundleOutcome(context);
	EXPECT_TRUE(outcome.Loop.ShouldContinueWaiting);
	EXPECT_EQ(outcome.Loop.WaitDelayMs, 1);
	EXPECT_FALSE(outcome.PostLoop.ShouldDisableScreensaver);
	EXPECT_FALSE(outcome.PostLoop.ShouldSendDebuggerResumedNotification);
}

TEST(DebuggerDispatchUtilsTests, SleepUntilResumeLoopPostBundleOutcomeComposesNonWaitingLoopWithNotificationDrivenPostLoopSideEffects) {
	SleepUntilResumeLoopPostBundleContext context = {};
	context.WaitForBreakResume = false;
	context.HasSuspendRequest = false;
	context.HasBreakRequest = false;
	context.NotificationSent = true;

	SleepUntilResumeLoopPostBundleOutcome outcome = ResolveSleepUntilResumeLoopPostBundleOutcome(context);
	EXPECT_FALSE(outcome.Loop.ShouldContinueWaiting);
	EXPECT_EQ(outcome.Loop.WaitDelayMs, 10);
	EXPECT_TRUE(outcome.PostLoop.ShouldDisableScreensaver);
	EXPECT_TRUE(outcome.PostLoop.ShouldSendDebuggerResumedNotification);
}
