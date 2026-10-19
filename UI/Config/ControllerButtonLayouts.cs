using System.Collections.Generic;
using System.Linq;

namespace Mesen.Config
{
	/// <summary>An emulated controller button that can be bound to several physical inputs.</summary>
	/// <param name="Name">Name of the matching KeyMapping property.</param>
	/// <param name="HasTurbo">True when the core supports a "Turbo" + Name mapping for this button.</param>
	public record ControllerButtonDef(string Name, string Label, bool HasTurbo = false);

	public static class ControllerButtonLayouts
	{
		private static ControllerButtonDef Btn(string name, string label, bool turbo = false)
		{
			return new ControllerButtonDef(name, label, turbo);
		}

		private static List<ControllerButtonDef> Standard(ControllerButtonDef[] face, ControllerButtonDef[] system)
		{
			List<ControllerButtonDef> buttons = new(face);
			buttons.AddRange(new[] { Btn("Up", "Up"), Btn("Down", "Down"), Btn("Left", "Left"), Btn("Right", "Right") });
			buttons.AddRange(system);
			return buttons;
		}

		/// <summary>Returns the buttons for controllers that use the standard key mapping fields, or null if the controller needs the key set pages.</summary>
		public static IReadOnlyList<ControllerButtonDef>? Get(ControllerType type)
		{
			ControllerButtonDef[] selectStart = { Btn("Select", "Select"), Btn("Start", "Start") };
			ControllerButtonDef[] twoButtons = { Btn("A", "A", true), Btn("B", "B", true) };

			switch(type) {
				case ControllerType.SnesController:
				case ControllerType.SnesRumbleController:
					return Standard(
						new[] { Btn("A", "A", true), Btn("B", "B", true), Btn("X", "X", true), Btn("Y", "Y", true), Btn("L", "L", true), Btn("R", "R", true) },
						selectStart
					);

				case ControllerType.NesController:
				case ControllerType.FamicomController:
				case ControllerType.HoriTrack:
				case ControllerType.GameboyController:
					return Standard(twoButtons, selectStart);

				case ControllerType.FamicomControllerP2:
					return Standard(twoButtons, new[] { Btn("Select", "Select"), Btn("Start", "Start"), Btn("GenericKey1", "Microphone") });

				case ControllerType.GbaController:
					return Standard(
						new[] { Btn("A", "A", true), Btn("B", "B", true), Btn("L", "L", true), Btn("R", "R", true) },
						selectStart
					);

				case ControllerType.PceController:
					return Standard(
						new[] { Btn("A", "I", true), Btn("B", "II", true) },
						new[] { Btn("Select", "Select"), Btn("Start", "Run") }
					);

				case ControllerType.PceAvenuePad6:
					return Standard(
						new[] { Btn("A", "I", true), Btn("B", "II", true), Btn("X", "III", true), Btn("Y", "IV", true), Btn("L", "V", true), Btn("R", "VI", true) },
						new[] { Btn("Select", "Select"), Btn("Start", "Run") }
					);

				case ControllerType.SmsController:
					return Standard(twoButtons, new[] { Btn("Start", "Pause / Start") });

				case ControllerType.WsController:
				case ControllerType.WsControllerVertical:
					return new List<ControllerButtonDef>() {
						Btn("A", "A", true), Btn("B", "B", true),
						Btn("Up", "D-Pad 1 Up"), Btn("Down", "D-Pad 1 Down"), Btn("Left", "D-Pad 1 Left"), Btn("Right", "D-Pad 1 Right"),
						Btn("U", "D-Pad 2 Up"), Btn("D", "D-Pad 2 Down"), Btn("L", "D-Pad 2 Left"), Btn("R", "D-Pad 2 Right"),
						Btn("Start", "Start"), Btn("GenericKey1", "Sound")
					};
			}

			return null;
		}
	}
}
