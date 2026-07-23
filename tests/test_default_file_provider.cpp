// Tests for the FileProvider layer: FileProviderDictionary (read-order precedence + payload discovery),
// AbstractFileProvider (ProjectName / FixPath / lookups), AbstractVfsFileProvider (the register -> submit
// key -> mount lifecycle over real in-memory paks) and DefaultFileProvider (the on-disk directory scan).
//
// Container fixtures are real version-8 paks authored byte-for-byte, mounted through the real
// PakFileReader — the same approach as test_pak.cpp, which owns the deeper format coverage. Encryption is
// exercised through the CustomEncryption delegate (a byte-wise XOR), because the port ships no encryptor;
// the wrong-real-AES-key path is still driven end-to-end to prove SubmitKey rejects instead of mounting
// garbage.
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include "Encryption/Aes/FAesKey.h"
#include "FileProvider/DefaultFileProvider.h"
#include "FileProvider/Objects/OsGameFile.h"
#include "FileProvider/Vfs/FileProviderDictionary.h"
#include "UE4/Objects/Core/Misc/FGuid.h"
#include "UE4/Pak/PakFileReader.h"
#include "UE4/Readers/FByteArchive.h"
#include "UE4/VirtualFileSystem/VfsEntry.h"

namespace fs = std::filesystem;
using namespace CUE4Parse::FileProvider;
using namespace CUE4Parse::FileProvider::Vfs;
using CUE4Parse::Encryption::Aes::FAesKey;
using CUE4Parse::FileProvider::Objects::GameFile;
using CUE4Parse::FileProvider::Objects::OsGameFile;
using CUE4Parse::UE4::Objects::Core::Misc::FGuid;
using CUE4Parse::UE4::Pak::PakFileReader;
using CUE4Parse::UE4::Readers::FArchive;
using CUE4Parse::UE4::Readers::FByteArchive;
using CUE4Parse::UE4::Versions::VersionContainer;
using CUE4Parse::UE4::VirtualFileSystem::GameFileMap;
using CUE4Parse::UE4::VirtualFileSystem::IAesVfsReader;
using CUE4Parse::UE4::VirtualFileSystem::IVfsReader;
using CUE4Parse::UE4::VirtualFileSystem::VfsEntry;
using CUE4Parse::Utils::StringComparer;

static int g_failures = 0;
#define CHECK(cond) do { if (!(cond)) { std::printf("FAIL: %s (line %d)\n", #cond, __LINE__); ++g_failures; } } while (0)

// --------------------------------------------------------------------------------------------------
// Fixture types
// --------------------------------------------------------------------------------------------------

// A GameFile that serves fixed bytes — enough for the dictionary and path tests.
class FakeGameFile : public GameFile
{
public:
    explicit FakeGameFile(std::string path, std::string content = "")
        : GameFile(std::move(path), static_cast<int64_t>(content.size())), _content(std::move(content)) {}

    bool IsEncrypted() const override { return false; }
    CUE4Parse::Compression::CompressionMethod GetCompressionMethod() const override
    { return CUE4Parse::Compression::CompressionMethod::None; }

    std::vector<uint8_t> Read() override { return {_content.begin(), _content.end()}; }
    std::unique_ptr<FArchive> CreateReader() override
    { return std::make_unique<FByteArchive>(Path(), Read()); }

private:
    std::string _content;
};

// The minimum IVfsReader for FindPayloads' same-archive preference.
class FakeVfsReader : public virtual IVfsReader
{
public:
    std::string PathName = "fake.pak";
    GameFileMap FileMap{StringComparer::OrdinalIgnoreCase()};
    std::string Mounted;
    VersionContainer Versions;

    const std::string& Path() const override { return PathName; }
    const std::string& Name() const override { return PathName; }
    int64_t ReadOrder() const override { return 3; }
    const GameFileMap& Files() const override { return FileMap; }
    const std::string& MountPoint() const override { return Mounted; }
    bool HasDirectoryIndex() const override { return true; }
    bool IsConcurrent() const override { return false; }
    void SetConcurrent(bool) override {}
    VersionContainer& GetVersions() override { return Versions; }
    CUE4Parse::UE4::Versions::EGame Game() const override { return Versions.Game(); }
    CUE4Parse::UE4::Versions::FPackageFileVersion Ver() const override { return Versions.Ver(); }
    void Mount(const StringComparer&) override {}
    std::vector<uint8_t> Extract(VfsEntry&) override { return {}; }
};

