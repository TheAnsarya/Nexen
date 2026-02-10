using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using Avalonia.Controls;
using Nexen.Debugger.Utilities;
using Nexen.Interop;
using Nexen.Utilities;
using Nexen.ViewModels;
using ReactiveUI;
using ReactiveUI.Fody.Helpers;

namespace Nexen.Debugger.ViewModels;

/// <summary>
/// ViewModel for editing NES ROM headers (iNES and NES 2.0 formats).
/// </summary>
/// <remarks>
/// <para>
/// Provides a full editing interface for NES ROM headers supporting both the
/// classic iNES format and the extended NES 2.0 specification. The editor
/// validates header values and provides real-time preview of the header bytes.
/// </para>
/// <para>
/// Key features:
/// <list type="bullet">
/// <item>PRG/CHR ROM size configuration with validation</item>
/// <item>Mapper and submapper selection</item>
/// <item>Mirroring type configuration</item>
/// <item>Battery/trainer flags</item>
/// <item>VS System and input type configuration (NES 2.0)</item>
/// <item>RAM size configuration (NES 2.0)</item>
/// <item>Real-time header byte preview</item>
/// </list>
/// </para>
/// </remarks>
public sealed class NesHeaderEditViewModel : DisposableViewModel {
	/// <summary>Gets the header data being edited.</summary>
	public NesHeader Header { get; }

	/// <summary>Gets whether the battery checkbox should be enabled.</summary>
	/// <remarks>
	/// Disabled when SaveRam or ChrRamBattery sizes are set, as those imply battery backup.
	/// </remarks>
	[Reactive] public bool IsBatteryCheckboxEnabled { get; private set; }

	/// <summary>Gets whether VS System options should be visible.</summary>
	[Reactive] public bool IsVsSystemVisible { get; private set; }

	/// <summary>Gets whether the header is in NES 2.0 format.</summary>
	[Reactive] public bool IsNes20 { get; private set; }

	/// <summary>Gets the available system types for selection.</summary>
	/// <remarks>Null for NES 2.0 (shows all), limited set for iNES.</remarks>
	[Reactive] public Enum[]? AvailableSystemTypes { get; private set; } = null;

	/// <summary>Gets the available timing options for selection.</summary>
	/// <remarks>Null for NES 2.0 (shows all), limited set for iNES.</remarks>
	[Reactive] public Enum[]? AvailableTimings { get; private set; } = null;

	/// <summary>Gets the 16-byte header preview as hex string.</summary>
	[Reactive] public string HeaderBytes { get; private set; } = "";

	/// <summary>Gets any validation error message.</summary>
	[Reactive] public string ErrorMessage { get; private set; } = "";

	/// <summary>The ROM information for the current file.</summary>
	private RomInfo _romInfo;

	/// <summary>
	/// Initializes a new instance of the <see cref="NesHeaderEditViewModel"/> class.
	/// </summary>
	/// <remarks>
	/// Reads the current ROM's header from the emulator and initializes the editor.
	/// If no debug windows are open, temporarily initializes and releases the debugger.
	/// </remarks>
	public NesHeaderEditViewModel() {
		bool releaseDebugger = !DebugWindowManager.HasOpenedDebugWindows();
		bool paused = EmuApi.IsPaused();
		byte[] headerBytes = DebugApi.GetRomHeader();
		if (releaseDebugger) {
			//GetRomHeader will initialize the debugger - stop the debugger if no other debug window is opened
			DebugApi.ReleaseDebugger();
			if (paused) {
				EmuApi.Pause();
			}
		}

		if (headerBytes.Length < 16) {
			Array.Resize(ref headerBytes, 16);
		}

		_romInfo = EmuApi.GetRomInfo();
		Header = NesHeader.FromBytes(headerBytes);

		AddDisposable(this.WhenAnyValue(x => x.Header.SaveRam, x => x.Header.ChrRamBattery).Subscribe(x => {
			IsBatteryCheckboxEnabled = Header.SaveRam == MemorySizes.None && Header.ChrRamBattery == MemorySizes.None;
			if (!IsBatteryCheckboxEnabled) {
				Header.HasBattery = true;
			}
		}));

		AddDisposable(this.WhenAnyValue(x => x.Header.System, x => x.Header.FileType).Subscribe(x => {
			bool isVsSystem = Header.System == TvSystem.VsSystem;
			IsVsSystemVisible = isVsSystem && Header.FileType == NesFileType.Nes2_0;
			if (!IsVsSystemVisible) {
				Header.VsPpu = VsPpuType.RP2C03B;
				Header.VsSystem = VsSystemType.Default;
			}
		}));

		AddDisposable(this.WhenAnyValue(x => x.Header.FileType).Subscribe(x => {
			IsNes20 = Header.FileType == NesFileType.Nes2_0;

			if (IsNes20) {
				AvailableSystemTypes = null;
				AvailableTimings = null;
			} else {
				AvailableSystemTypes = new Enum[] { TvSystem.NesFamicomDendy, TvSystem.VsSystem, TvSystem.Playchoice };
				AvailableTimings = new Enum[] { FrameTiming.Ntsc, FrameTiming.Pal, FrameTiming.NtscAndPal };

				Header.SubmapperId = 0;
				Header.ChrRam = MemorySizes.None;
				Header.ChrRamBattery = MemorySizes.None;
				Header.SaveRam = MemorySizes.None;
				Header.WorkRam = MemorySizes.None;
			}
		}));

		Header.PropertyChanged += Header_PropertyChanged;
		UpdateHeaderPreview();
	}

