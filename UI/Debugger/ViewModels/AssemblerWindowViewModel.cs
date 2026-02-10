using System;
using System.Collections.Generic;
using System.Linq;
using System.Reactive.Linq;
using System.Text;
using System.Threading.Tasks;
using Avalonia.Controls;
using Nexen.Config;
using Nexen.Debugger.Utilities;
using Nexen.Debugger.Windows;
using Nexen.Interop;
using Nexen.Localization;
using Nexen.Utilities;
using Nexen.ViewModels;
using Nexen.Windows;
using ReactiveUI;
using ReactiveUI.Fody.Helpers;

namespace Nexen.Debugger.ViewModels; 
/// <summary>
/// ViewModel for the inline assembler window, providing assembly code editing and compilation.
/// </summary>
/// <remarks>
/// <para>
/// This ViewModel provides:
/// <list type="bullet">
///   <item><description>Real-time assembly compilation with error feedback</description></item>
///   <item><description>Byte code preview as the user types</description></item>
///   <item><description>Validation of code size against available space</description></item>
///   <item><description>NOP padding when new code is smaller than original</description></item>
/// </list>
/// </para>
/// <para>
/// Supports editing existing code (with size warnings) or assembling new code at any address.
/// </para>
/// </remarks>
public sealed class AssemblerWindowViewModel : DisposableViewModel {
	/// <summary>Gets the assembler configuration settings.</summary>
	public AssemblerConfig Config { get; }

	/// <summary>Gets or sets the assembly source code text.</summary>
	[Reactive] public string Code { get; set; } = "";

	/// <summary>Gets or sets the compiled byte code preview display.</summary>
	[Reactive] public string ByteCodeView { get; set; } = "";

	/// <summary>Gets or sets the target starting address for the assembled code.</summary>
	[Reactive] public int StartAddress { get; set; }

	/// <summary>Gets or sets the number of bytes produced by the current assembly.</summary>
	[Reactive] public int BytesUsed { get; set; }

	/// <summary>Gets or sets whether there are warnings to display.</summary>
	[Reactive] public bool HasWarning { get; set; }

	/// <summary>Gets or sets whether the assembled code is identical to the original.</summary>
	[Reactive] public bool IsIdentical { get; set; }

	/// <summary>Gets or sets whether the new code exceeds the original code size.</summary>
	[Reactive] public bool OriginalSizeExceeded { get; set; }

	/// <summary>Gets or sets whether the new code exceeds available memory space.</summary>
	[Reactive] public bool MaxSizeExceeded { get; set; }

	/// <summary>Gets or sets whether the OK/Apply button should be enabled.</summary>
	[Reactive] public bool OkEnabled { get; set; } = false;

	/// <summary>Gets or sets the list of assembly errors from compilation.</summary>
	[Reactive] public List<AssemblerError> Errors { get; set; } = [];

	/// <summary>Gets or sets the File menu actions.</summary>
	[Reactive] public List<ContextMenuAction> FileMenuActions { get; private set; } = new();

	/// <summary>Gets or sets the Options menu actions.</summary>
	[Reactive] public List<ContextMenuAction> OptionsMenuActions { get; private set; } = new();

	/// <summary>Gets the CPU type being assembled for.</summary>
	public CpuType CpuType { get; }

	private List<byte> _bytes = new();

	/// <summary>Gets the maximum valid address for this memory type, or null if unavailable.</summary>
	public int? MaxAddress { get; }

	private int _originalAddress = -1;
	private byte[] _originalCode = [];

	/// <summary>Gets or sets the original byte count when editing existing code.</summary>
	[Reactive] public int OriginalByteCount { get; private set; } = 0;

	/// <summary>
	/// Designer-only constructor. Do not use in code.
	/// </summary>
	[Obsolete("For designer only")]
	public AssemblerWindowViewModel() : this(CpuType.Snes) { }

