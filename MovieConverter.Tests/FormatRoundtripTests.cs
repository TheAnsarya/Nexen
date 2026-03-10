namespace Nexen.MovieConverter.Tests;

/// <summary>
/// Tests for format roundtrip correctness: Write → Read → Verify for
/// ControllerInput format methods and InputFrame command parsing.
/// </summary>
public class FormatRoundtripTests {
	#region ControllerInput Nexen Roundtrip

	[Fact]
	public void NexenFormat_Roundtrip_AllButtons() {
		var input = new ControllerInput {
			A = true, B = true, X = true, Y = true,
			L = true, R = true, Start = true, Select = true,
			Up = true, Down = true, Left = true, Right = true
		};

		string nexen = input.ToNexenFormat();
		var parsed = ControllerInput.FromNexenFormat(nexen);

		Assert.True(parsed.A);
		Assert.True(parsed.B);
		Assert.True(parsed.X);
		Assert.True(parsed.Y);
		Assert.True(parsed.L);
		Assert.True(parsed.R);
		Assert.True(parsed.Start);
		Assert.True(parsed.Select);
		Assert.True(parsed.Up);
		Assert.True(parsed.Down);
		Assert.True(parsed.Left);
		Assert.True(parsed.Right);
	}

	[Fact]
	public void NexenFormat_Roundtrip_NoButtons() {
		var input = new ControllerInput();

		string nexen = input.ToNexenFormat();
		var parsed = ControllerInput.FromNexenFormat(nexen);

		Assert.Equal("............", nexen);
		Assert.False(parsed.HasInput);
	}

	[Fact]
	public void NexenFormat_Roundtrip_SingleButton_A() {
		var input = new ControllerInput { A = true };

		string nexen = input.ToNexenFormat();
		var parsed = ControllerInput.FromNexenFormat(nexen);

		Assert.True(parsed.A);
		Assert.False(parsed.B);
		Assert.Equal("........A...", nexen);
	}

	[Theory]
	[InlineData(true, false, false, false, false, false, false, false, false, false, false, false)]
	[InlineData(false, true, false, false, false, false, false, false, false, false, false, false)]
	[InlineData(false, false, true, false, false, false, false, false, false, false, false, false)]
	[InlineData(false, false, false, true, false, false, false, false, false, false, false, false)]
	[InlineData(false, false, false, false, true, false, false, false, false, false, false, false)]
	[InlineData(false, false, false, false, false, true, false, false, false, false, false, false)]
	[InlineData(false, false, false, false, false, false, true, false, false, false, false, false)]
	[InlineData(false, false, false, false, false, false, false, true, false, false, false, false)]
	[InlineData(false, false, false, false, false, false, false, false, true, false, false, false)]
	[InlineData(false, false, false, false, false, false, false, false, false, true, false, false)]
	[InlineData(false, false, false, false, false, false, false, false, false, false, true, false)]
	[InlineData(false, false, false, false, false, false, false, false, false, false, false, true)]
	public void NexenFormat_Roundtrip_EachButtonIndividually(
		bool b, bool y, bool sel, bool start, bool up, bool down,
		bool left, bool right, bool a, bool x, bool l, bool r) {
		var input = new ControllerInput {
			B = b, Y = y, Select = sel, Start = start,
			Up = up, Down = down, Left = left, Right = right,
			A = a, X = x, L = l, R = r
		};

		string nexen = input.ToNexenFormat();
		var parsed = ControllerInput.FromNexenFormat(nexen);

		Assert.Equal(b, parsed.B);
		Assert.Equal(y, parsed.Y);
		Assert.Equal(sel, parsed.Select);
		Assert.Equal(start, parsed.Start);
		Assert.Equal(up, parsed.Up);
		Assert.Equal(down, parsed.Down);
		Assert.Equal(left, parsed.Left);
		Assert.Equal(right, parsed.Right);
		Assert.Equal(a, parsed.A);
		Assert.Equal(x, parsed.X);
		Assert.Equal(l, parsed.L);
		Assert.Equal(r, parsed.R);
	}

	[Fact]
	public void NexenFormat_CharLayout_BYsSUDLRAXlr() {
		var input = new ControllerInput {
			B = true, Y = true, Select = true, Start = true,
			Up = true, Down = true, Left = true, Right = true,
			A = true, X = true, L = true, R = true
		};

		string nexen = input.ToNexenFormat();

		Assert.Equal(12, nexen.Length);
		Assert.Equal('B', nexen[0]);
		Assert.Equal('Y', nexen[1]);
		Assert.Equal('s', nexen[2]);  // lowercase
		Assert.Equal('S', nexen[3]);  // uppercase
		Assert.Equal('U', nexen[4]);
		Assert.Equal('D', nexen[5]);
		Assert.Equal('L', nexen[6]);
		Assert.Equal('R', nexen[7]);
		Assert.Equal('A', nexen[8]);
		Assert.Equal('X', nexen[9]);
		Assert.Equal('l', nexen[10]); // lowercase
		Assert.Equal('r', nexen[11]); // lowercase
	}