	/// <summary>
	/// Disposes the ViewModel and unsubscribes from header property changes.
	/// </summary>
	protected override void DisposeView() {
		base.DisposeView();
		Header.PropertyChanged -= Header_PropertyChanged;
	}

	/// <summary>
	/// Handles property changes on the header to update preview and validation.
	/// </summary>
	private void Header_PropertyChanged(object? sender, System.ComponentModel.PropertyChangedEventArgs e) {
		UpdateHeaderPreview();
		ErrorMessage = GetErrorMessage();
	}

	/// <summary>
	/// Updates the header bytes preview string from current header values.
	/// </summary>
	private void UpdateHeaderPreview() {
		byte[] headerBytes = Header.ToBytes();
		StringBuilder sb = new();
		for (int i = 0; i < 16; i++) {
			sb.Append(headerBytes[i].ToString("X2") + " ");
		}

		HeaderBytes = sb.ToString().Trim();
	}

	/// <summary>
	/// Validates the current header values and returns any error message.
	/// </summary>
	/// <returns>Error message string, or empty string if valid.</returns>
	/// <remarks>
	/// Validates:
	/// <list type="bullet">
	/// <item>PRG ROM size must be multiple of 16KB (unless valid NES 2.0 exponent value)</item>
	/// <item>CHR ROM size must be multiple of 8KB (unless valid NES 2.0 exponent value)</item>
	/// <item>NES 2.0: Mapper ID &lt; 4096, Submapper ID &lt; 16</item>
	/// <item>iNES: Mapper ID &lt; 256</item>
	/// <item>Size limits based on format</item>
	/// </list>
	/// </remarks>
	private string GetErrorMessage() {
		bool isValidPrgSize = Header.FileType == NesFileType.Nes2_0 && NesHeader.IsValidSize(Header.PrgRom);
		bool isValidChrSize = Header.FileType == NesFileType.Nes2_0 && NesHeader.IsValidSize(Header.ChrRom);
		if (!isValidPrgSize && (Header.PrgRom % 16) != 0) {
			return "Error: PRG ROM size must be a multiple of 16 KB";
		}

		if (!isValidChrSize && (Header.ChrRom % 8) != 0) {
			return "Error: CHR ROM size must be a multiple of 8 KB";
		}

		if (Header.FileType == NesFileType.Nes2_0) {
			if (Header.MapperId >= 4096) {
				return "Error: Mapper ID must be lower than 4096";
			}

			if (Header.SubmapperId >= 16) {
				return "Error: Submapper ID must be lower than 16";
			}

			if (!isValidChrSize && Header.ChrRom >= 16384) {
				return "Error: CHR ROM size must be lower than 16384 KB";
			}

			if (!isValidPrgSize && Header.PrgRom >= 32768) {
				return "Error: PRG ROM size must be lower than 32768 KB";
			}
		} else {
			if (Header.MapperId >= 256) {
				return "Error: Mapper ID must be lower than 256 ";
			}

			if (Header.ChrRom >= 2048) {
				return "Error: CHR ROM size must be lower than 2048 KB";
			}

			if (Header.PrgRom >= 4096) {
				return "Error: PRG ROM size must be lower than 4096 KB";
			}
		}

		return "";
	}

	/// <summary>
	/// Saves the ROM with the modified header to a file.
	/// </summary>
	/// <param name="wnd">The parent window for the save dialog.</param>
	/// <returns>True if save was successful, false otherwise.</returns>
	public async Task<bool> Save(Window wnd) {
		string? filepath = await FileDialogHelper.SaveFile(Path.GetDirectoryName(_romInfo.RomPath), Path.GetFileName(_romInfo.RomPath), wnd, FileDialogHelper.NesExt);
		if (filepath is not null) {
			byte[]? data = FileHelper.ReadAllBytes(_romInfo.RomPath);
			if (data is not null) {
				byte[] header = Header.ToBytes();
				for (int i = 0; i < 16; i++) {
					data[i] = header[i];
				}

				return FileHelper.WriteAllBytes(filepath, data);
			}
		}

		return false;
	}

