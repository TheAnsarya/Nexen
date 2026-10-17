using System;
using System.Collections.Generic;
using System.Linq;
using System.Reactive.Linq;
using System.Text;
using Avalonia.Controls;
using Nexen.Config;
using Nexen.Interop;
using Nexen.Utilities;
using Nexen.Windows;
using ReactiveUI;
using ReactiveUI.SourceGenerators;

namespace Nexen.ViewModels;
public sealed partial class CheatEditWindowViewModel : DisposableViewModel {
	public CheatCode Cheat { get; }

	[ObservableAsProperty] private string _convertedCodes = "";
	[Reactive] public partial bool ShowInvalidCodeHint { get; private set; } = false;
	[Reactive] public partial bool OkButtonEnabled { get; private set; } = false;

	[Reactive] public partial Enum[] AvailableCheatTypes { get; private set; } = [];

	private MainWindowViewModel MainWndModel { get; }

	[Obsolete("For designer only")]
	public CheatEditWindowViewModel() : this(new CheatCode()) { }

	public CheatEditWindowViewModel(CheatCode cheat) {
		Cheat = cheat;
		MainWndModel = MainWindowViewModel.Instance;

		AddDisposable(_convertedCodesHelper = this.WhenAnyValue(x => x.Cheat.Codes, x => x.Cheat.Type).Select(x => {
			string[] codes = cheat.Codes.Split(Environment.NewLine);
			StringBuilder sb = new StringBuilder();
			bool hasInvalidCode = false;
			bool hasValidCode = false;
			foreach (string codeString in codes) {
				if (sb.Length > 0) {
					sb.Append(Environment.NewLine);
				}

				InteropInternalCheatCode code = new();
				if (EmuApi.GetConvertedCheat(new InteropCheatCode(cheat.Type, codeString.Trim()), ref code)) {
					if (code.IsAbsoluteAddress) {
						sb.Append(code.MemType.GetShortName() + " ");
					}

					if (code.Compare >= 0) {
						sb.Append($"{code.Address:X2}:{code.Value:X2}:{code.Compare:X2}");
					} else {
						sb.Append($"{code.Address:X2}:{code.Value:X2}");
					}

					hasValidCode = true;
				} else {
					if (!string.IsNullOrWhiteSpace(codeString)) {
						hasInvalidCode = true;
						sb.Append("[invalid code]");
					}
				}
			}

			ShowInvalidCodeHint = hasInvalidCode;
			OkButtonEnabled = hasValidCode && !hasInvalidCode;

			return sb.ToString();
		}).ToProperty(this, _ => _.ConvertedCodes));

		if (Design.IsDesignMode) {
			return;
		}

		AddDisposable(this.WhenAnyValue(x => x.MainWndModel.RomInfo).Subscribe(romInfo => {
			Enum[] compatibleCheatTypes = Enum.GetValues<CheatType>().Where(e => romInfo.CpuTypes.Contains(e.ToCpuType())).Cast<Enum>().ToArray();
			AvailableCheatTypes = compatibleCheatTypes;

			if (compatibleCheatTypes.Length == 0) {
				OkButtonEnabled = false;
				return;
			}

			if (!compatibleCheatTypes.Contains(Cheat.Type)) {
				Cheat.Type = (CheatType)compatibleCheatTypes[0];
			}
		}));
	}
}