	[Fact]
	public void NexenFormat_NoneType_ReturnsDots() {
		var input = new ControllerInput { Type = ControllerType.None };

		string nexen = input.ToNexenFormat();

		Assert.Equal("............", nexen);
	}

	#endregion

	#region ControllerInput SMV Roundtrip

	[Fact]
	public void SmvFormat_Roundtrip_AllButtons() {
		var input = new ControllerInput {
			A = true, B = true, X = true, Y = true,
			L = true, R = true, Start = true, Select = true,
			Up = true, Down = true, Left = true, Right = true
		};

		ushort smv = input.ToSmvFormat();
		var parsed = ControllerInput.FromSmvFormat(smv);

		Assert.True(parsed.A);
		Assert.True(parsed.B);
		Assert.True(parsed.X);
		Assert.True(parsed.Y);
		Assert.True(parsed.L);
		Assert.True(parsed.R);
		Assert.True(parsed.Start);
		Assert.True(parsed.Select);
		Assert.True(parsed.Up);
		Assert.True(parsed.Down);
		Assert.True(parsed.Left);
		Assert.True(parsed.Right);
	}

	[Fact]
	public void SmvFormat_Roundtrip_NoButtons() {
		var input = new ControllerInput();

		ushort smv = input.ToSmvFormat();
		var parsed = ControllerInput.FromSmvFormat(smv);

		Assert.Equal((ushort)0, smv);
		Assert.False(parsed.HasInput);
	}

	[Fact]
	public void SmvFormat_Roundtrip_DpadOnly() {
		var input = new ControllerInput { Up = true, Right = true };

		ushort smv = input.ToSmvFormat();
		var parsed = ControllerInput.FromSmvFormat(smv);

		Assert.True(parsed.Up);
		Assert.True(parsed.Right);
		Assert.False(parsed.A);
		Assert.False(parsed.B);
	}

	#endregion

	#region ControllerInput FM2 Roundtrip

	[Fact]
	public void Fm2Format_Roundtrip_AllNesButtons() {
		var input = new ControllerInput {
			A = true, B = true, Start = true, Select = true,
			Up = true, Down = true, Left = true, Right = true
		};

		string fm2 = input.ToFm2Format();
		var parsed = ControllerInput.FromFm2Format(fm2);

		Assert.True(parsed.A);
		Assert.True(parsed.B);
		Assert.True(parsed.Start);
		Assert.True(parsed.Select);
		Assert.True(parsed.Up);
		Assert.True(parsed.Down);
		Assert.True(parsed.Left);
		Assert.True(parsed.Right);
	}

	[Fact]
	public void Fm2Format_Roundtrip_NoButtons() {
		var input = new ControllerInput();

		string fm2 = input.ToFm2Format();
		var parsed = ControllerInput.FromFm2Format(fm2);

		Assert.Equal("........", fm2);
		Assert.False(parsed.HasInput);
	}

	[Fact]
	public void Fm2Format_CharLayout_RLDUSTBA() {
		var input = new ControllerInput {
			A = true, B = true, Start = true, Select = true,
			Up = true, Down = true, Left = true, Right = true
		};

		string fm2 = input.ToFm2Format();

		Assert.Equal(8, fm2.Length);
		Assert.Equal('R', fm2[0]);
		Assert.Equal('L', fm2[1]);
		Assert.Equal('D', fm2[2]);
		Assert.Equal('U', fm2[3]);
		Assert.Equal('T', fm2[4]);  // FM2 uses T for Start
		Assert.Equal('S', fm2[5]);  // FM2 uses S for Select
		Assert.Equal('B', fm2[6]);
		Assert.Equal('A', fm2[7]);
	}

	#endregion

	#region ControllerInput LSMV Roundtrip

	[Fact]
	public void LsmvFormat_Roundtrip_AllButtons() {
		var input = new ControllerInput {
			A = true, B = true, X = true, Y = true,
			L = true, R = true, Start = true, Select = true,
			Up = true, Down = true, Left = true, Right = true
		};

		string lsmv = input.ToLsmvFormat();
		var parsed = ControllerInput.FromLsmvFormat(lsmv);

		Assert.True(parsed.A);
		Assert.True(parsed.B);
		Assert.True(parsed.X);
		Assert.True(parsed.Y);
		Assert.True(parsed.L);
		Assert.True(parsed.R);
		Assert.True(parsed.Start);
		Assert.True(parsed.Select);
		Assert.True(parsed.Up);
		Assert.True(parsed.Down);
		Assert.True(parsed.Left);
		Assert.True(parsed.Right);
	}

	[Fact]
	public void LsmvFormat_Roundtrip_NoButtons() {
		var input = new ControllerInput();

		string lsmv = input.ToLsmvFormat();
		var parsed = ControllerInput.FromLsmvFormat(lsmv);

		Assert.Equal("............", lsmv);
		Assert.False(parsed.HasInput);
	}