	/// <summary>
	/// Represents NES ROM header data with properties for all iNES and NES 2.0 fields.
	/// </summary>
	/// <remarks>
	/// <para>
	/// Provides bidirectional conversion between the 16-byte binary header format
	/// and individual property values for editing.
	/// </para>
	/// <para>
	/// NES 2.0 extends iNES with:
	/// <list type="bullet">
	/// <item>Extended mapper IDs (up to 4095)</item>
	/// <item>Submapper support</item>
	/// <item>Larger ROM sizes via exponent encoding</item>
	/// <item>Separate work RAM and save RAM size fields</item>
	/// <item>CHR RAM and battery-backed CHR RAM fields</item>
	/// <item>Frame timing specification</item>
	/// <item>VS System PPU and protection types</item>
	/// <item>Input device specification</item>
	/// </list>
	/// </para>
	/// </remarks>
	public sealed class NesHeader : ViewModelBase {
		/// <summary>
		/// Maps NES 2.0 exponent-based sizes to their encoded byte values.
		/// </summary>
		/// <remarks>
		/// NES 2.0 uses an exponent+multiplier encoding for ROM sizes that don't fit
		/// the standard KB multiples: value = multiplier * (2^exponent) / 1024
		/// </remarks>
		private static Dictionary<UInt64, int> _validSizeValues = new Dictionary<UInt64, int>();

		/// <summary>Gets or sets the header format type (iNES or NES 2.0).</summary>
		[Reactive] public NesFileType FileType { get; set; }

		/// <summary>Gets or sets the mapper ID (0-255 for iNES, 0-4095 for NES 2.0).</summary>
		[Reactive] public uint MapperId { get; set; }

		/// <summary>Gets or sets the submapper ID (NES 2.0 only, 0-15).</summary>
		[Reactive] public uint SubmapperId { get; set; }

		/// <summary>Gets or sets the PRG ROM size in KB.</summary>
		[Reactive] public UInt64 PrgRom { get; set; }

		/// <summary>Gets or sets the CHR ROM size in KB (0 for CHR RAM games).</summary>
		[Reactive] public UInt64 ChrRom { get; set; }

		/// <summary>Gets or sets the nametable mirroring type.</summary>
		[Reactive] public iNesMirroringType Mirroring { get; set; }

		/// <summary>Gets or sets the frame timing (NTSC/PAL/Dendy).</summary>
		[Reactive] public FrameTiming Timing { get; set; }

		/// <summary>Gets or sets the TV/console system type.</summary>
		[Reactive] public TvSystem System { get; set; }

		/// <summary>Gets or sets whether a 512-byte trainer is present.</summary>
		[Reactive] public bool HasTrainer { get; set; }

		/// <summary>Gets or sets whether the cartridge has battery-backed memory.</summary>
		[Reactive] public bool HasBattery { get; set; }

		/// <summary>Gets or sets the VS System PPU type.</summary>
		[Reactive] public VsPpuType VsPpu { get; set; }

		/// <summary>Gets or sets the VS System protection type.</summary>
		[Reactive] public VsSystemType VsSystem { get; set; }

		/// <summary>Gets or sets the input device type.</summary>
		[Reactive] public GameInputType InputType { get; set; }

		/// <summary>Gets or sets the work RAM size (NES 2.0 only).</summary>
		[Reactive] public MemorySizes WorkRam { get; set; } = MemorySizes.None;

		/// <summary>Gets or sets the battery-backed save RAM size (NES 2.0 only).</summary>
		[Reactive] public MemorySizes SaveRam { get; set; } = MemorySizes.None;

		/// <summary>Gets or sets the CHR RAM size (NES 2.0 only).</summary>
		[Reactive] public MemorySizes ChrRam { get; set; } = MemorySizes.None;

		/// <summary>Gets or sets the battery-backed CHR RAM size (NES 2.0 only).</summary>
		[Reactive] public MemorySizes ChrRamBattery { get; set; } = MemorySizes.None;

		/// <summary>
		/// Static constructor that initializes the valid size lookup table.
		/// </summary>
		static NesHeader() {
			_validSizeValues = new Dictionary<UInt64, int>();
			for (int i = 0; i < 256; i++) {
				int multiplier = ((i & 0x03) * 2) + 1;
				UInt64 value = ((UInt64)1 << (i >> 2)) / 1024;
				_validSizeValues[(UInt64)multiplier * value] = i;
			}
		}

