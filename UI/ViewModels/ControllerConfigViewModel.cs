using System;
using System.Collections.Generic;
using System.Linq;
using System.Reactive.Linq;
using System.Text;
using System.Threading.Tasks;
using Nexen.Config;
using ReactiveUI;
using ReactiveUI.SourceGenerators;

namespace Nexen.ViewModels; 
public partial class ControllerConfigViewModel : ViewModelBase {
	public ControllerConfig Config { get; }
	public ControllerConfig OriginalConfig { get; }
	public ControllerType Type { get; }

	[Reactive] public partial KeyMappingViewModel KeyMapping1 { get; set; }
	[Reactive] public partial KeyMappingViewModel KeyMapping2 { get; set; }
	[Reactive] public partial KeyMappingViewModel KeyMapping3 { get; set; }
	[Reactive] public partial KeyMappingViewModel KeyMapping4 { get; set; }

	[Reactive] public partial bool ShowPresets { get; set; } = false;
	[Reactive] public partial bool IsTwoButtonController { get; set; } = false;
	[Reactive] public partial bool ShowTurbo { get; set; } = false;

	[Obsolete("For designer only")]
	public ControllerConfigViewModel() : this(ControllerType.SnesController, new ControllerConfig(), new ControllerConfig(), 0) { }

	public ControllerConfigViewModel(ControllerType type, ControllerConfig config, ControllerConfig originalConfig, int port) {
		Config = config;
		OriginalConfig = originalConfig;
		Type = type;

		KeyMapping1 = new KeyMappingViewModel(type, config.Mapping1, 0, port);
		KeyMapping2 = new KeyMappingViewModel(type, config.Mapping2, 1, port);
		KeyMapping3 = new KeyMappingViewModel(type, config.Mapping3, 2, port);
		KeyMapping4 = new KeyMappingViewModel(type, config.Mapping4, 3, port);

		ShowPresets = type.HasPresets();
		IsTwoButtonController = type.IsTwoButtonController();
		ShowTurbo = type.HasTurbo();
	}
}

public partial class KeyMappingViewModel : ViewModelBase {
	[Reactive] public partial ControllerType Type { get; set; }
	[Reactive] public partial KeyMapping Mapping { get; set; }
	[Reactive] public partial int Port { get; set; }
	[Reactive] public partial List<CustomKeyMapping> CustomKeys { get; set; } = new();

	private int _mappingIndex = 0;

	[Obsolete("For designer only")]
	public KeyMappingViewModel() : this(ControllerType.None, new(), 0, 0) { }

	public KeyMappingViewModel(ControllerType type, KeyMapping mapping, int mappingIndex, int port) {
		Type = type;
		Mapping = mapping;
		Port = port;
		_mappingIndex = mappingIndex;

		CustomKeys = Mapping.ToCustomKeys(type, _mappingIndex);
	}

	public void RefreshCustomKeys() {
		CustomKeys = Mapping.ToCustomKeys(Type, _mappingIndex);
	}
}

public sealed partial class CustomKeyMapping : ViewModelBase {
	public string Name { get; set; }
	public UInt16[] Mappings { get; set; }
	public int Index { get; set; }
	[Reactive] public partial UInt16 KeyMapping { get; set; }

	public CustomKeyMapping(string name, UInt16[] mappings, int index) {
		Name = name;
		Mappings = mappings;
		Index = index;
		KeyMapping = mappings[index];

		this.WhenAnyValue(x => x.KeyMapping).Subscribe(x => mappings[index] = x);
	}
}