	[Fact]
	public void LsmvFormat_CharLayout_BYsSudlrAXLR() {
		var input = new ControllerInput {
			B = true, Y = true, Select = true, Start = true,
			Up = true, Down = true, Left = true, Right = true,
			A = true, X = true, L = true, R = true
		};

		string lsmv = input.ToLsmvFormat();

		Assert.Equal(12, lsmv.Length);
		Assert.Equal('B', lsmv[0]);
		Assert.Equal('Y', lsmv[1]);
		Assert.Equal('s', lsmv[2]);  // lowercase
		Assert.Equal('S', lsmv[3]);  // uppercase
		Assert.Equal('u', lsmv[4]);  // lowercase (LSMV difference)
		Assert.Equal('d', lsmv[5]);  // lowercase
		Assert.Equal('l', lsmv[6]);  // lowercase
		Assert.Equal('r', lsmv[7]);  // lowercase
		Assert.Equal('A', lsmv[8]);
		Assert.Equal('X', lsmv[9]);
		Assert.Equal('L', lsmv[10]); // uppercase (LSMV difference)
		Assert.Equal('R', lsmv[11]); // uppercase
	}

	#endregion

	#region ControllerInput BK2 Roundtrip — NES

	[Fact]
	public void Bk2Format_Nes_Roundtrip_AllButtons() {
		var input = new ControllerInput {
			A = true, B = true, Start = true, Select = true,
			Up = true, Down = true, Left = true, Right = true
		};

		string bk2 = input.ToBk2Format(SystemType.Nes);
		var parsed = ControllerInput.FromBk2Format(bk2, SystemType.Nes);

		Assert.True(parsed.A);
		Assert.True(parsed.B);
		Assert.True(parsed.Start);
		Assert.True(parsed.Select);
		Assert.True(parsed.Up);
		Assert.True(parsed.Down);
		Assert.True(parsed.Left);
		Assert.True(parsed.Right);
	}

	[Fact]
	public void Bk2Format_Nes_Roundtrip_NoButtons() {
		var input = new ControllerInput();

		string bk2 = input.ToBk2Format(SystemType.Nes);
		var parsed = ControllerInput.FromBk2Format(bk2, SystemType.Nes);

		Assert.Equal("........", bk2);
		Assert.False(parsed.HasInput);
	}

	[Fact]
	public void Bk2Format_Nes_CharLayout_UDLRsSBA() {
		var input = new ControllerInput {
			Up = true, Down = true, Left = true, Right = true,
			Select = true, Start = true, B = true, A = true
		};

		string bk2 = input.ToBk2Format(SystemType.Nes);

		Assert.Equal(8, bk2.Length);
		Assert.Equal('U', bk2[0]);
		Assert.Equal('D', bk2[1]);
		Assert.Equal('L', bk2[2]);
		Assert.Equal('R', bk2[3]);
		Assert.Equal('s', bk2[4]); // lowercase select
		Assert.Equal('S', bk2[5]); // uppercase start
		Assert.Equal('B', bk2[6]);
		Assert.Equal('A', bk2[7]);
	}

	#endregion

	#region ControllerInput BK2 Roundtrip — SNES

	[Fact]
	public void Bk2Format_Snes_Roundtrip_AllButtons() {
		var input = new ControllerInput {
			A = true, B = true, X = true, Y = true,
			L = true, R = true, Start = true, Select = true,
			Up = true, Down = true, Left = true, Right = true
		};

		string bk2 = input.ToBk2Format(SystemType.Snes);
		var parsed = ControllerInput.FromBk2Format(bk2, SystemType.Snes);

		Assert.True(parsed.A);
		Assert.True(parsed.B);
		Assert.True(parsed.X);
		Assert.True(parsed.Y);
		Assert.True(parsed.L);
		Assert.True(parsed.R);
		Assert.True(parsed.Start);
		Assert.True(parsed.Select);
		Assert.True(parsed.Up);
		Assert.True(parsed.Down);
		Assert.True(parsed.Left);
		Assert.True(parsed.Right);
	}

	[Fact]
	public void Bk2Format_Snes_CharLayout_UDLRsSlrBAXY() {
		var input = new ControllerInput {
			Up = true, Down = true, Left = true, Right = true,
			Select = true, Start = true, L = true, R = true,
			B = true, A = true, X = true, Y = true
		};

		string bk2 = input.ToBk2Format(SystemType.Snes);

		Assert.Equal(12, bk2.Length);
		Assert.Equal('U', bk2[0]);
		Assert.Equal('D', bk2[1]);
		Assert.Equal('L', bk2[2]);
		Assert.Equal('R', bk2[3]);
		Assert.Equal('s', bk2[4]);  // lowercase select
		Assert.Equal('S', bk2[5]);  // uppercase start
		Assert.Equal('l', bk2[6]);  // lowercase L shoulder
		Assert.Equal('r', bk2[7]);  // lowercase R shoulder
		Assert.Equal('B', bk2[8]);
		Assert.Equal('A', bk2[9]);
		Assert.Equal('X', bk2[10]);
		Assert.Equal('Y', bk2[11]);
	}