		/// <summary>
		/// Converts the header properties to a 16-byte binary header.
		/// </summary>
		/// <returns>16-byte array containing the NES header.</returns>
		/// <remarks>
		/// Generates either iNES or NES 2.0 format based on <see cref="FileType"/>.
		/// For NES 2.0, supports the exponent+multiplier encoding for large ROM sizes.
		/// </remarks>
		public byte[] ToBytes() {
			byte[] header = new byte[16];
			header[0] = 0x4E;
			header[1] = 0x45;
			header[2] = 0x53;
			header[3] = 0x1A;

			UInt64 prgRomValue = PrgRom / 16;
			UInt64 chrRomValue = ChrRom / 8;

			if (FileType == NesFileType.Nes2_0) {
				if ((PrgRom % 16) != 0 || PrgRom >= 32768) {
					if (_validSizeValues.ContainsKey(PrgRom)) {
						//This value is a valid exponent+multiplier combo (NES 2.0 only)
						prgRomValue = ((uint)_validSizeValues[PrgRom] & 0xFF) | 0xF00;
					}
				}

				if ((ChrRom % 8) != 0 || ChrRom >= 16384) {
					if (_validSizeValues.ContainsKey(ChrRom)) {
						//This value is a valid exponent+multiplier combo (NES 2.0 only)
						chrRomValue = ((uint)_validSizeValues[ChrRom] & 0xFF) | 0xF00;
					}
				}

				//NES 2.0
				header[4] = (byte)prgRomValue;
				header[5] = (byte)chrRomValue;

				header[6] = (byte)(
					((byte)(MapperId & 0x0F) << 4) |
					(byte)Mirroring | (HasTrainer ? 0x04 : 0x00) | (HasBattery ? 0x02 : 0x00)
				);

				header[7] = (byte)(MapperId & 0xF0);

				switch (System) {
					case TvSystem.NesFamicomDendy: header[7] |= 0x00; break;
					case TvSystem.VsSystem: header[7] |= 0x01; break;
					case TvSystem.Playchoice: header[7] |= 0x02; break;
					default: header[7] |= 0x03; break;
				}

				//Enable NES 2.0 header
				header[7] |= 0x08;

				header[8] = (byte)(((SubmapperId & 0x0F) << 4) | ((MapperId & 0xF00) >> 8));
				header[9] = (byte)(((prgRomValue & 0xF00) >> 8) | ((chrRomValue & 0xF00) >> 4));

				header[10] = (byte)((byte)WorkRam | (((byte)SaveRam) << 4));
				header[11] = (byte)((byte)ChrRam | (((byte)ChrRamBattery) << 4));

				header[12] = (byte)Timing;

				header[13] = System == TvSystem.VsSystem ? (byte)(((byte)VsPpu & 0x0F) | ((((byte)VsSystem) & 0x0F) << 4)) : (byte)System;
				header[14] = 0;
				header[15] = (byte)InputType;
			} else {
				//iNES
				header[4] = prgRomValue == 0x100 ? (byte)0 : (byte)prgRomValue;
				header[5] = (byte)chrRomValue;

				header[6] = (byte)(
					((byte)(MapperId & 0x0F) << 4) |
					(byte)Mirroring | (HasTrainer ? 0x04 : 0x00) | (HasBattery ? 0x02 : 0x00)
				);
				header[7] = (byte)(
					((byte)MapperId & 0xF0) |
					(byte)(System == TvSystem.VsSystem ? 0x01 : 0x00) | (System == TvSystem.Playchoice ? 0x02 : 0x00)
				);

				header[8] = 0;
				header[9] = (byte)(Timing == FrameTiming.Pal ? 0x01 : 0x00);
				header[10] = 0;
				header[11] = 0;
				header[12] = 0;
				header[13] = 0;
				header[14] = 0;
				header[15] = 0;
			}

			return header;
		}

		/// <summary>
		/// Parses a 16-byte binary header into a NesHeader object.
		/// </summary>
		/// <param name="bytes">The 16-byte header data.</param>
		/// <returns>A new NesHeader populated from the binary data.</returns>
		public static NesHeader FromBytes(byte[] bytes) {
			BinaryHeader binHeader = new BinaryHeader(bytes);

			NesHeader header = new NesHeader();
			header.FileType = binHeader.GetRomHeaderVersion() == RomHeaderVersion.Nes2_0 ? NesFileType.Nes2_0 : NesFileType.iNes;
			header.PrgRom = (uint)binHeader.GetPrgSize();
			header.ChrRom = (uint)binHeader.GetChrSize();
			header.HasTrainer = binHeader.HasTrainer();
			header.HasBattery = binHeader.HasBattery();

			header.System = binHeader.GetTvSystem();
			header.Timing = binHeader.GetFrameTiming();

			header.Mirroring = binHeader.GetMirroringType();
			header.MapperId = (uint)binHeader.GetMapperID();
			header.SubmapperId = (uint)binHeader.GetSubMapper();
			header.WorkRam = (MemorySizes)binHeader.GetWorkRamSize();
			header.SaveRam = (MemorySizes)binHeader.GetSaveRamSize();
			header.ChrRam = (MemorySizes)binHeader.GetChrRamSize();
			header.ChrRamBattery = (MemorySizes)binHeader.GetSaveChrRamSize();
			header.InputType = binHeader.GetInputType();
			header.VsPpu = (VsPpuType)(bytes[13] & 0x0F);
			header.VsSystem = binHeader.GetVsSystemType();

			return header;
		}

		/// <summary>
		/// Checks if a size value is valid for NES 2.0 exponent encoding.
		/// </summary>
		/// <param name="size">The size in KB to check.</param>
		/// <returns>True if the size can be encoded using the exponent+multiplier format.</returns>
		public static bool IsValidSize(ulong size) {
			return _validSizeValues.ContainsKey(size);
		}
	}

	/// <summary>
	/// Internal helper class for parsing binary NES headers.
	/// </summary>
	/// <remarks>
	/// Provides low-level access to header bytes with version-aware parsing
	/// for iNES, NES 2.0, and old iNES formats.
	/// </remarks>
	private class BinaryHeader {
		/// <summary>The raw header bytes.</summary>
		private byte[] _bytes;

