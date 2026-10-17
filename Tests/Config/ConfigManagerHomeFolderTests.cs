using Nexen.Config;
using Xunit;

namespace Nexen.Tests.Config;

public sealed class ConfigManagerHomeFolderTests {
	[Fact]
	public void ResolveHomeFolderPath_NoPortableSettings_UsesDocumentsFolder() {
		string root = Path.Combine(Path.GetTempPath(), "nexen-tests", Guid.NewGuid().ToString("N"));
		string portable = Path.Combine(root, "portable");
		string documents = Path.Combine(root, "documents");
		Directory.CreateDirectory(portable);
		Directory.CreateDirectory(documents);

		string result = ConfigManager.ResolveHomeFolderPath(
			portable,
			documents,
			_ => true,
			(_, _) => true
		);

		Assert.Equal(documents, result);
	}

	[Fact]
	public void ResolveHomeFolderPath_PortableSettingsAndWritable_UsesPortableFolder() {
		string root = Path.Combine(Path.GetTempPath(), "nexen-tests", Guid.NewGuid().ToString("N"));
		string portable = Path.Combine(root, "portable");
		string documents = Path.Combine(root, "documents");
		Directory.CreateDirectory(portable);
		Directory.CreateDirectory(documents);
		File.WriteAllText(Path.Combine(portable, "settings.json"), "{}");

		string result = ConfigManager.ResolveHomeFolderPath(
			portable,
			documents,
			folder => folder == portable,
			(_, _) => true
		);

		Assert.Equal(portable, result);
	}

	[Fact]
	public void ResolveHomeFolderPath_PortableReadOnlyAndDocumentsWritable_MigratesAndUsesDocuments() {
		string root = Path.Combine(Path.GetTempPath(), "nexen-tests", Guid.NewGuid().ToString("N"));
		string portable = Path.Combine(root, "portable");
		string documents = Path.Combine(root, "documents");
		Directory.CreateDirectory(portable);
		Directory.CreateDirectory(documents);
		string portableConfig = Path.Combine(portable, "settings.json");
		File.WriteAllText(portableConfig, "{}");

		bool migrateCalled = false;
		string result = ConfigManager.ResolveHomeFolderPath(
			portable,
			documents,
			folder => folder == documents,
			(source, destination) => {
				migrateCalled = source == portableConfig && destination == Path.Combine(documents, "settings.json");
				return true;
			}
		);

		Assert.True(migrateCalled);
		Assert.Equal(documents, result);
	}

	[Fact]
	public void ResolveHomeFolderPath_PortableReadOnlyAndDocumentsAlreadyHasSettings_DoesNotMigrate() {
		string root = Path.Combine(Path.GetTempPath(), "nexen-tests", Guid.NewGuid().ToString("N"));
		string portable = Path.Combine(root, "portable");
		string documents = Path.Combine(root, "documents");
		Directory.CreateDirectory(portable);
		Directory.CreateDirectory(documents);
		File.WriteAllText(Path.Combine(portable, "settings.json"), "{\"portable\":true}");
		File.WriteAllText(Path.Combine(documents, "settings.json"), "{\"documents\":true}");

		bool migrateCalled = false;
		string result = ConfigManager.ResolveHomeFolderPath(
			portable,
			documents,
			folder => folder == documents,
			(_, _) => {
				migrateCalled = true;
				return true;
			}
		);

		Assert.False(migrateCalled);
		Assert.Equal(documents, result);
	}

	[Fact]
	public void ResolveHomeFolderPath_BothFoldersReadOnly_FallsBackToPortable() {
		string root = Path.Combine(Path.GetTempPath(), "nexen-tests", Guid.NewGuid().ToString("N"));
		string portable = Path.Combine(root, "portable");
		string documents = Path.Combine(root, "documents");
		Directory.CreateDirectory(portable);
		Directory.CreateDirectory(documents);
		File.WriteAllText(Path.Combine(portable, "settings.json"), "{}");

		string result = ConfigManager.ResolveHomeFolderPath(
			portable,
			documents,
			_ => false,
			(_, _) => false
		);

		Assert.Equal(portable, result);
	}
}
