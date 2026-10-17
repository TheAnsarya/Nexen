using Nexen.Debugger.RegisterViewer;
using Nexen.Debugger.ViewModels;
using Nexen.Interop;
using Xunit;

namespace Nexen.Tests.Debugger.ViewModels;

/// <summary>
/// Tests for the Lynx register viewer UI integration.
/// Verifies all 5 tabs (CPU, Mikey, Suzy, Math, APU) are correctly structured
/// with the right entry counts, labels, CpuType, and MemoryType.
/// </summary>
public class LynxRegisterViewerTests {
	#region Tab Structure

	[Fact]
	public void GetTabs_Returns5Tabs() {
		var tabs = GetAllTabs();
		Assert.Equal(5, tabs.Count);
	}

	[Theory]
	[InlineData(0, "CPU")]
	[InlineData(1, "Mikey")]
	[InlineData(2, "Suzy")]
	[InlineData(3, "Math")]
	[InlineData(4, "APU")]
	public void GetTabs_CorrectNames(int index, string expectedName) {
		var tabs = GetAllTabs();
		Assert.Equal(expectedName, tabs[index].TabName);
	}

	[Fact]
	public void AllTabs_UseLynxCpuType() {
		var tabs = GetAllTabs();
		foreach (var tab in tabs) {
			Assert.Equal(CpuType.Lynx, tab.CpuType);
		}
	}

	[Fact]
	public void AllTabs_UseLynxMemoryType() {
		var tabs = GetAllTabs();
		foreach (var tab in tabs) {
			Assert.Equal(MemoryType.LynxMemory, tab.MemoryType);
		}
	}

	#endregion

	#region CPU Tab

	[Fact]
	public void CpuTab_HasCoreRegisters() {
		var tab = GetTabByName("CPU");
		var names = GetEntryNames(tab);

		Assert.Contains("A", names);
		Assert.Contains("X", names);
		Assert.Contains("Y", names);
		Assert.Contains("SP", names);
		Assert.Contains("PC", names);
		Assert.Contains("PS", names);
	}

	[Fact]
	public void CpuTab_HasPSFlagBreakdown() {
		var tab = GetTabByName("CPU");
		var names = GetEntryNames(tab);

		Assert.Contains("Negative", names);
		Assert.Contains("Overflow", names);
		Assert.Contains("Decimal", names);
		Assert.Contains("IRQ Disable", names);
		Assert.Contains("Zero", names);
		Assert.Contains("Carry", names);
	}

	[Fact]
	public void CpuTab_HasIrqNmiState() {
		var tab = GetTabByName("CPU");
		var names = GetEntryNames(tab);

		Assert.Contains("IRQ Flag", names);
		Assert.Contains("NMI Flag", names);
		Assert.Contains("Stop State", names);
	}

	#endregion

	#region Mikey Tab

	[Fact]
	public void MikeyTab_HasIrqAndDisplay() {
		var tab = GetTabByName("Mikey");
		var names = GetEntryNames(tab);

		Assert.Contains("IRQ Enabled", names);
		Assert.Contains("IRQ Pending", names);
		Assert.Contains("Display Control", names);
	}

	[Fact]
	public void MikeyTab_HasUartSection() {
		var tab = GetTabByName("Mikey");
		var names = GetEntryNames(tab);

		Assert.Contains("Serial Control (SERCTL)", names);
	}

	[Fact]
	public void MikeyTab_Has8Timers() {
		var tab = GetTabByName("Mikey");
		var names = GetEntryNames(tab);

		for (int i = 0; i < 8; i++) {
			Assert.Contains($"Timer {i}", names);
		}
	}

	[Fact]
	public void MikeyTab_TimerEntries_HaveCountAndBackup() {
		var tab = GetTabByName("Mikey");
		var names = GetEntryNames(tab);

		Assert.Contains("Count", names);
		Assert.Contains("Backup", names);
	}

	#endregion

	#region Suzy Tab

	[Fact]
	public void SuzyTab_HasSpriteState() {
		var tab = GetTabByName("Suzy");
		var names = GetEntryNames(tab);

		Assert.Contains("Sprite Busy", names);
		Assert.Contains("Sprite Enabled", names);
	}