class FakeVfsEntry : public VfsEntry
{
public:
    FakeVfsEntry(IVfsReader* vfs, std::string path, std::string content)
        : VfsEntry(vfs, std::move(path), static_cast<int64_t>(content.size())), _content(std::move(content)) {}

    bool IsEncrypted() const override { return false; }
    CUE4Parse::Compression::CompressionMethod GetCompressionMethod() const override
    { return CUE4Parse::Compression::CompressionMethod::None; }
    std::vector<uint8_t> Read() override { return {_content.begin(), _content.end()}; }
    std::unique_ptr<FArchive> CreateReader() override
    { return std::make_unique<FByteArchive>(Path(), Read()); }

private:
    std::string _content;
};

// AbstractVfsFileProvider with the constructor opened up and no directory scan.
class TestProvider : public AbstractVfsFileProvider
{
public:
    explicit TestProvider(StringComparer pathComparer = StringComparer::OrdinalIgnoreCase(),
                          VersionContainer versions = VersionContainer())
        : AbstractVfsFileProvider(std::move(versions), pathComparer) {}
    void Initialize() override {}
};

// --------------------------------------------------------------------------------------------------
// Minimal legacy (version 8) pak writer — the uncompressed subset of test_pak.cpp's writer.
// --------------------------------------------------------------------------------------------------
struct Writer
{
    std::vector<uint8_t> Bytes;

    int64_t Pos() const { return static_cast<int64_t>(Bytes.size()); }

    template <typename T>
    void Put(T value)
    {
        const auto* p = reinterpret_cast<const uint8_t*>(&value);
        Bytes.insert(Bytes.end(), p, p + sizeof(T));
    }

    void Raw(const std::string& data) { Bytes.insert(Bytes.end(), data.begin(), data.end()); }
    void Raw(const std::vector<uint8_t>& data) { Bytes.insert(Bytes.end(), data.begin(), data.end()); }
    void Zeros(int64_t count) { Bytes.insert(Bytes.end(), static_cast<size_t>(count), 0); }

    void FString(const std::string& s)
    {
        Put<int32_t>(static_cast<int32_t>(s.size() + 1));
        Raw(s);
        Put<uint8_t>(0);
    }
};

// A stored (uncompressed, unencrypted) legacy record is always 53 bytes.
static constexpr int32_t kStoredStructSize = 8 + 8 + 8 + 4 + 20 + 1 + 4;

static void WriteStoredRecord(Writer& w, int64_t offset, int64_t size)
{
    w.Put<int64_t>(offset);
    w.Put<int64_t>(size); // compressed
    w.Put<int64_t>(size); // uncompressed
    w.Put<int32_t>(0);    // method index: stored
    w.Zeros(20);          // hash
    w.Put<uint8_t>(0);    // flags
    w.Put<uint32_t>(0);   // compression block size
}

static void WriteTrailer(Writer& w, int64_t indexOffset, int64_t indexSize, bool encryptedIndex)
{
    const int64_t start = w.Pos();
    w.Zeros(16); // EncryptionKeyGuid
    w.Put<uint8_t>(encryptedIndex ? 1 : 0);
    w.Put<uint32_t>(CUE4Parse::UE4::Pak::Objects::FPakInfo::PAK_FILE_MAGIC);
    w.Put<int32_t>(8);
    w.Put<int64_t>(indexOffset);
    w.Put<int64_t>(indexSize);
    w.Zeros(20); // IndexHash
    for (int i = 0; i < 5; ++i) w.Zeros(32); // empty compression-method names
    CHECK(w.Pos() - start == 221);
}

