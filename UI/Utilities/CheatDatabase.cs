using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Xml.Serialization;

namespace Nexen.Utilities; 
[XmlRoot("cheats")]
public sealed class CheatDatabase {
	[XmlElement("game")]
	public List<CheatDbGameEntry> Games { get; set; } = null!;
}

[XmlType("game")]
public sealed class CheatDbGameEntry {
	[XmlAttribute("name")]
	public string Name { get; set; } = null!;

	[XmlAttribute("sha1")]
	public string Sha1 { get; set; } = null!;

	[XmlElement("cheat")]
	public List<CheatDbCheatEntry> Cheats { get; set; } = null!;

	public override string ToString() {
		return this.Name;
	}
}

public sealed class CheatDbCheatEntry {
	[XmlAttribute("desc")]
	public string Desc { get; set; } = null!;

	[XmlAttribute("code")]
	public string Code { get; set; } = null!;
}