		/// <summary>PRG ROM page count from byte 4.</summary>
		private byte PrgCount;

		/// <summary>CHR ROM page count from byte 5.</summary>
		private byte ChrCount;

		/// <summary>
		/// Initializes a new instance with the header bytes.
		/// </summary>
		/// <param name="bytes">The 16-byte header.</param>
		public BinaryHeader(byte[] bytes) {
			_bytes = bytes;
			PrgCount = bytes[4];
			ChrCount = bytes[5];
		}

		/// <summary>
		/// Determines the ROM header format version.
		/// </summary>
		/// <returns>The detected header version.</returns>
		public RomHeaderVersion GetRomHeaderVersion() {
			if ((_bytes[7] & 0x0C) == 0x08) {
				return RomHeaderVersion.Nes2_0;
			} else if ((_bytes[7] & 0x0C) == 0x00) {
				return RomHeaderVersion.iNes;
			} else {
				return RomHeaderVersion.OldiNes;
			}
		}

		/// <summary>Gets the mapper ID from the header.</summary>
		public int GetMapperID() {
			switch (GetRomHeaderVersion()) {
				case RomHeaderVersion.Nes2_0:
					return ((_bytes[8] & 0x0F) << 8) | (_bytes[7] & 0xF0) | (_bytes[6] >> 4);
				default:
				case RomHeaderVersion.iNes:
					return (_bytes[7] & 0xF0) | (_bytes[6] >> 4);
				case RomHeaderVersion.OldiNes:
					return _bytes[6] >> 4;
			}
		}

		/// <summary>Gets whether the cartridge has battery backup.</summary>
		public bool HasBattery() {
			return (_bytes[6] & 0x02) == 0x02;
		}

		/// <summary>Gets whether a 512-byte trainer is present.</summary>
		public bool HasTrainer() {
			return (_bytes[6] & 0x04) == 0x04;
		}

		/// <summary>Gets the frame timing (NTSC/PAL) from the header.</summary>
		public FrameTiming GetFrameTiming() {
			if (GetRomHeaderVersion() == RomHeaderVersion.Nes2_0) {
				return (FrameTiming)(_bytes[12] & 0x03);
			} else if (GetRomHeaderVersion() == RomHeaderVersion.iNes) {
				return (_bytes[9] & 0x01) == 0x01 ? FrameTiming.Pal : FrameTiming.Ntsc;
			}

			return FrameTiming.Ntsc;
		}

		/// <summary>Gets the TV/console system type from the header.</summary>
		public TvSystem GetTvSystem() {
			if (GetRomHeaderVersion() == RomHeaderVersion.Nes2_0) {
				switch (_bytes[7] & 0x03) {
					case 0: return TvSystem.NesFamicomDendy;
					case 1: return TvSystem.VsSystem;
					case 2: return TvSystem.Playchoice;
					case 3: return (TvSystem)_bytes[13];
				}
			} else if (GetRomHeaderVersion() == RomHeaderVersion.iNes) {
				if ((_bytes[7] & 0x01) == 0x01) {
					return TvSystem.VsSystem;
				} else if ((_bytes[7] & 0x02) == 0x02) {
					return TvSystem.Playchoice;
				}
			}

			return TvSystem.NesFamicomDendy;
		}

		/// <summary>
		/// Calculates size from NES 2.0 exponent+multiplier encoding.
		/// </summary>
		/// <param name="exponent">The exponent value (bits 2-7).</param>
		/// <param name="multiplier">The multiplier value (bits 0-1).</param>
		/// <returns>The size in KB.</returns>
		private UInt64 GetSizeValue(int exponent, int multiplier) {
			multiplier = (multiplier * 2) + 1;
			return (UInt64)multiplier * (((UInt64)1 << exponent) / 1024);
		}

		/// <summary>Gets the PRG ROM size in KB.</summary>
		public UInt64 GetPrgSize() {
			if (GetRomHeaderVersion() == RomHeaderVersion.Nes2_0) {
				if ((_bytes[9] & 0x0F) == 0x0F) {
					return GetSizeValue(PrgCount >> 2, PrgCount & 0x03);
				} else {
					return (UInt64)(((_bytes[9] & 0x0F) << 8) | PrgCount) * 16;
				}
			} else {
				if (PrgCount == 0) {
					return 256 * 16; //0 is a special value and means 256
				} else {
					return (UInt64)PrgCount * 16;
				}
			}
		}

		/// <summary>Gets the CHR ROM size in KB.</summary>
		public UInt64 GetChrSize() {
			if (GetRomHeaderVersion() == RomHeaderVersion.Nes2_0) {
				if ((_bytes[9] & 0xF0) == 0xF0) {
					return GetSizeValue(ChrCount >> 2, ChrCount & 0x03);
				} else {
					return (UInt64)(((_bytes[9] & 0xF0) << 4) | ChrCount) * 8;
				}
			} else {
				return (UInt64)ChrCount * 8;
			}
		}

