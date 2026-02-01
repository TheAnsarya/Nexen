using System;
using System.Globalization;
using System.Text;
using Nexen.Interop;
using Nexen.Utilities;
using ReactiveUI;
using ReactiveUI.Fody.Helpers;

namespace Nexen.Debugger.Labels {
	/// <summary>
	/// Represents a label (symbol) attached to a memory address in the debugger.
	/// Labels provide human-readable names for addresses and can include comments.
	/// </summary>
	/// <remarks>
	/// <para>
	/// CodeLabel is the primary data structure for debugger symbols. Each label associates
	/// a memory address (in a specific memory type) with a name and optional comment.
	/// </para>
	/// <para>
	/// Labels support multi-byte ranges via the <see cref="Length"/> property, allowing
	/// a single label to cover arrays, structures, or data tables.
	/// </para>
	/// <para>
	/// Serialization format: "MemoryType:Address[-EndAddress]:Label[:Comment]"
	/// Example: "NesPrgRom:8000-80FF:ResetVector:Entry point after reset"
	/// </para>
	/// </remarks>
	public class CodeLabel {
		/// <summary>
		/// Gets or sets the absolute address within the memory type.
		/// </summary>
		public UInt32 Address { get; set; }

		/// <summary>
		/// Gets or sets the memory type this label belongs to (e.g., PRG ROM, RAM, Save RAM).
		/// </summary>
		public MemoryType MemoryType { get; set; }

		/// <summary>
		/// Gets or sets the label name (symbol). Must match the pattern [A-Za-z_@][A-Za-z0-9_@]*.
		/// </summary>
		public string Label { get; set; } = "";

		/// <summary>
		/// Gets or sets the optional comment for this label. Supports multi-line via \n.
		/// </summary>
		public string Comment { get; set; } = "";

		/// <summary>
		/// Gets or sets flags that modify label behavior (e.g., auto-generated, function entry).
		/// </summary>
		public CodeLabelFlags Flags { get; set; }

		/// <summary>
		/// Gets or sets the number of bytes this label covers. Default is 1.
		/// Use for arrays, structures, or data tables that span multiple bytes.
		/// </summary>
		public UInt32 Length { get; set; } = 1;

		/// <summary>
		/// Initializes a new instance of the <see cref="CodeLabel"/> class with default values.
		/// </summary>
		public CodeLabel() {
		}

		/// <summary>
		/// Initializes a new instance of the <see cref="CodeLabel"/> class from an address.
		/// </summary>
		/// <param name="absAddress">The absolute address to create a label for.</param>
		public CodeLabel(AddressInfo absAddress) {
			Address = (uint)absAddress.Address;
			MemoryType = absAddress.Type;
		}

		/// <summary>
		/// Converts this label to its serialization string format.
		/// </summary>
		/// <returns>A string in the format "MemoryType:Address[-EndAddress]:Label[:Comment]".</returns>
		public override string ToString() {
			StringBuilder sb = new StringBuilder();

			sb.Append(MemoryType.ToString());
			sb.Append(":");

			sb.Append(Address.ToString("X4"));
			if (Length > 1) {
				sb.Append("-" + (Address + Length - 1).ToString("X4"));
			}

			sb.Append(":");

			sb.Append(Label);
			if (!string.IsNullOrWhiteSpace(Comment)) {
				sb.Append(":");
				sb.Append(Comment.Replace(Environment.NewLine, "\\n").Replace("\n", "\\n").Replace("\r", "\\n"));
			}

			return sb.ToString();
		}

		/// <summary>Separator character for parsing serialized labels.</summary>
		private static char[] _separator = new char[1] { ':' };

		/// <summary>
		/// Parses a label from its serialized string representation.
		/// </summary>
		/// <param name="data">The serialized label string.</param>
		/// <returns>A <see cref="CodeLabel"/> if parsing succeeds; otherwise, <c>null</c>.</returns>
		/// <remarks>
		/// Expected format: "MemoryType:Address[-EndAddress]:Label[:Comment]"
		/// Supports legacy memory type names for Mesen v1/Mesen-S compatibility.
		/// </remarks>
		public static CodeLabel? FromString(string data) {
			string[] rowData = data.Split(_separator, 4);
			if (rowData.Length < 3) {
				//Invalid row
				return null;
			}

			if (!Enum.TryParse(rowData[0], out MemoryType type)) {
				if (!GetLegacyMemoryType(rowData[0], out type)) {
					//Invalid memory type
					return null;
				}
			}

			string addressString = rowData[1];
			uint address = 0;
			uint length = 1;
			if (addressString.Contains("-")) {
				uint addressEnd;
				string[] addressStartEnd = addressString.Split('-');
				if (UInt32.TryParse(addressStartEnd[0], NumberStyles.HexNumber, CultureInfo.InvariantCulture, out address) &&
					UInt32.TryParse(addressStartEnd[1], NumberStyles.HexNumber, CultureInfo.InvariantCulture, out addressEnd)) {
					if (addressEnd > address) {
						length = addressEnd - address + 1;
					} else {
						//Invalid label (start < end)
						return null;
					}
				} else {
					//Invalid label (can't parse)
					return null;
				}
			} else {
				if (!UInt32.TryParse(rowData[1], NumberStyles.HexNumber, CultureInfo.InvariantCulture, out address)) {
					//Invalid label (can't parse)
					return null;
				}

				length = 1;
			}

			string labelName = rowData[2];
			if (!string.IsNullOrEmpty(labelName) && !LabelManager.LabelRegex.IsMatch(labelName)) {
				//Reject labels that don't respect the label naming restrictions
				return null;
			}

			CodeLabel codeLabel;
			codeLabel = new CodeLabel {
				Address = address,
				MemoryType = type,
				Label = labelName,
				Length = length,
				Comment = ""
			};

			if (rowData.Length > 3) {
				codeLabel.Comment = rowData[3].Replace("\\n", "\n");
			}

			return codeLabel;
		}

