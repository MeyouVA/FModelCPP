// Tests LoadVirtualPaths and the JSON reader it is built on.
//
// The provider fixture is an in-memory AbstractFileProvider holding hand-written .uproject / .uplugin /
// .upluginmanifest entries — the same three shapes a mounted game presents — so the manifest arm, the
// standalone-descriptor arm, the CanContainContent filter and the manifest-wins-over-uplugin precedence are
// all driven without touching disk. FixPath is then asked to resolve a path through a virtual root, which
// is the only reason VirtualPaths exists.
#include <cstdint>
#include <cstdio>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include "FileProvider/AbstractFileProvider.h"
#include "FileProvider/Objects/GameFile.h"
#include "UE4/Readers/FByteArchive.h"
#include "Utils/Json.h"

using namespace CUE4Parse::FileProvider;
using CUE4Parse::Compression::CompressionMethod;
using CUE4Parse::FileProvider::Objects::FByteBulkDataHeader;
using CUE4Parse::FileProvider::Objects::GameFile;
using CUE4Parse::UE4::Readers::FArchive;
using CUE4Parse::UE4::Readers::FByteArchive;
using CUE4Parse::UE4::VirtualFileSystem::GameFileMap;
namespace Json = CUE4Parse::Utils::Json;

static int g_failures = 0;
#define CHECK(cond) do { if (!(cond)) { std::printf("FAIL: %s (line %d)\n", #cond, __LINE__); ++g_failures; } } while (0)

// A GameFile whose bytes are held in the object — enough for TryCreateReader, which is all the plugin
// scan asks of it.
class MemoryGameFile : public GameFile
{
public:
    MemoryGameFile(const std::string& path, std::string contents)
        : GameFile(path, static_cast<int64_t>(contents.size())), _contents(std::move(contents)) {}

    bool IsEncrypted() const override { return false; }
    CompressionMethod GetCompressionMethod() const override { return CompressionMethod::None; }

    std::vector<uint8_t> Read(const FByteBulkDataHeader*) override
    {
        return std::vector<uint8_t>(_contents.begin(), _contents.end());
    }

    std::unique_ptr<FArchive> CreateReader(const FByteBulkDataHeader*) override
    {
        return std::make_unique<FByteArchive>(Path(), Read(nullptr));
    }

private:
    std::string _contents;
};

// The smallest concrete provider: AbstractFileProvider with the pure virtuals it does not implement itself.
class MemoryFileProvider : public AbstractFileProvider
{
public:
    MemoryFileProvider() : AbstractFileProvider() {}

    void Add(const std::string& path, const std::string& contents)
    {
        GameFileMap map;
        map[path] = std::make_shared<MemoryGameFile>(path, contents);
        Files.AddFiles(std::move(map));
    }
};

static void TestJsonReader()
{
    const auto doc = Json::Parse(R"({
        "Name": "PluginA\/Content",
        "Enabled": true,
        "Disabled": false,
        "Version": 3,
        "Ratio": -1.5e2,
        "Missing": null,
        "List": [1, "two", {"Nested": true}]
    })");
    CHECK(doc.has_value());
    if (!doc.has_value()) return;

    CHECK((*doc)["Name"].AsString() == "PluginA/Content");
    CHECK((*doc)["Enabled"].AsBool());
    CHECK(!(*doc)["Disabled"].AsBool(true));
    CHECK((*doc)["Version"].AsInt() == 3);
    CHECK((*doc)["Ratio"].AsNumber() == -150.0);
    CHECK((*doc)["Missing"].IsNull());
    CHECK((*doc)["List"].Count() == 3);
    CHECK((*doc)["List"][1].AsString() == "two");
    CHECK((*doc)["List"][2]["Nested"].AsBool());

    // Absent keys chain without blowing up, and fall back rather than throw.
    CHECK((*doc)["Nope"]["Deeper"].AsBool(true));
    CHECK((*doc)["Nope"].IsNull());
    CHECK((*doc)["List"][9].IsNull());

    // A UTF-8 BOM (the editor writes one) is skipped, and a surrogate pair folds to one code point.
    const auto bom = Json::Parse("\xEF\xBB\xBF{\"K\":\"\\uD83D\\uDE00\"}");
    CHECK(bom.has_value() && (*bom)["K"].AsString() == "\xF0\x9F\x98\x80");

    // Malformed input is nullopt, not a half-built document.
    CHECK(!Json::Parse("{\"a\": }").has_value());
    CHECK(!Json::Parse("{\"a\": 1}{\"b\": 2}").has_value()); // trailing garbage
    CHECK(!Json::Parse("[1, 2").has_value());
    CHECK(Json::Parse("{}").has_value());
}

