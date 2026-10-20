using Mesen.Config;
using Mesen.Debugger.Utilities;
using ReactiveUI;
using ReactiveUI.Fody.Helpers;
using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Linq;
using System.Reactive.Linq;
using System.Windows.Input;

namespace Mesen.ViewModels
{
	/// <summary>Reads and writes a KeyMapping field by name, without reflection (AOT-safe).</summary>
	public static class KeyMappingAccess
	{
		public static UInt16 Get(KeyMapping m, string name)
		{
			return name switch {
				"A" => m.A, "B" => m.B, "X" => m.X, "Y" => m.Y, "L" => m.L, "R" => m.R,
				"Up" => m.Up, "Down" => m.Down, "Left" => m.Left, "Right" => m.Right,
				"Start" => m.Start, "Select" => m.Select, "U" => m.U, "D" => m.D,
				"GenericKey1" => m.GenericKey1,
				"TurboA" => m.TurboA, "TurboB" => m.TurboB, "TurboX" => m.TurboX,
				"TurboY" => m.TurboY, "TurboL" => m.TurboL, "TurboR" => m.TurboR,
				_ => 0
			};
		}

		public static void Set(KeyMapping m, string name, UInt16 value)
		{
			switch(name) {
				case "A": m.A = value; break;
				case "B": m.B = value; break;
				case "X": m.X = value; break;
				case "Y": m.Y = value; break;
				case "L": m.L = value; break;
				case "R": m.R = value; break;
				case "Up": m.Up = value; break;
				case "Down": m.Down = value; break;
				case "Left": m.Left = value; break;
				case "Right": m.Right = value; break;
				case "Start": m.Start = value; break;
				case "Select": m.Select = value; break;
				case "U": m.U = value; break;
				case "D": m.D = value; break;
				case "GenericKey1": m.GenericKey1 = value; break;
				case "TurboA": m.TurboA = value; break;
				case "TurboB": m.TurboB = value; break;
				case "TurboX": m.TurboX = value; break;
				case "TurboY": m.TurboY = value; break;
				case "TurboL": m.TurboL = value; break;
				case "TurboR": m.TurboR = value; break;
			}
		}
	}

	/// <summary>One input of a button, backed by a single field in one of the key sets.</summary>
	public class BindingSlotViewModel : ViewModelBase, IDisposable
	{
		private readonly KeyMapping _mapping;
		private readonly PropertyChangedEventHandler _mappingChanged;
		private readonly IDisposable _keySubscription;

		[Reactive] public UInt16 Key { get; set; }
		[Reactive] public bool IsBound { get; set; }
		[Reactive] public bool IsVisible { get; set; }

		public ICommand ClearCommand { get; }

		public BindingSlotViewModel(KeyMapping mapping, string property)
		{
			_mapping = mapping;
			Key = KeyMappingAccess.Get(mapping, property);
			ClearCommand = new SimpleCommand(() => Key = 0);

			//Keep in sync when presets, clear or the key set pages change the same field
			_mappingChanged = (s, e) => {
				if(e.PropertyName == property) {
					UInt16 value = KeyMappingAccess.Get(mapping, property);
					if(Key != value) {
						Key = value;
					}
				}
			};
			mapping.PropertyChanged += _mappingChanged;

			_keySubscription = this.WhenAnyValue(x => x.Key).Subscribe(value => {
				IsBound = value != 0;
				if(KeyMappingAccess.Get(mapping, property) != value) {
					KeyMappingAccess.Set(mapping, property, value);
				}
			});
		}

		public void Dispose()
		{
			_mapping.PropertyChanged -= _mappingChanged;
			_keySubscription.Dispose();
		}
	}

	/// <summary>The inputs for one button (or its turbo version), one slot per key set.</summary>
	public class BindingListViewModel : ViewModelBase, IDisposable
	{
		public static BindingListViewModel Empty => new(Array.Empty<KeyMapping>(), "");

		private readonly List<IDisposable> _subscriptions = new();

		public List<BindingSlotViewModel> Slots { get; }

		public BindingListViewModel(IEnumerable<KeyMapping> keySets, string property)
		{
			Slots = keySets.Select(set => new BindingSlotViewModel(set, property)).ToList();

			foreach(BindingSlotViewModel slot in Slots) {
				_subscriptions.Add(slot.WhenAnyValue(x => x.Key).Subscribe(_ => RefreshVisibility()));
			}
			RefreshVisibility();
		}

		private void RefreshVisibility()
		{
			//Show every bound input plus a single empty slot to add another one
			bool emptyShown = false;
			foreach(BindingSlotViewModel slot in Slots) {
				if(slot.Key != 0) {
					slot.IsVisible = true;
				} else {
					slot.IsVisible = !emptyShown;
					emptyShown = true;
				}
			}
		}

		public void Dispose()
		{
			foreach(IDisposable subscription in _subscriptions) {
				subscription.Dispose();
			}
			foreach(BindingSlotViewModel slot in Slots) {
				slot.Dispose();
			}
		}
	}

	public class ButtonBindingsRowViewModel : ViewModelBase
	{
		public string Label { get; }
		public bool HasTurbo { get; }
		public BindingListViewModel Inputs { get; }
		public BindingListViewModel TurboInputs { get; }

		public ButtonBindingsRowViewModel(ControllerButtonDef button, KeyMapping[] keySets)
		{
			Label = button.Label;
			HasTurbo = button.HasTurbo;
			Inputs = new BindingListViewModel(keySets, button.Name);
			TurboInputs = button.HasTurbo ? new BindingListViewModel(keySets, "Turbo" + button.Name) : BindingListViewModel.Empty;
		}
	}
}
