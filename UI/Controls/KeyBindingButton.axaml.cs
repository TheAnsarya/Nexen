using Avalonia;
using Avalonia.Controls;
using Avalonia.Input;
using Avalonia.Markup.Xaml;
using Avalonia.Media;
using Avalonia.Styling;
using Avalonia.VisualTree;
using Mesen.Interop;
using Mesen.Config;
using Mesen.Localization;
using Mesen.Utilities;
using Mesen.ViewModels;
using Mesen.Windows;
using System;
using System.ComponentModel;
using System.Linq;
using System.Threading.Tasks;

namespace Mesen.Controls
{
	public class KeyBindingButton : Button
	{
		protected override Type StyleKeyOverride => typeof(Button);

		public static readonly StyledProperty<UInt16> KeyBindingProperty = AvaloniaProperty.Register<KeyBindingButton, UInt16>(nameof(KeyBinding), 0, false, Avalonia.Data.BindingMode.TwoWay);

		public UInt16 KeyBinding
		{
			get { return GetValue(KeyBindingProperty); }
			set { SetValue(KeyBindingProperty, value); }
		}

		public static readonly StyledProperty<string> EmptyTextProperty = AvaloniaProperty.Register<KeyBindingButton, string>(nameof(EmptyText), "");

		//Text shown while no input is bound
		public string EmptyText
		{
			get { return GetValue(EmptyTextProperty); }
			set { SetValue(EmptyTextProperty, value); }
		}

		public static readonly StyledProperty<bool> EnableMultiBindingProperty = AvaloniaProperty.Register<KeyBindingButton, bool>(nameof(EnableMultiBinding), true);

		//When true, shift+click opens a menu to bind several inputs to the same controller button
		public bool EnableMultiBinding
		{
			get { return GetValue(EnableMultiBindingProperty); }
			set { SetValue(EnableMultiBindingProperty, value); }
		}

		private const UInt16 ProbeValue = 0xFFFF;

		private bool _shiftOnPress;
		private bool _probed;
		private KeyMapping[]? _keySets;
		private KeyMapping? _ownerSet;
		private string? _property;
		private int _extraBindings;
		private PropertyChangedEventHandler? _setChangedHandler;

		public KeyBindingButton()
		{
			InitializeComponent();
			Loaded += (s, e) => ProbeBinding();
		}

		private void InitializeComponent()
		{
			AvaloniaXamlLoader.Load(this);
		}

		protected override void OnDataContextChanged(EventArgs e)
		{
			base.OnDataContextChanged(e);
			SetKeyName(this.KeyBinding);
		}

		protected override void OnKeyUp(KeyEventArgs e)
		{
			if(e.Key == Key.Space) {
				//Prevent using space to open up the configuration dialog
				//Otherwise the dialog opens up again when the user is trying
				//to bind the space key to a button
				e.Handled = true;
			} else {
				base.OnKeyUp(e);
			}
		}

		protected override void OnAttachedToVisualTree(VisualTreeAttachmentEventArgs e)
		{
			base.OnAttachedToVisualTree(e);
			if(_keySets != null) {
				SubscribeToKeySets();
				UpdateExtraBindings();
			}
		}

		protected override void OnDetachedFromVisualTree(VisualTreeAttachmentEventArgs e)
		{
			base.OnDetachedFromVisualTree(e);
			UnsubscribeFromKeySets();
		}

		//Finds which key mapping field this button edits by writing a marker value and watching which field changes
		private void ProbeBinding()
		{
			if(_probed) {
				return;
			}
			_probed = true;

			if(!EnableMultiBinding || this.GetVisualRoot() is not Window { DataContext: ControllerConfigViewModel model }) {
				return;
			}

			KeyMapping[] keySets = { model.Config.Mapping1, model.Config.Mapping2, model.Config.Mapping3, model.Config.Mapping4 };
			string? property = null;
			KeyMapping? owner = null;
			PropertyChangedEventHandler probe = (s, e) => {
				if(property == null) {
					property = e.PropertyName;
					owner = s as KeyMapping;
				}
			};

			foreach(KeyMapping set in keySets) {
				set.PropertyChanged += probe;
			}

			UInt16 original = KeyBinding;
			try {
				KeyBinding = ProbeValue;
			} finally {
				KeyBinding = original;
				foreach(KeyMapping set in keySets) {
					set.PropertyChanged -= probe;
				}
			}

			if(property != null && owner != null) {
				_keySets = keySets;
				_ownerSet = owner;
				_property = property;
				SubscribeToKeySets();
				UpdateExtraBindings();
			}
		}