	/// <summary>
	/// Initializes a new instance of the <see cref="AssemblerWindowViewModel"/> class.
	/// </summary>
	/// <param name="cpuType">The CPU type to assemble code for.</param>
	/// <remarks>
	/// Sets up reactive subscriptions to recompile code whenever the source or start address changes.
	/// </remarks>
	public AssemblerWindowViewModel(CpuType cpuType) {
		Config = ConfigManager.Config.Debug.Assembler;
		CpuType = cpuType;

		if (Design.IsDesignMode) {
			return;
		}

		MaxAddress = DebugApi.GetMemorySize(CpuType.ToMemoryType()) - 1;

		// Recompile whenever code or start address changes
		AddDisposable(this.WhenAnyValue(x => x.Code, x => x.StartAddress).Subscribe(_ => UpdateAssembly(Code)));
	}

	/// <summary>
	/// Initializes the File and Options menu items.
	/// </summary>
	/// <param name="wnd">The parent window for menu actions.</param>
	public void InitMenu(Window wnd) {
		FileMenuActions = AddDisposables(new List<ContextMenuAction>() {
			SaveRomActionHelper.GetSaveRomAction(wnd),
			SaveRomActionHelper.GetSaveRomAsAction(wnd),
			SaveRomActionHelper.GetSaveEditsAsIpsAction(wnd),
			new ContextMenuSeparator(),
			new ContextMenuAction() {
				ActionType = ActionType.Exit,
				OnClick = () => wnd.Close()
			}
		});

		OptionsMenuActions = AddDisposables(new List<ContextMenuAction>() {
			new ContextMenuAction() {
				ActionType = ActionType.OpenDebugSettings,
				Shortcut = () => ConfigManager.Config.Debug.Shortcuts.Get(DebuggerShortcut.OpenDebugSettings),
				OnClick = () => DebuggerConfigWindow.Open(DebugConfigWindowTab.FontAndColors, wnd)
			}
		});
	}

	/// <summary>
	/// Initializes the assembler for editing existing code at a specific address.
	/// </summary>
	/// <param name="address">The starting address of the code to edit.</param>
	/// <param name="code">The disassembled source code text.</param>
	/// <param name="byteCount">The number of bytes of original code.</param>
	/// <remarks>
	/// Saves the original code for comparison and NOP padding when the new code is smaller.
	/// </remarks>
	public void InitEditCode(int address, string code, int byteCount) {
		OriginalByteCount = byteCount;
		_originalAddress = address;

		using var delayNotifs = DelayChangeNotifications();
		StartAddress = address;
		Code = code;

		if (OriginalByteCount > 0) {
			_originalCode = DebugApi.GetMemoryValues(CpuType.ToMemoryType(), (uint)StartAddress, (uint)(StartAddress + OriginalByteCount - 1));
		}
	}