	#endregion

	#region ControllerInput BK2 Roundtrip — Genesis

	[Fact]
	public void Bk2Format_Genesis_Roundtrip_AllButtons() {
		var input = new ControllerInput {
			A = true, B = true, C = true, Start = true,
			Up = true, Down = true, Left = true, Right = true
		};

		string bk2 = input.ToBk2Format(SystemType.Genesis);
		var parsed = ControllerInput.FromBk2Format(bk2, SystemType.Genesis);

		Assert.True(parsed.A);
		Assert.True(parsed.B);
		Assert.True(parsed.C);
		Assert.True(parsed.Start);
		Assert.True(parsed.Up);
		Assert.True(parsed.Down);
		Assert.True(parsed.Left);
		Assert.True(parsed.Right);
	}

	[Fact]
	public void Bk2Format_Genesis_CharLayout_UDLRSABC() {
		var input = new ControllerInput {
			Up = true, Down = true, Left = true, Right = true,
			Start = true, A = true, B = true, C = true
		};

		string bk2 = input.ToBk2Format(SystemType.Genesis);

		Assert.Equal(8, bk2.Length);
		Assert.Equal('U', bk2[0]);
		Assert.Equal('D', bk2[1]);
		Assert.Equal('L', bk2[2]);
		Assert.Equal('R', bk2[3]);
		Assert.Equal('S', bk2[4]);
		Assert.Equal('A', bk2[5]);
		Assert.Equal('B', bk2[6]);
		Assert.Equal('C', bk2[7]);
	}

	#endregion

	#region ControllerInput Cross-Format Roundtrip

	[Fact]
	public void CrossFormat_Nexen_To_Fm2_Roundtrip() {
		var input = new ControllerInput {
			A = true, B = true, Up = true, Start = true
		};

		// Nexen → FM2 → Nexen (NES 8-button subset)
		string nexen = input.ToNexenFormat();
		var fromNexen = ControllerInput.FromNexenFormat(nexen);
		string fm2 = fromNexen.ToFm2Format();
		var fromFm2 = ControllerInput.FromFm2Format(fm2);

		Assert.True(fromFm2.A);
		Assert.True(fromFm2.B);
		Assert.True(fromFm2.Up);
		Assert.True(fromFm2.Start);
	}

	[Fact]
	public void CrossFormat_Nexen_To_Smv_Roundtrip() {
		var input = new ControllerInput {
			A = true, B = true, X = true, Y = true,
			L = true, R = true, Start = true, Select = true,
			Up = true, Down = true, Left = true, Right = true
		};

		// Nexen → SMV → Nexen
		string nexen = input.ToNexenFormat();
		var fromNexen = ControllerInput.FromNexenFormat(nexen);
		ushort smv = fromNexen.ToSmvFormat();
		var fromSmv = ControllerInput.FromSmvFormat(smv);

		Assert.True(fromSmv.A);
		Assert.True(fromSmv.B);
		Assert.True(fromSmv.X);
		Assert.True(fromSmv.Y);
		Assert.True(fromSmv.L);
		Assert.True(fromSmv.R);
		Assert.True(fromSmv.Start);
		Assert.True(fromSmv.Select);
		Assert.True(fromSmv.Up);
		Assert.True(fromSmv.Down);
		Assert.True(fromSmv.Left);
		Assert.True(fromSmv.Right);
	}

	[Fact]
	public void CrossFormat_Fm2_To_Bk2Nes_Roundtrip() {
		var input = new ControllerInput {
			A = true, B = true, Up = true, Right = true, Select = true
		};

		string fm2 = input.ToFm2Format();
		var fromFm2 = ControllerInput.FromFm2Format(fm2);
		string bk2 = fromFm2.ToBk2Format(SystemType.Nes);
		var fromBk2 = ControllerInput.FromBk2Format(bk2, SystemType.Nes);

		Assert.True(fromBk2.A);
		Assert.True(fromBk2.B);
		Assert.True(fromBk2.Up);
		Assert.True(fromBk2.Right);
		Assert.True(fromBk2.Select);
		Assert.False(fromBk2.Start);
	}

	[Fact]
	public void CrossFormat_Lsmv_To_Nexen_Roundtrip() {
		var input = new ControllerInput {
			A = true, Y = true, Down = true, L = true
		};

		string lsmv = input.ToLsmvFormat();
		var fromLsmv = ControllerInput.FromLsmvFormat(lsmv);
		string nexen = fromLsmv.ToNexenFormat();
		var fromNexen = ControllerInput.FromNexenFormat(nexen);

		Assert.True(fromNexen.A);
		Assert.True(fromNexen.Y);
		Assert.True(fromNexen.Down);
		Assert.True(fromNexen.L);
		Assert.False(fromNexen.B);
	}

	#endregion

	#region InputFrame ToNexenLogLine/FromNexenLogLine Roundtrip