// A version-8 pak of stored files under "../../../Game/". When xorMask is nonzero the index region is
// XOR-"encrypted" (padded to 16 bytes so the real-AES rejection path stays legal too).
static std::vector<uint8_t> MakePak(const std::vector<std::pair<std::string, std::string>>& files,
                                    uint8_t xorMask = 0)
{
    Writer file;
    std::vector<int64_t> offsets;
    for (const auto& [path, content] : files)
    {
        offsets.push_back(file.Pos());
        file.Zeros(kStoredStructSize); // the duplicated record in front of the payload
        file.Raw(content);
    }

    Writer index;
    index.FString("../../../Game/");
    index.Put<int32_t>(static_cast<int32_t>(files.size()));
    for (size_t i = 0; i < files.size(); ++i)
    {
        index.FString(files[i].first);
        WriteStoredRecord(index, offsets[i], static_cast<int64_t>(files[i].second.size()));
    }

    if (xorMask != 0)
    {
        while (index.Pos() % 16 != 0) index.Zeros(1); // real encrypted indices are 16-byte aligned
        for (uint8_t& b : index.Bytes) b ^= xorMask;
    }

    const int64_t indexOffset = file.Pos();
    file.Raw(index.Bytes);
    WriteTrailer(file, indexOffset, index.Pos(), xorMask != 0);
    return file.Bytes;
}

static std::shared_ptr<FArchive> Archive(std::string name, std::vector<uint8_t> bytes)
{
    return std::make_shared<FByteArchive>(std::move(name), std::move(bytes));
}

static std::string ToString(const std::vector<uint8_t>& bytes)
{
    return std::string(bytes.begin(), bytes.end());
}

// --------------------------------------------------------------------------------------------------

static void TestFileProviderDictionary()
{
    FileProviderDictionary dict;

    GameFileMap base{StringComparer::OrdinalIgnoreCase()};
    base["Game/A.uasset"] = std::make_shared<FakeGameFile>("Game/A.uasset", "base A");
    base["Game/B.uasset"] = std::make_shared<FakeGameFile>("Game/B.uasset", "base B");
    dict.AddFiles(std::move(base), 3);

    GameFileMap patch{StringComparer::OrdinalIgnoreCase()};
    patch["Game/A.uasset"] = std::make_shared<FakeGameFile>("Game/A.uasset", "patch A");
    dict.AddFiles(std::move(patch), 203);

    CHECK(dict.Count() == 3);
    CHECK(dict.ContainsKey("Game/B.uasset"));
    CHECK(!dict.ContainsKey("Game/C.uasset"));

    // Highest read order wins...
    std::shared_ptr<GameFile> f;
    CHECK(dict.TryGetValue("Game/A.uasset", f) && ToString(f->Read()) == "patch A");
    // ...and the per-index comparer still applies.
    CHECK(dict.TryGetValue("GAME/a.UASSET", f) && ToString(f->Read()) == "patch A");
    CHECK(dict.TryGetValue("Game/B.uasset", f) && ToString(f->Read()) == "base B");
    CHECK(!dict.TryGetValue("Game/C.uasset", f) && f == nullptr);

    // TryGetValues collects every layer, best first.
    std::vector<std::shared_ptr<GameFile>> all;
    CHECK(dict.TryGetValues("Game/A.uasset", all) && all.size() == 2);
    CHECK(ToString(all[0]->Read()) == "patch A");
    CHECK(ToString(all[1]->Read()) == "base A");

    // At throws on a miss.
    bool threw = false;
    try { dict.At("Game/C.uasset"); } catch (const std::out_of_range&) { threw = true; }
    CHECK(threw);

    // Equal read orders resolve to the earliest-added index (the tie-break C# leaves unspecified).
    GameFileMap dupe{StringComparer::OrdinalIgnoreCase()};
    dupe["Game/B.uasset"] = std::make_shared<FakeGameFile>("Game/B.uasset", "later B");
    dict.AddFiles(std::move(dupe), 3);
    CHECK(dict.TryGetValue("Game/B.uasset", f) && ToString(f->Read()) == "base B");

    // ForEach walks descending read order: the patch index first, then the two order-3 indices.
    std::vector<std::string> visited;
    dict.ForEach([&visited](const std::string& path, const std::shared_ptr<GameFile>&) { visited.push_back(path); });
    CHECK(visited.size() == 4);
    CHECK(visited[0] == "Game/A.uasset"); // patch layer

    dict.Clear();
    CHECK(dict.Count() == 0);
}