		private void SubscribeToKeySets()
		{
			_setChangedHandler ??= (s, e) => {
				if(e.PropertyName == _property) {
					UpdateExtraBindings();
				}
			};

			foreach(KeyMapping set in _keySets!) {
				set.PropertyChanged -= _setChangedHandler;
				set.PropertyChanged += _setChangedHandler;
			}
		}

		private void UnsubscribeFromKeySets()
		{
			if(_keySets != null && _setChangedHandler != null) {
				foreach(KeyMapping set in _keySets) {
					set.PropertyChanged -= _setChangedHandler;
				}
			}
		}

		private void UpdateExtraBindings()
		{
			_extraBindings = _keySets!.Count(set => set != _ownerSet && KeyMappingAccess.Get(set, _property!) != 0);
			SetKeyName(KeyBinding);
		}

		protected override void OnPointerPressed(PointerPressedEventArgs e)
		{
			_shiftOnPress = e.KeyModifiers.HasFlag(KeyModifiers.Shift);
			base.OnPointerPressed(e);
		}

		private static string FormatPropertyName(string property)
		{
			return property.StartsWith("Turbo") ? "Turbo " + property.Substring(5) : property;
		}

		private async Task ShowBindingsWindow()
		{
			BindingListViewModel bindings = new(_keySets!, _property!);
			ButtonBindingsWindow wnd = new(FormatPropertyName(_property!));
			wnd.DataContext = bindings;
			try {
				await wnd.ShowCenteredDialog(this.GetVisualRoot() as Visual);
			} finally {
				bindings.Dispose();
			}
		}

		protected override async void OnClick()
		{
			bool multiBinding = _shiftOnPress && EnableMultiBinding;
			_shiftOnPress = false;

			if(multiBinding) {
				ProbeBinding();
				if(_keySets != null && _property != null) {
					await ShowBindingsWindow();
					return;
				}
			}

			GetKeyWindow wnd = new GetKeyWindow(false);
			wnd.SingleKeyMode = true;
			wnd.WindowStartupLocation = WindowStartupLocation.CenterOwner;
			await wnd.ShowCenteredDialog(this.GetVisualRoot() as Visual);
			this.KeyBinding = wnd.ShortcutKey.Key1;
		}

		protected override void OnPointerReleased(PointerReleasedEventArgs e)
		{
			base.OnPointerReleased(e);

			//Allow using right mouse button to clear bindings
			if(e.InitialPressMouseButton == MouseButton.Right) {
				this.KeyBinding = 0;
			}
		}

		protected override void OnPropertyChanged(AvaloniaPropertyChangedEventArgs change)
		{
			base.OnPropertyChanged(change);

			if(change.Property == KeyBindingProperty) {
				UInt16 value = Convert.ToUInt16(change.NewValue);
				SetKeyName(value);
			} else if(change.Property == EmptyTextProperty) {
				SetKeyName(KeyBinding);
			}
		}

		private void SetKeyName(ushort value)
		{
			string keyname = InputApi.GetKeyName(value);
			string text = string.IsNullOrEmpty(keyname) ? EmptyText : keyname;
			if(_extraBindings > 0) {
				text = (text.Length > 0 ? text + " " : "") + "+" + _extraBindings;
			}
			this.Content = text;
			KeyBindingButton.SetBindingButtonTooltip(this, keyname, _keySets != null);
		}

		public static void SetBindingButtonTooltip(Button btn, string keyname, bool multiBinding = false)
		{
			if(string.IsNullOrWhiteSpace(keyname) && !multiBinding) {
				ToolTip.SetTip(btn, null);
			} else {
				StackPanel pnl = new();
				if(!string.IsNullOrWhiteSpace(keyname)) {
					pnl.Children.Add(new TextBlock() { Text = keyname });
					pnl.Children.Add(new TextBlock() { Text = ResourceHelper.GetMessage("RightClickToClearBinding"), FontStyle = FontStyle.Italic, Margin = new(0, 10, 0, 0) });
				}
				if(multiBinding) {
					pnl.Children.Add(new TextBlock() { Text = ResourceHelper.GetMessage("ShiftClickForMultipleBindings"), FontStyle = FontStyle.Italic, Margin = new(0, string.IsNullOrWhiteSpace(keyname) ? 0 : 5, 0, 0) });
				}
				ToolTip.SetTip(btn, pnl);
			}
		}
	}
}