		/// <summary>Gets the work RAM size index (NES 2.0 only).</summary>
		public int GetWorkRamSize() {
			if (GetRomHeaderVersion() == RomHeaderVersion.Nes2_0) {
				return _bytes[10] & 0x0F;
			} else {
				return 0;
			}
		}

		/// <summary>Gets the save RAM size index (NES 2.0 only).</summary>
		public int GetSaveRamSize() {
			if (GetRomHeaderVersion() == RomHeaderVersion.Nes2_0) {
				return (_bytes[10] & 0xF0) >> 4;
			} else {
				return 0;
			}
		}

		/// <summary>Gets the CHR RAM size index (NES 2.0 only).</summary>
		public int GetChrRamSize() {
			if (GetRomHeaderVersion() == RomHeaderVersion.Nes2_0) {
				return _bytes[11] & 0x0F;
			} else {
				return 0;
			}
		}

		/// <summary>Gets the battery-backed CHR RAM size index (NES 2.0 only).</summary>
		public int GetSaveChrRamSize() {
			if (GetRomHeaderVersion() == RomHeaderVersion.Nes2_0) {
				return (_bytes[11] & 0xF0) >> 4;
			} else {
				return 0;
			}
		}

		/// <summary>Gets the submapper ID (NES 2.0 only).</summary>
		public int GetSubMapper() {
			if (GetRomHeaderVersion() == RomHeaderVersion.Nes2_0) {
				return (_bytes[8] & 0xF0) >> 4;
			} else {
				return 0;
			}
		}

		/// <summary>Gets the nametable mirroring type.</summary>
		public iNesMirroringType GetMirroringType() {
			if ((_bytes[6] & 0x08) != 0) {
				return iNesMirroringType.FourScreens;
			} else {
				return (_bytes[6] & 0x01) != 0 ? iNesMirroringType.Vertical : iNesMirroringType.Horizontal;
			}
		}

		/// <summary>Gets the input device type (NES 2.0 only).</summary>
		public GameInputType GetInputType() {
			if (GetRomHeaderVersion() == RomHeaderVersion.Nes2_0) {
				if (_bytes[15] < Enum.GetValues<GameInputType>().Length) {
					return (GameInputType)_bytes[15];
				}

				return GameInputType.Unspecified;
			} else {
				return GameInputType.Unspecified;
			}
		}

		/// <summary>Gets the VS System protection type (NES 2.0 only).</summary>
		public VsSystemType GetVsSystemType() {
			if (GetRomHeaderVersion() == RomHeaderVersion.Nes2_0) {
				if ((_bytes[13] >> 4) <= 0x06) {
					return (VsSystemType)(_bytes[13] >> 4);
				}
			}

			return VsSystemType.Default;
		}
	}

	/// <summary>
	/// Specifies the NES ROM header format version detected during parsing.
	/// </summary>
	public enum RomHeaderVersion {
		/// <summary>Standard iNES format (most common).</summary>
		iNes = 0,
		/// <summary>NES 2.0 extended format with additional metadata.</summary>
		Nes2_0 = 1,
		/// <summary>Old iNES format with potentially corrupted bytes 7-15.</summary>
		OldiNes = 2
	}

	/// <summary>
	/// Specifies the NES ROM header file type for editing.
	/// </summary>
	public enum NesFileType {
		/// <summary>Standard iNES format.</summary>
		iNes = 0,
		/// <summary>NES 2.0 extended format.</summary>
		Nes2_0 = 1
	}

	/// <summary>
	/// Specifies the nametable mirroring arrangement.
	/// </summary>
	public enum iNesMirroringType {
		/// <summary>Horizontal mirroring (vertical scrolling games).</summary>
		Horizontal = 0,
		/// <summary>Vertical mirroring (horizontal scrolling games).</summary>
		Vertical = 1,
		/// <summary>Four-screen VRAM (uses additional cartridge RAM).</summary>
		FourScreens = 8
	}

	/// <summary>
	/// Specifies the frame timing/region.
	/// </summary>
	public enum FrameTiming {
		/// <summary>NTSC timing (60Hz, North America/Japan).</summary>
		Ntsc = 0,
		/// <summary>PAL timing (50Hz, Europe).</summary>
		Pal = 1,
		/// <summary>Multi-region (works with both NTSC and PAL).</summary>
		NtscAndPal = 2,
		/// <summary>Dendy timing (Soviet/Russian clone systems).</summary>
		Dendy = 3
	}