static void TestFindPayloads()
{
    // Archive A holds the package and its own payloads; a higher-priority patch layer holds a competing
    // uexp. The same-archive payload must win — that is what keeps patched archives self-consistent.
    FakeVfsReader readerA;
    auto pkg = std::make_shared<FakeVfsEntry>(&readerA, "Game/Pkg.uasset", "pkg");
    auto uexpA = std::make_shared<FakeVfsEntry>(&readerA, "Game/Pkg.uexp", "uexp A");
    auto ubulkA = std::make_shared<FakeVfsEntry>(&readerA, "Game/Pkg.ubulk", "ubulk A");
    readerA.FileMap["Game/Pkg.uasset"] = pkg;
    readerA.FileMap["Game/Pkg.uexp"] = uexpA;
    readerA.FileMap["Game/Pkg.ubulk"] = ubulkA;

    FileProviderDictionary dict;
    dict.AddFiles(readerA.FileMap, 3);

    GameFileMap patch{StringComparer::OrdinalIgnoreCase()};
    patch["Game/Pkg.uexp"] = std::make_shared<FakeGameFile>("Game/Pkg.uexp", "uexp PATCH");
    patch["Game/Pkg.uptnl"] = std::make_shared<FakeGameFile>("Game/Pkg.uptnl", "uptnl PATCH");
    dict.AddFiles(std::move(patch), 203);

    std::shared_ptr<GameFile> uexp;
    std::vector<std::shared_ptr<GameFile>> ubulks, uptnls;
    dict.FindPayloads(*pkg, uexp, ubulks, uptnls);
    CHECK(uexp != nullptr && ToString(uexp->Read()) == "uexp A");        // same archive preferred
    CHECK(ubulks.size() == 1 && ToString(ubulks[0]->Read()) == "ubulk A");
    CHECK(uptnls.size() == 1 && ToString(uptnls[0]->Read()) == "uptnl PATCH"); // only the patch has one

    // A file with no archive attribution falls back to the global (read-order) lookup.
    FakeGameFile loose("Game/Pkg.uasset", "pkg");
    dict.FindPayloads(loose, uexp, ubulks, uptnls);
    CHECK(uexp != nullptr && ToString(uexp->Read()) == "uexp PATCH");

    // Non-packages have no payloads.
    FakeGameFile ini("Game/Config.ini", "[x]");
    dict.FindPayloads(ini, uexp, ubulks, uptnls);
    CHECK(uexp == nullptr && ubulks.empty() && uptnls.empty());
}