		/// <summary>
		/// Checks if this label is accessible by the specified CPU type.
		/// </summary>
		/// <param name="type">The CPU type to check.</param>
		/// <returns><c>true</c> if the CPU can access this label's memory type; otherwise, <c>false</c>.</returns>
		public bool Matches(CpuType type) {
			return type.CanAccessMemoryType(MemoryType);
		}

		/// <summary>
		/// Gets the absolute address information for this label.
		/// </summary>
		/// <returns>An <see cref="AddressInfo"/> with the absolute address and memory type.</returns>
		public AddressInfo GetAbsoluteAddress() {
			return new AddressInfo() { Address = (int)this.Address, Type = this.MemoryType };
		}

		/// <summary>
		/// Gets the relative (CPU-visible) address for this label.
		/// </summary>
		/// <param name="cpuType">The CPU type to get the relative address for.</param>
		/// <returns>The relative address as seen by the specified CPU.</returns>
		/// <remarks>
		/// If the memory type is already relative, returns the absolute address unchanged.
		/// Otherwise, translates through the memory mapper.
		/// </remarks>
		public AddressInfo GetRelativeAddress(CpuType cpuType) {
			if (MemoryType.IsRelativeMemory()) {
				return GetAbsoluteAddress();
			}

			return DebugApi.GetRelativeAddress(GetAbsoluteAddress(), cpuType);
		}

		/// <summary>
		/// Gets the current byte value at this label's address.
		/// </summary>
		/// <returns>The byte value from memory.</returns>
		public byte GetValue() {
			return DebugApi.GetMemoryValue(this.MemoryType, this.Address);
		}

		/// <summary>
		/// Creates a deep copy of this label.
		/// </summary>
		/// <returns>A new <see cref="CodeLabel"/> with the same values.</returns>
		public CodeLabel Clone() {
			return JsonHelper.Clone(this);
		}

		/// <summary>
		/// Translates legacy memory type names from Mesen v1 and Mesen-S to current enum values.
		/// </summary>
		/// <param name="name">The legacy memory type name.</param>
		/// <param name="type">The translated <see cref="MemoryType"/> if successful.</param>
		/// <returns><c>true</c> if translation succeeded; otherwise, <c>false</c>.</returns>
		private static bool GetLegacyMemoryType(string name, out MemoryType type) {
			//For Nexen v1 & Nexen-S compatibility
			type = MemoryType.SnesMemory;
			switch (name) {
				case "REG": type = MemoryType.SnesRegister; break;
				case "PRG": type = MemoryType.SnesPrgRom; break;
				case "SAVE": type = MemoryType.SnesSaveRam; break;
				case "WORK": type = MemoryType.SnesWorkRam; break;
				case "IRAM": type = MemoryType.Sa1InternalRam; break;
				case "SPCRAM": type = MemoryType.SpcRam; break;
				case "SPCROM": type = MemoryType.SpcRom; break;
				case "PSRAM": type = MemoryType.BsxPsRam; break;
				case "MPACK": type = MemoryType.BsxMemoryPack; break;
				case "DSPPRG": type = MemoryType.DspProgramRom; break;
				case "GBPRG": type = MemoryType.GbPrgRom; break;
				case "GBWRAM": type = MemoryType.GbWorkRam; break;
				case "GBSRAM": type = MemoryType.GbCartRam; break;
				case "GBHRAM": type = MemoryType.GbHighRam; break;
				case "GBBOOT": type = MemoryType.GbBootRom; break;
				case "GBREG": type = MemoryType.GameboyMemory; break;

				case "G": type = MemoryType.NesMemory; break;
				case "R": type = MemoryType.NesInternalRam; break;
				case "P": type = MemoryType.NesPrgRom; break;
				case "S": type = MemoryType.NesSaveRam; break;
				case "W": type = MemoryType.NesWorkRam; break;

				default: return false;
			}

			return true;
		}
	}
}