	/// <summary>
	/// Specifies the TV/console system type.
	/// </summary>
	public enum TvSystem {
		/// <summary>Standard NES/Famicom/Dendy.</summary>
		NesFamicomDendy,
		/// <summary>VS System arcade hardware.</summary>
		VsSystem,
		/// <summary>PlayChoice-10 arcade hardware.</summary>
		Playchoice,
		/// <summary>Clone with decimal mode support.</summary>
		CloneWithDecimal,
		/// <summary>EPSM (Expansion Port Sound Module).</summary>
		EpsmModule,
		/// <summary>VT01 Red/Cyan variant.</summary>
		Vt01RedCyan,
		/// <summary>VT02 system.</summary>
		Vt02,
		/// <summary>VT03 system.</summary>
		Vt03,
		/// <summary>VT09 system.</summary>
		Vt09,
		/// <summary>VT32 system.</summary>
		Vt32,
		/// <summary>VT369 system.</summary>
		Vt369,
		/// <summary>UMC UM6578 system.</summary>
		UmcUm6578,
		/// <summary>Famicom Network System.</summary>
		FamicomNetworkSystem,
		/// <summary>Reserved for future use.</summary>
		ReservedD,
		/// <summary>Reserved for future use.</summary>
		ReservedE,
		/// <summary>Reserved for future use.</summary>
		ReservedF
	}

	/// <summary>
	/// Specifies RAM/ROM sizes using NES 2.0 shift count encoding.
	/// </summary>
	/// <remarks>
	/// Size = 64 &lt;&lt; shift_count bytes (except None which is 0).
	/// </remarks>
	public enum MemorySizes {
		/// <summary>No memory.</summary>
		None = 0,
		/// <summary>128 bytes.</summary>
		_128Bytes = 1,
		/// <summary>256 bytes.</summary>
		_256Bytes = 2,
		/// <summary>512 bytes.</summary>
		_512Bytes = 3,
		/// <summary>1 KB.</summary>
		_1KB = 4,
		/// <summary>2 KB.</summary>
		_2KB = 5,
		/// <summary>4 KB.</summary>
		_4KB = 6,
		/// <summary>8 KB.</summary>
		_8KB = 7,
		/// <summary>16 KB.</summary>
		_16KB = 8,
		/// <summary>32 KB.</summary>
		_32KB = 9,
		/// <summary>64 KB.</summary>
		_64KB = 10,
		/// <summary>128 KB.</summary>
		_128KB = 11,
		/// <summary>256 KB.</summary>
		_256KB = 12,
		/// <summary>512 KB.</summary>
		_512KB = 13,
		/// <summary>1024 KB (1 MB).</summary>
		_1024KB = 14,
		/// <summary>Reserved value.</summary>
		Reserved = 15
	}

	/// <summary>
	/// Specifies the VS System PPU type.
	/// </summary>
	public enum VsPpuType {
		/// <summary>RP2C03B (standard).</summary>
		RP2C03B = 0,
		/// <summary>RP2C03G.</summary>
		RP2C03G = 1,
		/// <summary>RP2C04-0001.</summary>
		RP2C040001 = 2,
		/// <summary>RP2C04-0002.</summary>
		RP2C040002 = 3,
		/// <summary>RP2C04-0003.</summary>
		RP2C040003 = 4,
		/// <summary>RP2C04-0004.</summary>
		RP2C040004 = 5,
		/// <summary>RC2C03B.</summary>
		RC2C03B = 6,
		/// <summary>RC2C03C.</summary>
		RC2C03C = 7,
		/// <summary>RC2C05-01.</summary>
		RC2C0501 = 8,
		/// <summary>RC2C05-02.</summary>
		RC2C0502 = 9,
		/// <summary>RC2C05-03.</summary>
		RC2C0503 = 10,
		/// <summary>RC2C05-04.</summary>
		RC2C0504 = 11,
		/// <summary>RC2C05-05.</summary>
		RC2C0505 = 12,
		/// <summary>Reserved.</summary>
		ReservedD = 13,
		/// <summary>Reserved.</summary>
		ReservedE = 14,
		/// <summary>Reserved.</summary>
		ReservedF = 15
	}

	/// <summary>
	/// Specifies VS System protection/hardware type.
	/// </summary>
	public enum VsSystemType {
		/// <summary>Default VS System.</summary>
		Default = 0,
		/// <summary>RBI Baseball protection.</summary>
		RbiBaseballProtection = 1,
		/// <summary>TKO Boxing protection.</summary>
		TkoBoxingProtection = 2,
		/// <summary>Super Xevious protection.</summary>
		SuperXeviousProtection = 3,
		/// <summary>Ice Climber protection.</summary>
		IceClimberProtection = 4,
		/// <summary>VS Dual System hardware.</summary>
		VsDualSystem = 5,
		/// <summary>Raid on Bungeling Bay protection.</summary>
		RaidOnBungelingBayProtection = 6,
	}