static void TestProjectNameAndFixPath()
{
    TestProvider provider;

    GameFileMap files{provider.PathComparer};
    files["MyGame/MyGame.uproject"] = std::make_shared<FakeGameFile>("MyGame/MyGame.uproject");
    files["MyGame/Content/Chars/Hero.uasset"] = std::make_shared<FakeGameFile>("MyGame/Content/Chars/Hero.uasset", "hero");
    files["MyGame/Content/Maps/Lobby.umap"] = std::make_shared<FakeGameFile>("MyGame/Content/Maps/Lobby.umap", "lobby");
    files["MyGame/Config/DefaultGame.ini"] = std::make_shared<FakeGameFile>("MyGame/Config/DefaultGame.ini", "[cfg]");
    provider.Files.AddFiles(std::move(files));

    CHECK(provider.ProjectName() == "MyGame");

    CHECK(provider.FixPath("/Game/Chars/Hero") == "MyGame/Content/Chars/Hero.uasset");
    CHECK(provider.FixPath("Game\\Chars\\Hero") == "MyGame/Content/Chars/Hero.uasset");
    CHECK(provider.FixPath("/Game/Config/DefaultGame.ini") == "MyGame/Config/DefaultGame.ini");
    // The FSoftObjectPath duplicate-name form collapses — WITHOUT gaining .uasset, because C#'s second
    // check still looks at the original lastPart, which contains the dot. Kept verbatim.
    CHECK(provider.FixPath("/Game/Chars/Hero.Hero") == "MyGame/Content/Chars/Hero");
    CHECK(provider.FixPath("/Engine/Fonts/Roboto") == "Engine/Content/Fonts/Roboto.uasset");
    // A path already under the project root is left alone.
    CHECK(provider.FixPath("MyGame/Content/Chars/Hero") == "MyGame/Content/Chars/Hero.uasset");

    // Virtual (plugin) roots redirect through VirtualPaths.
    provider.VirtualPaths["CoolPlugin"] = "MyGame/Plugins/CoolPlugin";
    CHECK(provider.FixPath("/CoolPlugin/UI/Widget") == "MyGame/Plugins/CoolPlugin/Content/UI/Widget.uasset");

    // Lookups: extensionless input finds the umap through the fallback.
    CHECK(provider.GetGameFile("/Game/Maps/Lobby")->Path() == "MyGame/Content/Maps/Lobby.umap");
    CHECK(provider.TryGetGameFile("/Game/Maps/Nowhere") == nullptr);
    bool threw = false;
    try { provider.GetGameFile("/Game/Maps/Nowhere"); } catch (const std::out_of_range&) { threw = true; }
    CHECK(threw);

    CHECK(ToString(provider.SaveAsset("/Game/Chars/Hero")) == "hero");
    CHECK(!provider.TrySaveAsset("/Game/Missing").has_value());
    auto reader = provider.CreateReader("/Game/Chars/Hero");
    CHECK(reader != nullptr && reader->Length == 4);

    // LoadPackage refuses non-UE packages outright.
    threw = false;
    try { provider.LoadPackage("/Game/Config/DefaultGame.ini"); }
    catch (const std::invalid_argument&) { threw = true; }
    CHECK(threw);

    // The MidnightSuns -> CodaGame rename.
    TestProvider midnight;
    GameFileMap ms{midnight.PathComparer};
    ms["MidnightSuns/MidnightSuns.uproject"] = std::make_shared<FakeGameFile>("MidnightSuns/MidnightSuns.uproject");
    midnight.Files.AddFiles(std::move(ms));
    CHECK(midnight.ProjectName() == "CodaGame");

    // Without a .uproject the first plausible root (not starting '/', not Engine) names the project.
    TestProvider bare;
    GameFileMap bareFiles{bare.PathComparer};
    bareFiles["Engine/Content/Base.uasset"] = std::make_shared<FakeGameFile>("Engine/Content/Base.uasset");
    bareFiles["Shooter/Content/Gun.uasset"] = std::make_shared<FakeGameFile>("Shooter/Content/Gun.uasset");
    bare.Files.AddFiles(std::move(bareFiles));
    CHECK(bare.ProjectName() == "Shooter");
}