	[Fact]
	public void NexenLogLine_Roundtrip_BasicInput() {
		var frame = new InputFrame(42);
		frame.Controllers[0].A = true;
		frame.Controllers[0].Up = true;
		frame.Controllers[1].B = true;

		string line = frame.ToNexenLogLine(2);
		var parsed = InputFrame.FromNexenLogLine(line, 42);

		Assert.Equal(42, parsed.FrameNumber);
		Assert.True(parsed.Controllers[0].A);
		Assert.True(parsed.Controllers[0].Up);
		Assert.True(parsed.Controllers[1].B);
		Assert.False(parsed.Controllers[0].B);
		Assert.False(parsed.Controllers[1].A);
	}

	[Fact]
	public void NexenLogLine_Roundtrip_WithComment() {
		var frame = new InputFrame(10) { Comment = "Important frame" };
		frame.Controllers[0].Start = true;

		string line = frame.ToNexenLogLine(1);
		var parsed = InputFrame.FromNexenLogLine(line, 10);

		Assert.Equal("Important frame", parsed.Comment);
		Assert.True(parsed.Controllers[0].Start);
	}

	[Fact]
	public void NexenLogLine_Roundtrip_LagFrame() {
		var frame = new InputFrame(5) { IsLagFrame = true };

		string line = frame.ToNexenLogLine(1);
		var parsed = InputFrame.FromNexenLogLine(line, 5);

		Assert.True(parsed.IsLagFrame);
	}

	[Fact]
	public void NexenLogLine_Roundtrip_LagFrameWithComment() {
		var frame = new InputFrame(7) {
			IsLagFrame = true,
			Comment = "Lag in cutscene"
		};

		string line = frame.ToNexenLogLine(1);
		var parsed = InputFrame.FromNexenLogLine(line, 7);

		Assert.True(parsed.IsLagFrame);
		Assert.Equal("Lag in cutscene", parsed.Comment);
	}

	[Fact]
	public void NexenLogLine_Roundtrip_EmptyFrame() {
		var frame = new InputFrame(0);

		string line = frame.ToNexenLogLine(2);
		var parsed = InputFrame.FromNexenLogLine(line, 0);

		Assert.False(parsed.HasAnyInput);
		Assert.False(parsed.IsLagFrame);
		Assert.Null(parsed.Comment);
	}

	[Fact]
	public void NexenLogLine_Roundtrip_FourPorts() {
		var frame = new InputFrame(99);
		frame.Controllers[0].A = true;
		frame.Controllers[1].B = true;
		frame.Controllers[2].X = true;
		frame.Controllers[3].Y = true;

		string line = frame.ToNexenLogLine(4);
		var parsed = InputFrame.FromNexenLogLine(line, 99);

		Assert.True(parsed.Controllers[0].A);
		Assert.True(parsed.Controllers[1].B);
		Assert.True(parsed.Controllers[2].X);
		Assert.True(parsed.Controllers[3].Y);
	}

	[Fact]
	public void NexenLogLine_EmptyLineParsesToEmptyFrame() {
		var parsed = InputFrame.FromNexenLogLine("", 0);

		Assert.False(parsed.HasAnyInput);
		Assert.Equal(FrameCommand.None, parsed.Command);
	}

	[Fact]
	public void NexenLogLine_WhitespaceLineParsesToEmptyFrame() {
		var parsed = InputFrame.FromNexenLogLine("   ", 0);

		Assert.False(parsed.HasAnyInput);
	}

	#endregion

	#region InputFrame Command Prefix Generation

	[Fact]
	public void GetCommandPrefix_SoftReset() {
		var frame = new InputFrame { Command = FrameCommand.SoftReset };

		string line = frame.ToNexenLogLine(1);

		Assert.StartsWith("CMD:SOFT_RESET|", line);
	}

	[Fact]
	public void GetCommandPrefix_HardReset() {
		var frame = new InputFrame { Command = FrameCommand.HardReset };

		string line = frame.ToNexenLogLine(1);

		Assert.StartsWith("CMD:HARD_RESET|", line);
	}

	[Fact]
	public void GetCommandPrefix_FdsInsert_WithDiskAndSide() {
		var frame = new InputFrame {
			Command = FrameCommand.FdsInsert,
			FdsDiskNumber = 1,
			FdsDiskSide = 1
		};

		string line = frame.ToNexenLogLine(1);

		Assert.StartsWith("CMD:FDS_INSERT:1:1|", line);
	}

	[Fact]
	public void GetCommandPrefix_FdsSelect_WithDiskAndSide() {
		var frame = new InputFrame {
			Command = FrameCommand.FdsSelect,
			FdsDiskNumber = 2,
			FdsDiskSide = 0
		};

		string line = frame.ToNexenLogLine(1);

		Assert.StartsWith("CMD:FDS_SELECT:2:0|", line);
	}

