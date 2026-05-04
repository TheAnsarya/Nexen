using System.Collections.Generic;
using System.Linq;
using Nexen.Debugger.RegisterViewer;
using Nexen.Debugger.ViewModels;
using Nexen.Interop;
using Xunit;

namespace Nexen.Tests.Debugger.ViewModels;

public class GenesisRegisterViewerTests {
	[Fact]
	public void GetTabs_Returns4Tabs() {
		var tabs = GetAllTabs();
		Assert.Equal(4, tabs.Count);
	}

	[Theory]
	[InlineData(0, "CPU")]
	[InlineData(1, "VDP")]
	[InlineData(2, "PSG")]
	[InlineData(3, "I/O")]
	public void GetTabs_HasExpectedNames(int index, string expectedName) {
		var tabs = GetAllTabs();
		Assert.Equal(expectedName, tabs[index].TabName);
	}

	[Fact]
	public void CpuTab_ContainsCoreRegisters() {
		RegisterViewerTab tab = GetTabByName("CPU");
		List<string> names = GetEntryNames(tab);

		Assert.Contains("PC", names);
		Assert.Contains("SR", names);
		Assert.Contains("D0", names);
		Assert.Contains("A7", names);
		Assert.Contains("CpuDigestHi", names);
		Assert.Contains("CpuDigestLo", names);
		Assert.Contains("CpuFlowParityKey", names);
		Assert.Contains("CpuRegsParityKey", names);
		Assert.Contains("CpuStackParityKey", names);
		Assert.Contains("CpuStatusParityKey", names);
	}

	[Fact]
	public void VdpTab_ContainsStatusFields() {
		RegisterViewerTab tab = GetTabByName("VDP");
		List<string> names = GetEntryNames(tab);

		Assert.Contains("StatusRegister", names);
		Assert.Contains("AddressRegister", names);
		Assert.Contains("DmaActive", names);
		Assert.Contains("Register 0", names);
		Assert.Contains("VdpDigestHi", names);
		Assert.Contains("VdpDigestLo", names);
		Assert.Contains("RasterParityKey", names);
		Assert.Contains("DmaParityKey", names);
		Assert.Contains("VdpRegsParityKey", names);
		Assert.Contains("VdpStatusParityKey", names);
		Assert.Contains("VdpCommandParityKey", names);
		Assert.Contains("VdpFifoParityKey", names);
	}

	private static List<RegisterViewerTab> GetAllTabs() {
		GenesisState state = new() {
			Cpu = new GenesisM68kState {
				D = new uint[8],
				A = new uint[8],
				PC = 0x123456,
				SR = 0x2700,
				CycleCount = 42,
			},
			Vdp = new GenesisVdpState {
				Registers = new byte[24],
				Cram = new ushort[64],
				Vsram = new ushort[40],
				StatusRegister = 0x3400,
				AddressRegister = 0xc00000,
				DmaActive = true,
			},
			Io = new GenesisIoState {
				DataPort = new byte[3],
				CtrlPort = new byte[3],
				TxData = new byte[3],
				RxData = new byte[3],
				SCtrl = new byte[3],
				ThCount = new byte[2],
				ThState = new byte[2],
			},
			Psg = new GenesisPsgState {
				Channels = new GenesisPsgChannelState[4],
				WriteCount = 3,
			}
		};

		return GenesisRegisterViewer.GetTabs(ref state);
	}

	private static RegisterViewerTab GetTabByName(string name) {
		return GetAllTabs().Single(t => t.TabName == name);
	}

	private static List<string> GetEntryNames(RegisterViewerTab tab) {
		return tab.Data.Select(e => e.Name).ToList();
	}
}