static void TestRegisterAndMount()
{
    TestProvider provider;

    int registered = 0, mounted = 0, unmounted = 0;
    provider.VfsRegistered = [&registered](IVfsReader&, int) { ++registered; };
    provider.VfsMounted = [&mounted](IVfsReader&, int) { ++mounted; };
    provider.VfsUnmounted = [&unmounted](IVfsReader&, int) { ++unmounted; };

    const auto basePak = MakePak({{"fileA.uasset", "base A"}, {"fileB.bin", "base B"}});
    const auto patchPak = MakePak({{"fileA.uasset", "patch A"}});

    provider.RegisterVfs(Archive("pakchunk0-Windows.pak", basePak));
    provider.RegisterVfs(Archive("pakchunk0-Windows_1_P.pak", patchPak));
    // Garbage archives are swallowed at registration, like C#'s catch-and-log.
    provider.RegisterVfs(Archive("junk.pak", std::vector<uint8_t>(64, 0x7F)));
    CHECK(registered == 2);
    CHECK(provider.UnloadedVfs().size() == 2);
    CHECK(provider.RequiredKeys().empty());

    CHECK(provider.Mount() == 2);
    CHECK(mounted == 2);
    CHECK(provider.MountedVfs().size() == 2);
    CHECK(provider.UnloadedVfs().empty());
    CHECK(provider.Files.Count() == 3);

    // The patch pak (read order 203) shadows the base pak (3).
    CHECK(ToString(provider.SaveAsset("Game/fileA.uasset")) == "patch A");
    CHECK(ToString(provider.SaveAsset("Game/fileB.bin")) == "base B");

    // Per-archive lookups see through the shadowing.
    auto& baseArchive = provider.GetArchive("pakchunk0-Windows.pak");
    CHECK(baseArchive.ReadOrder() == 3);
    CHECK(ToString(provider.GetGameFile("Game/fileA.uasset", baseArchive)->Read()) == "base A");
    CHECK(provider.TryGetArchive("nope.pak") == nullptr);
    CHECK(provider.TryGetGameFile("Game/fileA.uasset", "pakchunk0-Windows_1_P.pak") != nullptr);
    CHECK(provider.TryGetGameFile("Game/fileB.bin", "pakchunk0-Windows_1_P.pak") == nullptr);

    // A second Mount is a no-op.
    CHECK(provider.Mount() == 0);

    provider.UnloadAllVfs();
    CHECK(unmounted == 2);
    CHECK(provider.MountedVfs().empty());
    CHECK(provider.UnloadedVfs().size() == 2);
    CHECK(provider.Files.Count() == 0);

    // ...and everything can come back.
    CHECK(provider.Mount() == 2);
    CHECK(ToString(provider.SaveAsset("Game/fileA.uasset")) == "patch A");
}

static void TestEncryptedProviderFlow()
{
    const FGuid zeroGuid;
    const auto key = std::make_shared<FAesKey>(std::vector<uint8_t>(32, 0x11));
    const auto encryptedPak = MakePak({{"secret.bin", "hidden bytes"}}, 0x5A);

    // Without a custom-encryption hook the encrypted pak needs a real AES key — and a wrong one must be
    // rejected end-to-end (the mount-point probe fails), never mounted as garbage.
    TestProvider provider;
    provider.RegisterVfs(Archive("enc.pak", encryptedPak));
    CHECK(provider.UnloadedVfs().size() == 1);
    CHECK(provider.RequiredKeys().size() == 1 && provider.RequiredKeys().count(zeroGuid) == 1);

    CHECK(provider.Mount() == 0); // encrypted + no hook: skipped entirely
    CHECK(provider.SubmitKey(zeroGuid, key) == 0); // wrong real AES key: InvalidAesKeyException, swallowed
    CHECK(provider.UnloadedVfs().size() == 1);
    CHECK(provider.Keys().empty());
    CHECK(provider.RequiredKeys().size() == 1);

    // With the XOR hook installed before registration, SubmitKey mounts it and records the key.
    TestProvider xorProvider;
    xorProvider.CustomEncryption = [](const std::vector<uint8_t>& bytes, int beginOffset, int count,
                                      bool /*isIndex*/, IAesVfsReader&)
    {
        std::vector<uint8_t> out = bytes;
        for (int i = 0; i < count; ++i) out[static_cast<size_t>(beginOffset) + i] ^= 0x5A;
        return out;
    };
    xorProvider.RegisterVfs(Archive("enc.pak", encryptedPak));
    CHECK(xorProvider.SubmitKey(zeroGuid, key) == 1);
    CHECK(xorProvider.MountedVfs().size() == 1);
    CHECK(xorProvider.RequiredKeys().empty());
    CHECK(xorProvider.Keys().size() == 1 && xorProvider.Keys().count(zeroGuid) == 1);
    CHECK(ToString(xorProvider.SaveAsset("Game/secret.bin")) == "hidden bytes");
}