	[Fact]
	public void SuzyTab_HasSCBAddress() {
		var tab = GetTabByName("Suzy");
		var names = GetEntryNames(tab);

		Assert.Contains("SCB Address", names);
	}

	[Fact]
	public void SuzyTab_HasJoystickAndSwitches() {
		var tab = GetTabByName("Suzy");
		var names = GetEntryNames(tab);

		Assert.Contains("Joystick", names);
		Assert.Contains("Switches", names);
	}

	[Fact]
	public void SuzyTab_HasSPRSYSFlags() {
		var tab = GetTabByName("Suzy");
		var names = GetEntryNames(tab);

		Assert.Contains("SPRSYS Flags", names);
		Assert.Contains("Unsafe Access", names);
		Assert.Contains("Sprite Collision", names);
	}

	#endregion

	#region Math Tab

	[Fact]
	public void MathTab_HasOperandRegisters() {
		var tab = GetTabByName("Math");
		var names = GetEntryNames(tab);

		Assert.Contains("ABCD", names);
		Assert.Contains("EFGH", names);
		Assert.Contains("JKLM", names);
		Assert.Contains("NP", names);
	}

	[Fact]
	public void MathTab_HasStatusFlags() {
		var tab = GetTabByName("Math");
		var names = GetEntryNames(tab);

		Assert.Contains("Signed Mode", names);
		Assert.Contains("Accumulate", names);
		Assert.Contains("In Progress", names);
	}

	[Fact]
	public void MathTab_HasPerByteBreakdown() {
		var tab = GetTabByName("Math");
		var names = GetEntryNames(tab);

		Assert.Contains("A (byte 3)", names);
		Assert.Contains("B (byte 2)", names);
		Assert.Contains("C (byte 1)", names);
		Assert.Contains("D (byte 0)", names);
	}

	#endregion

	#region APU Tab

	[Fact]
	public void ApuTab_HasMasterSettings() {
		var tab = GetTabByName("APU");
		var names = GetEntryNames(tab);

		Assert.Contains("MSTEREO", names);
		Assert.Contains("MPAN", names);
	}

	[Fact]
	public void ApuTab_Has4Channels() {
		var tab = GetTabByName("APU");
		var names = GetEntryNames(tab);

		for (int i = 0; i < 3; i++) {
			Assert.Contains($"Channel {i}", names);
		}
		Assert.Contains("Channel 3 (DAC)", names);
	}

	[Fact]
	public void ApuTab_ChannelEntries_HaveVolumeAndFeedback() {
		var tab = GetTabByName("APU");
		var names = GetEntryNames(tab);

		Assert.Contains("Volume", names);
		Assert.Contains("Feedback Enable", names);
	}

	[Fact]
	public void ApuTab_ChannelEntries_HaveShiftRegister() {
		var tab = GetTabByName("APU");
		var names = GetEntryNames(tab);

		Assert.Contains("Shift Register", names);
	}

	[Fact]
	public void ApuTab_ChannelEntries_HaveAttenuation() {
		var tab = GetTabByName("APU");
		var names = GetEntryNames(tab);

		Assert.Contains("Attenuation", names);
	}

	#endregion

	#region Helpers

	private static List<RegisterViewerTab> GetAllTabs() {
		var state = new LynxState {
			Mikey = new LynxMikeyState {
				Timers = new LynxTimerState[8],
				Apu = new LynxApuState {
					Channels = new LynxAudioChannelState[4],
				},
			},
		};
		return LynxRegisterViewer.GetTabs(ref state);
	}

	private static RegisterViewerTab GetTabByName(string name) {
		var tabs = GetAllTabs();
		var tab = tabs.FirstOrDefault(t => t.TabName == name);
		Assert.NotNull(tab);
		return tab;
	}

	private static List<string> GetEntryNames(RegisterViewerTab tab) {
		var names = new List<string>();
		foreach (var entry in tab.Data) {
			if (!string.IsNullOrEmpty(entry.Name)) {
				names.Add(entry.Name.Trim());
			}
		}
		return names;
	}

	#endregion
}