	[Fact]
	public void GetCommandPrefix_FdsInsert_DefaultDiskSide() {
		var frame = new InputFrame { Command = FrameCommand.FdsInsert };

		string line = frame.ToNexenLogLine(1);

		// When FdsDiskNumber/FdsDiskSide are null, defaults to 0:0
		Assert.StartsWith("CMD:FDS_INSERT:0:0|", line);
	}

	[Fact]
	public void GetCommandPrefix_VsCoin() {
		var frame = new InputFrame { Command = FrameCommand.VsInsertCoin };

		string line = frame.ToNexenLogLine(1);

		Assert.StartsWith("CMD:VS_COIN|", line);
	}

	[Fact]
	public void GetCommandPrefix_VsService() {
		var frame = new InputFrame { Command = FrameCommand.VsServiceButton };

		string line = frame.ToNexenLogLine(1);

		Assert.StartsWith("CMD:VS_SERVICE|", line);
	}

	[Fact]
	public void GetCommandPrefix_ControllerSwap() {
		var frame = new InputFrame { Command = FrameCommand.ControllerSwap };

		string line = frame.ToNexenLogLine(1);

		Assert.StartsWith("CMD:CTRL_SWAP|", line);
	}

	[Fact]
	public void GetCommandPrefix_Pause() {
		var frame = new InputFrame { Command = FrameCommand.Pause };

		string line = frame.ToNexenLogLine(1);

		Assert.StartsWith("CMD:PAUSE|", line);
	}

	[Fact]
	public void GetCommandPrefix_PowerOff() {
		var frame = new InputFrame { Command = FrameCommand.PowerOff };

		string line = frame.ToNexenLogLine(1);

		Assert.StartsWith("CMD:POWER_OFF|", line);
	}

	[Fact]
	public void GetCommandPrefix_CombinedFlags() {
		var frame = new InputFrame {
			Command = FrameCommand.SoftReset | FrameCommand.VsInsertCoin
		};

		string line = frame.ToNexenLogLine(1);

		Assert.Contains("SOFT_RESET", line);
		Assert.Contains("VS_COIN", line);
		Assert.StartsWith("CMD:", line);
	}

	[Fact]
	public void GetCommandPrefix_SoftAndHardReset() {
		var frame = new InputFrame {
			Command = FrameCommand.SoftReset | FrameCommand.HardReset
		};

		string line = frame.ToNexenLogLine(1);

		Assert.StartsWith("CMD:SOFT_RESET,HARD_RESET|", line);
	}

	[Fact]
	public void GetCommandPrefix_FdsInsertWithOtherCommands() {
		var frame = new InputFrame {
			Command = FrameCommand.SoftReset | FrameCommand.FdsInsert,
			FdsDiskNumber = 0,
			FdsDiskSide = 1
		};

		string line = frame.ToNexenLogLine(1);

		Assert.Contains("SOFT_RESET", line);
		Assert.Contains("FDS_INSERT:0:1", line);
	}

	[Fact]
	public void GetCommandPrefix_NoCommand_NoPrefix() {
		var frame = new InputFrame();
		frame.Controllers[0].A = true;

		string line = frame.ToNexenLogLine(1);

		Assert.DoesNotContain("CMD:", line);
	}

	#endregion

	#region InputFrame Command Parsing Roundtrip

	[Theory]
	[InlineData(FrameCommand.SoftReset)]
	[InlineData(FrameCommand.HardReset)]
	[InlineData(FrameCommand.VsInsertCoin)]
	[InlineData(FrameCommand.VsServiceButton)]
	[InlineData(FrameCommand.ControllerSwap)]
	[InlineData(FrameCommand.Pause)]
	[InlineData(FrameCommand.PowerOff)]
	public void CommandRoundtrip_SingleCommand(FrameCommand cmd) {
		var frame = new InputFrame { Command = cmd };

		string line = frame.ToNexenLogLine(1);
		var parsed = InputFrame.FromNexenLogLine(line, 0);

		Assert.Equal(cmd, parsed.Command);
	}

	[Fact]
	public void CommandRoundtrip_FdsInsert_PreservesDiskInfo() {
		var frame = new InputFrame {
			Command = FrameCommand.FdsInsert,
			FdsDiskNumber = 2,
			FdsDiskSide = 1
		};

		string line = frame.ToNexenLogLine(1);
		var parsed = InputFrame.FromNexenLogLine(line, 0);

		Assert.True(parsed.Command.HasFlag(FrameCommand.FdsInsert));
	}

	[Fact]
	public void CommandRoundtrip_FdsSelect_PreservesDiskInfo() {
		var frame = new InputFrame {
			Command = FrameCommand.FdsSelect,
			FdsDiskNumber = 1,
			FdsDiskSide = 0
		};

		string line = frame.ToNexenLogLine(1);
		var parsed = InputFrame.FromNexenLogLine(line, 0);

		Assert.True(parsed.Command.HasFlag(FrameCommand.FdsSelect));
	}