static void TestLoadVirtualPaths()
{
    MemoryFileProvider provider;

    // ProjectName comes from the first .uproject key, and the manifest regex is anchored on it.
    provider.Add("MyGame/MyGame.uproject", "{}");
    CHECK(provider.ProjectName() == "MyGame");

    // A manifest listing three plugins: two that can hold content and one that cannot.
    provider.Add("MyGame/Plugins/MyGame.upluginmanifest", R"({
        "Contents": [
            {
                "File": "../../../MyGame/Plugins/GameFeatures/Alpha/Alpha.uplugin",
                "Descriptor": { "CanContainContent": true }
            },
            {
                "File": "../../../MyGame/Plugins/Runtime/Beta/Beta.uplugin",
                "Descriptor": { "CanContainContent": true }
            },
            {
                "File": "../../../MyGame/Plugins/Runtime/CodeOnly/CodeOnly.uplugin",
                "Descriptor": { "CanContainContent": false }
            }
        ]
    })");

    // A standalone .uplugin that the manifest does not mention, plus one it does (Alpha), plus a code-only
    // one. The manifest entry must win for Alpha: the uplugin arm skips a key that already exists.
    provider.Add("MyGame/Plugins/Extra/Gamma/Gamma.uplugin", R"({ "CanContainContent": true })");
    provider.Add("MyGame/Plugins/Extra/Delta/Delta.uplugin", R"({ "CanContainContent": false })");
    provider.Add("MyGame/Plugins/GameFeatures/Alpha/Alpha.uplugin", R"({ "CanContainContent": true })");

    // Neither of these is a plugin file, so neither may reach VirtualPaths.
    provider.Add("MyGame/Content/Thing.uasset", "not json");
    provider.Add("MyGame/AssetRegistry.bin", "not json");

    const int count = provider.LoadVirtualPaths();
    CHECK(count == 3);
    CHECK(static_cast<int>(provider.VirtualPaths.size()) == count);

    // Manifest entries: the virtual root is the file's basename, the value its directory with the
    // "../../../" cook prefix removed.
    CHECK(provider.VirtualPaths["Alpha"] == "MyGame/Plugins/GameFeatures/Alpha");
    CHECK(provider.VirtualPaths["Beta"] == "MyGame/Plugins/Runtime/Beta");
    CHECK(provider.VirtualPaths.count("CodeOnly") == 0); // CanContainContent false

    // The standalone .uplugin arm uses the game file's own directory.
    CHECK(provider.VirtualPaths["Gamma"] == "MyGame/Plugins/Extra/Gamma");
    CHECK(provider.VirtualPaths.count("Delta") == 0); // CanContainContent false

    // FixPath is the consumer: an unknown root that is a virtual path resolves under it, and one that is
    // not is left to the other arms.
    CHECK(provider.FixPath("/Alpha/Maps/Level") == "MyGame/Plugins/GameFeatures/Alpha/Content/Maps/Level.uasset");
    CHECK(provider.FixPath("/Game/Maps/Level") == "MyGame/Content/Maps/Level.uasset");
    CHECK(provider.FixPath("/CodeOnly/Maps/Level") == "CodeOnly/Maps/Level.uasset");

    // A second call rebuilds from scratch rather than accumulating.
    CHECK(provider.LoadVirtualPaths() == 3);

    // A manifest outside "<ProjectName>/Plugins/" fails the regex, and unparseable JSON is skipped instead
    // of throwing — a run over a partly-decrypted game must not abort.
    MemoryFileProvider other;
    other.Add("MyGame/MyGame.uproject", "{}");
    other.Add("Engine/Plugins/Engine.upluginmanifest", R"({"Contents":[
        {"File":"../../../Engine/Plugins/Ignored/Ignored.uplugin","Descriptor":{"CanContainContent":true}}]})");
    other.Add("MyGame/Plugins/Broken/Broken.uplugin", "{ this is not json");
    CHECK(other.LoadVirtualPaths() == 0);
}

int main()
{
    std::printf("=== test_virtual_paths ===\n");
    TestJsonReader();
    TestLoadVirtualPaths();

    if (g_failures == 0) std::printf("ALL PASSED\n");
    else std::printf("%d FAILURE(S)\n", g_failures);
    return g_failures == 0 ? 0 : 1;
}