static void TestDefaultFileProviderScan()
{
    const fs::path root = fs::temp_directory_path() / "fmodelcpp_test_dfp";
    fs::remove_all(root);
    fs::create_directories(root / "Paks");
    fs::create_directories(root / "Config");
    fs::create_directories(root / "Textures");
    fs::create_directories(root / "Binaries" / "ThirdParty" / "CEF");

    const auto writeFile = [](const fs::path& p, const std::vector<uint8_t>& bytes)
    {
        std::ofstream out(p, std::ios::binary);
        out.write(reinterpret_cast<const char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
    };
    const auto writeText = [&writeFile](const fs::path& p, const std::string& text)
    {
        writeFile(p, {text.begin(), text.end()});
    };

    const auto pak = MakePak({{"fileA.uasset", "from disk"}});
    writeFile(root / "Paks" / "pakchunk0-Windows.pak", pak);
    writeFile(root / "Binaries" / "ThirdParty" / "CEF" / "cef.pak", pak); // excluded path
    writeText(root / "Paks" / "readme.txt", "not a game file");           // unknown extension
    writeText(root / "Config" / "Defaults.ini", "[settings]");
    writeText(root / "Textures" / "streaming.tfc", "texcache");

    // Scoped: the registered pak keeps its file handle open for the provider's lifetime, and remove_all
    // below needs it released.
    {
    DefaultFileProvider provider(root, SearchOption::AllDirectories,
                                 VersionContainer(), StringComparer::OrdinalIgnoreCase());
    provider.Initialize();

    // One registered container (the CEF one is excluded), one loose known-extension file.
    CHECK(provider.UnloadedVfs().size() == 1);
    CHECK(provider.LooseFileCount == 1);
    CHECK(provider.TextureCachePaths.count("streaming") == 1);

    // The loose file is an OsGameFile at its working-directory-relative path.
    auto ini = provider.TryGetGameFile("Config/Defaults.ini");
    CHECK(ini != nullptr && dynamic_cast<OsGameFile*>(ini.get()) != nullptr);
    CHECK(ToString(ini->Read()) == "[settings]");
    auto iniReader = ini->CreateReader();
    CHECK(iniReader != nullptr && iniReader->Length == 10);

    // Mount the pak found on disk and read through the provider.
    CHECK(provider.Mount() == 1);
    CHECK(ToString(provider.SaveAsset("Game/fileA.uasset")) == "from disk");
    // The pak file's garbage "uasset" fails to parse as a package: null, not a crash.
    CHECK(provider.TryLoadPackage("Game/fileA.uasset") == nullptr);

    // A .uproject at the top level flips the scan into loose-file mode: containers are not registered,
    // and files are gathered recursively even with TopDirectoryOnly.
    const fs::path projRoot = root / "Project";
    fs::create_directories(projRoot / "Content");
    writeText(projRoot / "MyGame.uproject", "{}");
    writeText(projRoot / "Content" / "Thing.uasset", "thing");
    writeFile(projRoot / "Content" / "nested.pak", pak);

    DefaultFileProvider projProvider(projRoot, SearchOption::TopDirectoryOnly,
                                     VersionContainer(), StringComparer::OrdinalIgnoreCase());
    projProvider.Initialize();
    CHECK(projProvider.UnloadedVfs().empty()); // no containers in uproject mode
    auto thing = projProvider.TryGetGameFile("Content/Thing.uasset");
    CHECK(thing != nullptr && ToString(thing->Read()) == "thing");

    // A missing directory is reported, not scanned.
    DefaultFileProvider missing(root / "DoesNotExist", SearchOption::AllDirectories);
    bool threw = false;
    try { missing.Initialize(); } catch (const std::runtime_error&) { threw = true; }
    CHECK(threw);
    } // release the providers (and their open pak handles) before cleaning up

    fs::remove_all(root);
}

int main()
{
    TestFileProviderDictionary();
    TestFindPayloads();
    TestProjectNameAndFixPath();
    TestRegisterAndMount();
    TestEncryptedProviderFlow();
    TestDefaultFileProviderScan();

    if (g_failures == 0) std::printf("test_default_file_provider: all checks passed\n");
    else std::printf("test_default_file_provider: %d check(s) failed\n", g_failures);
    return g_failures == 0 ? 0 : 1;
}