	/// <summary>
	/// Specifies the input device type for the game.
	/// </summary>
	/// <remarks>
	/// NES 2.0 header byte 15 specifies the default input device.
	/// Many games work with standard controllers regardless of this setting.
	/// </remarks>
	public enum GameInputType {
		/// <summary>Input type not specified.</summary>
		Unspecified = 0,
		/// <summary>Standard NES controllers.</summary>
		StandardControllers = 1,
		/// <summary>NES Four Score adapter.</summary>
		FourScore = 2,
		/// <summary>Famicom 4-player adapter.</summary>
		FourPlayerAdapter = 3,
		/// <summary>VS System controls.</summary>
		VsSystem = 4,
		/// <summary>VS System with swapped controllers.</summary>
		VsSystemSwapped = 5,
		/// <summary>VS System with swapped A/B buttons.</summary>
		VsSystemSwapAB = 6,
		/// <summary>VS System Zapper.</summary>
		VsZapper = 7,
		/// <summary>NES Zapper light gun.</summary>
		Zapper = 8,
		/// <summary>Two Zappers.</summary>
		TwoZappers = 9,
		/// <summary>Bandai Hyper Shot.</summary>
		BandaiHypershot = 0x0A,
		/// <summary>Power Pad Side A.</summary>
		PowerPadSideA = 0x0B,
		/// <summary>Power Pad Side B.</summary>
		PowerPadSideB = 0x0C,
		/// <summary>Family Trainer Side A.</summary>
		FamilyTrainerSideA = 0x0D,
		/// <summary>Family Trainer Side B.</summary>
		FamilyTrainerSideB = 0x0E,
		/// <summary>Arkanoid Controller (NES).</summary>
		ArkanoidControllerNes = 0x0F,
		/// <summary>Arkanoid Controller (Famicom).</summary>
		ArkanoidControllerFamicom = 0x10,
		/// <summary>Two Arkanoid Controllers.</summary>
		DoubleArkanoidController = 0x11,
		/// <summary>Konami Hyper Shot.</summary>
		KonamiHyperShot = 0x12,
		/// <summary>Pachinko Controller.</summary>
		PachinkoController = 0x13,
		/// <summary>Exciting Boxing punching bag.</summary>
		ExcitingBoxing = 0x14,
		/// <summary>Jissen Mahjong Controller.</summary>
		JissenMahjong = 0x15,
		/// <summary>Party Tap controller.</summary>
		PartyTap = 0x16,
		/// <summary>Oeka Kids Tablet.</summary>
		OekaKidsTablet = 0x17,
		/// <summary>Barcode Battler.</summary>
		BarcodeBattler = 0x18,
		/// <summary>Miracle Piano (not supported).</summary>
		MiraclePiano = 0x19,
		/// <summary>Pokkun Moguraa (not supported).</summary>
		PokkunMoguraa = 0x1A,
		/// <summary>Top Rider (not supported).</summary>
		TopRider = 0x1B,
		/// <summary>Double Fisted (not supported).</summary>
		DoubleFisted = 0x1C,
		/// <summary>Famicom 3D System (not supported).</summary>
		Famicom3dSystem = 0x1D,
		/// <summary>Doremikko Keyboard (not supported).</summary>
		DoremikkoKeyboard = 0x1E,
		/// <summary>R.O.B. (not supported).</summary>
		ROB = 0x1F,
		/// <summary>Famicom Data Recorder.</summary>
		FamicomDataRecorder = 0x20,
		/// <summary>Turbo File storage device.</summary>
		TurboFile = 0x21,
		/// <summary>Battle Box.</summary>
		BattleBox = 0x22,
		/// <summary>Family BASIC Keyboard.</summary>
		FamilyBasicKeyboard = 0x23,
		/// <summary>PEC-586 Keyboard (not supported).</summary>
		Pec586Keyboard = 0x24,
		/// <summary>Bit-79 Keyboard (not supported).</summary>
		Bit79Keyboard = 0x25,
		/// <summary>Subor Keyboard.</summary>
		SuborKeyboard = 0x26,
		/// <summary>Subor Keyboard with Mouse (variant 1).</summary>
		SuborKeyboardMouse1 = 0x27,
		/// <summary>Subor Keyboard with Mouse (variant 2).</summary>
		SuborKeyboardMouse2 = 0x28,
		/// <summary>SNES Mouse.</summary>
		SnesMouse = 0x29,
		/// <summary>Generic Multicart (not supported).</summary>
		GenericMulticart = 0x2A,
		/// <summary>SNES Controllers.</summary>
		SnesControllers = 0x2B,
		/// <summary>RacerMate Bicycle (not supported).</summary>
		RacerMateBicycle = 0x2C,
		/// <summary>U-Force (not supported).</summary>
		UForce = 0x2D,
		/// <summary>R.O.B. Stack-Up (not supported).</summary>
		RobStackUp = 0x2E,
		/// <summary>City Patrolman Lightgun (not supported).</summary>
		CityPatrolmanLightgun = 0x2F,
		/// <summary>Sharp C1 Cassette Interface (not supported).</summary>
		SharpC1CassetteInterface = 0x30,
		/// <summary>Standard controller with swapped buttons (not supported).</summary>
		StandardControllerWithSwappedButtons = 0x31,
		/// <summary>Excalibor Sudoku Pad (not supported).</summary>
		ExcaliborSudokuPad = 0x32,
		/// <summary>ABL Pinball (not supported).</summary>
		AblPinball = 0x33,
		/// <summary>Golden Nugget Casino (not supported).</summary>
		GoldenNuggetCasino = 0x34
	}
}