	[Fact]
	public void CommandRoundtrip_MultipleFlags() {
		var cmd = FrameCommand.SoftReset | FrameCommand.VsInsertCoin | FrameCommand.Pause;
		var frame = new InputFrame { Command = cmd };

		string line = frame.ToNexenLogLine(1);
		var parsed = InputFrame.FromNexenLogLine(line, 0);

		Assert.True(parsed.Command.HasFlag(FrameCommand.SoftReset));
		Assert.True(parsed.Command.HasFlag(FrameCommand.VsInsertCoin));
		Assert.True(parsed.Command.HasFlag(FrameCommand.Pause));
	}

	[Fact]
	public void CommandRoundtrip_CommandWithInput() {
		var frame = new InputFrame {
			Command = FrameCommand.SoftReset
		};
		frame.Controllers[0].A = true;
		frame.Controllers[0].Up = true;

		string line = frame.ToNexenLogLine(1);
		var parsed = InputFrame.FromNexenLogLine(line, 0);

		Assert.Equal(FrameCommand.SoftReset, parsed.Command);
		Assert.True(parsed.Controllers[0].A);
		Assert.True(parsed.Controllers[0].Up);
	}

	[Fact]
	public void CommandRoundtrip_CommandWithLagAndComment() {
		var frame = new InputFrame {
			Command = FrameCommand.HardReset,
			IsLagFrame = true,
			Comment = "Reset here"
		};

		string line = frame.ToNexenLogLine(1);
		var parsed = InputFrame.FromNexenLogLine(line, 0);

		Assert.Equal(FrameCommand.HardReset, parsed.Command);
		Assert.True(parsed.IsLagFrame);
		Assert.Equal("Reset here", parsed.Comment);
	}

	[Fact]
	public void CommandParsing_CaseInsensitive_FdsInsert() {
		// Manually construct a line with lowercase command
		string line = "CMD:fds_insert:0:0|............";
		var parsed = InputFrame.FromNexenLogLine(line, 0);

		Assert.True(parsed.Command.HasFlag(FrameCommand.FdsInsert));
	}

	[Fact]
	public void CommandParsing_CaseInsensitive_FdsSelect() {
		string line = "CMD:Fds_Select:1:0|............";
		var parsed = InputFrame.FromNexenLogLine(line, 0);

		Assert.True(parsed.Command.HasFlag(FrameCommand.FdsSelect));
	}

	[Fact]
	public void CommandParsing_CaseInsensitive_SimpleCommands() {
		string line = "CMD:soft_reset|............";
		var parsed = InputFrame.FromNexenLogLine(line, 0);

		Assert.True(parsed.Command.HasFlag(FrameCommand.SoftReset));
	}

	[Fact]
	public void CommandParsing_InvalidCommand_Ignored() {
		string line = "CMD:INVALID_COMMAND|............";
		var parsed = InputFrame.FromNexenLogLine(line, 0);

		Assert.Equal(FrameCommand.None, parsed.Command);
	}

	[Fact]
	public void CommandParsing_MixedValidInvalid() {
		string line = "CMD:SOFT_RESET,GARBAGE,VS_COIN|............";
		var parsed = InputFrame.FromNexenLogLine(line, 0);

		Assert.True(parsed.Command.HasFlag(FrameCommand.SoftReset));
		Assert.True(parsed.Command.HasFlag(FrameCommand.VsInsertCoin));
	}

	[Fact]
	public void CommandParsing_WhitespaceInParts() {
		string line = "CMD: SOFT_RESET , VS_COIN |............";
		var parsed = InputFrame.FromNexenLogLine(line, 0);

		Assert.True(parsed.Command.HasFlag(FrameCommand.SoftReset));
		Assert.True(parsed.Command.HasFlag(FrameCommand.VsInsertCoin));
	}

	#endregion

	#region InputFrame Full Roundtrip — Complex Scenarios

	[Fact]
	public void FullRoundtrip_CommandInputLagComment() {
		var frame = new InputFrame(100) {
			Command = FrameCommand.SoftReset,
			IsLagFrame = true,
			Comment = "Frame 100 reset"
		};
		frame.Controllers[0].A = true;
		frame.Controllers[0].B = true;
		frame.Controllers[1].Up = true;

		string line = frame.ToNexenLogLine(2);
		var parsed = InputFrame.FromNexenLogLine(line, 100);

		Assert.Equal(100, parsed.FrameNumber);
		Assert.Equal(FrameCommand.SoftReset, parsed.Command);
		Assert.True(parsed.IsLagFrame);
		Assert.Equal("Frame 100 reset", parsed.Comment);
		Assert.True(parsed.Controllers[0].A);
		Assert.True(parsed.Controllers[0].B);
		Assert.True(parsed.Controllers[1].Up);
	}