	/// <summary>
	/// Assembles the source code and updates the byte code preview and validation state.
	/// </summary>
	/// <param name="code">The assembly source code to compile.</param>
	/// <remarks>
	/// Parses each line, collects byte output and errors, and updates all reactive properties.
	/// </remarks>
	private void UpdateAssembly(string code) {
		string[] codeLines = code.Replace("\r", "").Split('\n').Select(x => x.Trim()).ToArray();
		short[] byteCode = DebugApi.AssembleCode(CpuType, string.Join('\n', codeLines), (uint)StartAddress);
		List<AssemblerError> errorList = [];
		List<byte> convertedByteCode = new List<byte>(byteCode.Length);
		StringBuilder sb = new StringBuilder();
		int line = 1;
		for (int i = 0; i < byteCode.Length; i++) {
			short s = byteCode[i];
			if (s >= 0) {
				convertedByteCode.Add((byte)s);
				sb.Append(s.ToString("X2") + " ");
			} else if (s == (int)AssemblerSpecialCodes.EndOfLine) {
				line++;
				if (line <= codeLines.Length) {
					sb.Append(Environment.NewLine);
				}
			} else if (s < (int)AssemblerSpecialCodes.EndOfLine) {
				// Convert error code to human-readable message
				string message = "unknown error";
				switch ((AssemblerSpecialCodes)s) {
					case AssemblerSpecialCodes.ParsingError: message = "Invalid syntax"; break;
					case AssemblerSpecialCodes.OutOfRangeJump: message = "Relative jump is out of range (-128 to 127)"; break;
					case AssemblerSpecialCodes.LabelRedefinition: message = "Cannot redefine an existing label"; break;
					case AssemblerSpecialCodes.MissingOperand: message = "Operand is missing"; break;
					case AssemblerSpecialCodes.OperandOutOfRange: message = "Operand is out of range (invalid value)"; break;
					case AssemblerSpecialCodes.InvalidHex: message = "Hexadecimal string is invalid"; break;
					case AssemblerSpecialCodes.InvalidSpaces: message = "Operand cannot contain spaces"; break;
					case AssemblerSpecialCodes.TrailingText: message = "Invalid text trailing at the end of line"; break;
					case AssemblerSpecialCodes.UnknownLabel: message = "Unknown label"; break;
					case AssemblerSpecialCodes.InvalidInstruction: message = "Invalid instruction"; break;
					case AssemblerSpecialCodes.InvalidBinaryValue: message = "Invalid binary value"; break;
					case AssemblerSpecialCodes.InvalidOperands: message = "Invalid operands for instruction"; break;
					case AssemblerSpecialCodes.InvalidLabel: message = "Invalid label name"; break;
				}

				errorList.Add(new AssemblerError() { Message = message + " - " + codeLines[line - 1], LineNumber = line });

				sb.Append("<error: " + message + ">");
				if (i + 1 < byteCode.Length) {
					sb.Append(Environment.NewLine);
				}

				line++;
			}
		}

		// Update validation state
		BytesUsed = convertedByteCode.Count;
		IsIdentical = MatchesOriginalCode(convertedByteCode);
		OriginalSizeExceeded = BytesUsed > OriginalByteCount;
		MaxSizeExceeded = StartAddress + BytesUsed - 1 > MaxAddress;
		OkEnabled = BytesUsed > 0 && !IsIdentical && !MaxSizeExceeded;

		HasWarning = errorList.Count > 0 || OriginalSizeExceeded || MaxSizeExceeded;

		ByteCodeView = sb.ToString();
		Errors = errorList;
		_bytes = convertedByteCode;
	}

	/// <summary>
	/// Compares the newly assembled code against the original code bytes.
	/// </summary>
	/// <param name="convertedByteCode">The newly assembled byte code.</param>
	/// <returns>True if the code is byte-for-byte identical to the original.</returns>
	private bool MatchesOriginalCode(List<byte> convertedByteCode) {
		bool isIdentical = false;
		if (_originalCode.Length > 0 && convertedByteCode.Count == _originalCode.Length) {
			isIdentical = true;
			for (int i = 0; i < _originalCode.Length; i++) {
				if (_originalCode[i] != convertedByteCode[i]) {
					isIdentical = false;
					break;
				}
			}
		}

		return isIdentical;
	}