	[Fact]
	public void FullRoundtrip_MultiplePortsWithAllButtons() {
		var frame = new InputFrame(0);
		frame.Controllers[0] = new ControllerInput {
			A = true, B = true, X = true, Y = true,
			L = true, R = true, Start = true, Select = true,
			Up = true, Down = true, Left = true, Right = true
		};
		frame.Controllers[1] = new ControllerInput {
			A = true, Start = true, Up = true, Right = true
		};

		string line = frame.ToNexenLogLine(2);
		var parsed = InputFrame.FromNexenLogLine(line, 0);

		// P1 all buttons
		Assert.True(parsed.Controllers[0].A);
		Assert.True(parsed.Controllers[0].B);
		Assert.True(parsed.Controllers[0].X);
		Assert.True(parsed.Controllers[0].Y);
		Assert.True(parsed.Controllers[0].L);
		Assert.True(parsed.Controllers[0].R);
		Assert.True(parsed.Controllers[0].Start);
		Assert.True(parsed.Controllers[0].Select);
		Assert.True(parsed.Controllers[0].Up);
		Assert.True(parsed.Controllers[0].Down);
		Assert.True(parsed.Controllers[0].Left);
		Assert.True(parsed.Controllers[0].Right);

		// P2 partial
		Assert.True(parsed.Controllers[1].A);
		Assert.True(parsed.Controllers[1].Start);
		Assert.True(parsed.Controllers[1].Up);
		Assert.True(parsed.Controllers[1].Right);
		Assert.False(parsed.Controllers[1].B);
	}

	[Fact]
	public void FullRoundtrip_FdsCommandWithMultiplePorts() {
		var frame = new InputFrame(50) {
			Command = FrameCommand.FdsInsert,
			FdsDiskNumber = 1,
			FdsDiskSide = 0
		};
		frame.Controllers[0].A = true;

		string line = frame.ToNexenLogLine(2);
		var parsed = InputFrame.FromNexenLogLine(line, 50);

		Assert.True(parsed.Command.HasFlag(FrameCommand.FdsInsert));
		Assert.True(parsed.Controllers[0].A);
	}

	#endregion

	#region ControllerInput ButtonBits Setter

	[Fact]
	public void ButtonBits_Setter_SetsAllButtons() {
		var input = new ControllerInput();

		input.ButtonBits = 0x0fff; // All 12 buttons

		Assert.True(input.A);
		Assert.True(input.B);
		Assert.True(input.X);
		Assert.True(input.Y);
		Assert.True(input.L);
		Assert.True(input.R);
		Assert.True(input.Start);
		Assert.True(input.Select);
		Assert.True(input.Up);
		Assert.True(input.Down);
		Assert.True(input.Left);
		Assert.True(input.Right);
	}

	[Fact]
	public void ButtonBits_Setter_ClearsAllButtons() {
		var input = new ControllerInput {
			A = true, B = true, Up = true
		};

		input.ButtonBits = 0;

		Assert.False(input.A);
		Assert.False(input.B);
		Assert.False(input.Up);
	}

	[Fact]
	public void ButtonBits_Roundtrip() {
		var input = new ControllerInput {
			A = true, B = true, Start = true, Up = true, Right = true
		};

		ushort bits = input.ButtonBits;
		var copy = new ControllerInput();
		copy.ButtonBits = bits;

		Assert.Equal(input.A, copy.A);
		Assert.Equal(input.B, copy.B);
		Assert.Equal(input.Start, copy.Start);
		Assert.Equal(input.Up, copy.Up);
		Assert.Equal(input.Right, copy.Right);
		Assert.Equal(input.X, copy.X);
		Assert.Equal(input.Y, copy.Y);
	}

	#endregion

	#region Edge Cases

	[Fact]
	public void NexenFormat_FromShortInput_ReturnsDefault() {
		// Less than 12 chars → should return default (no buttons)
		var parsed = ControllerInput.FromNexenFormat("ABCD");

		Assert.False(parsed.HasInput);
	}

	[Fact]
	public void Fm2Format_FromShortInput_ReturnsDefault() {
		var parsed = ControllerInput.FromFm2Format("ABC");

		Assert.False(parsed.HasInput);
	}

	[Fact]
	public void LsmvFormat_FromShortInput_ReturnsDefault() {
		var parsed = ControllerInput.FromLsmvFormat("SHORT");

		Assert.False(parsed.HasInput);
	}

	[Fact]
	public void Bk2Format_FromShortInput_ReturnsDefault() {
		var parsed = ControllerInput.FromBk2Format("AB", SystemType.Nes);

		Assert.False(parsed.HasInput);
	}

	[Fact]
	public void NexenLogLine_ControllerInputTooShort_Skipped() {
		// Port input shorter than 8 chars should be skipped
		string line = "ABC|............";
		var parsed = InputFrame.FromNexenLogLine(line, 0);

		// "ABC" is < 8 chars, so portIndex stays at 0 and "............" becomes port 0
		Assert.False(parsed.Controllers[0].HasInput);
	}

	[Fact]
	public void NexenLogLine_CommentWithSpecialChars() {
		var frame = new InputFrame(0) { Comment = "Test: reset @100" };

		string line = frame.ToNexenLogLine(1);
		var parsed = InputFrame.FromNexenLogLine(line, 0);

		Assert.Equal("Test: reset @100", parsed.Comment);
	}

	#endregion
}