	/// <summary>
	/// Applies the assembled code changes to the emulator's memory.
	/// </summary>
	/// <param name="assemblerWindow">The parent window for dialogs.</param>
	/// <returns>True if changes were applied; false if cancelled or failed.</returns>
	/// <remarks>
	/// <para>
	/// Shows confirmation dialogs for warnings (errors, size exceeded).
	/// Pads with NOP instructions if new code is smaller than original.
	/// Updates CDL flags to mark the modified range as code.
	/// </para>
	/// </remarks>
	public async Task<bool> ApplyChanges(Window assemblerWindow) {
		MemoryType memType = CpuType.ToMemoryType();

		List<byte> bytes = new List<byte>(_bytes);
		if (OriginalByteCount > 0) {
			byte nopOpCode = CpuType.GetNopOpCode();
			while (OriginalByteCount > bytes.Count) {
				//Pad data with NOPs as needed
				bytes.Add(nopOpCode);
			}
		}

		string addrFormat = memType.GetFormatString();
		UInt32 endAddress = (uint)(StartAddress + bytes.Count - 1);

		List<string> warningMessages = [];
		if (Errors.Count > 0) {
			warningMessages.Add("Warning: The code contains errors - lines with errors will be ignored.");
		}

		if (_originalAddress >= 0) {
			if (OriginalSizeExceeded) {
				warningMessages.Add(ResourceHelper.GetViewLabel(nameof(AssemblerWindow), "lblByteCountExceeded"));
			}
		} else {
			warningMessages.Add($"Warning: The code currently mapped to CPU memory addresses ${StartAddress.ToString(addrFormat)} to ${endAddress.ToString(addrFormat)} will be overridden.");
		}

		if (warningMessages.Count > 0 && await NexenMsgBox.Show(assemblerWindow, "AssemblerConfirmation", MessageBoxButtons.OKCancel, MessageBoxIcon.Warning, string.Join(Environment.NewLine + Environment.NewLine, warningMessages)) != DialogResult.OK) {
			return false;
		}


		DebugApi.SetMemoryValues(memType, (uint)StartAddress, bytes.ToArray(), bytes.Count);
		if (OriginalByteCount > 0) {
			_originalCode = bytes.ToArray();
			IsIdentical = MatchesOriginalCode(bytes);
		}

		AddressInfo absStart = DebugApi.GetAbsoluteAddress(new AddressInfo() { Address = StartAddress, Type = memType });
		AddressInfo absEnd = DebugApi.GetAbsoluteAddress(new AddressInfo() { Address = (int)endAddress, Type = memType });
		if (absStart.Type == absEnd.Type && (absEnd.Address - absStart.Address + 1) == bytes.Count) {
			DebugApi.MarkBytesAs(absStart.Type, (uint)absStart.Address, (uint)absEnd.Address, CdlFlags.Code);
		}

		DebuggerWindow? wnd = DebugWindowManager.GetDebugWindow<DebuggerWindow>(wnd => wnd.CpuType == CpuType);
		if (wnd is not null) {
			wnd.RefreshDisassembly();
		}

		return true;
	}

	/// <summary>
	/// Special codes returned by the assembler to indicate errors or line boundaries.
	/// </summary>
	private enum AssemblerSpecialCodes {
		/// <summary>Assembly succeeded for this byte.</summary>
		OK = 0,
		/// <summary>End of a source line.</summary>
		EndOfLine = -1,
		/// <summary>General parsing error.</summary>
		ParsingError = -2,
		/// <summary>Relative branch target is out of range.</summary>
		OutOfRangeJump = -3,
		/// <summary>Label was already defined.</summary>
		LabelRedefinition = -4,
		/// <summary>Required operand is missing.</summary>
		MissingOperand = -5,
		/// <summary>Operand value is out of range for the instruction.</summary>
		OperandOutOfRange = -6,
		/// <summary>Invalid hexadecimal number format.</summary>
		InvalidHex = -7,
		/// <summary>Operand contains invalid spaces.</summary>
		InvalidSpaces = -8,
		/// <summary>Unexpected text after instruction.</summary>
		TrailingText = -9,
		/// <summary>Referenced label does not exist.</summary>
		UnknownLabel = -10,
		/// <summary>Unrecognized instruction mnemonic.</summary>
		InvalidInstruction = -11,
		/// <summary>Invalid binary number format.</summary>
		InvalidBinaryValue = -12,
		/// <summary>Invalid operand combination for instruction.</summary>
		InvalidOperands = -13,
		/// <summary>Invalid label name format.</summary>
		InvalidLabel = -14,
	}
}

/// <summary>
/// Represents an assembly error at a specific line.
/// </summary>
public sealed class AssemblerError {
	/// <summary>Gets or sets the error message description.</summary>
	public string Message { get; set; } = "";

	/// <summary>Gets or sets the 1-based line number where the error occurred.</summary>
	public int LineNumber { get; set; }

	/// <summary>
	/// Returns a string representation of the error with line number.
	/// </summary>
	public override string ToString() {
		return "Line " + LineNumber.ToString() + ": " + this.Message;
	}
}
