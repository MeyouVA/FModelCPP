// Tests for the import/export map layer: FAssetArchive (name resolution + byte forwarding), FPackageIndex,
// and the FObjectExport / FObjectImport serialization constructors. A tiny in-memory IPackage supplies the
// name pool and package flags.
#include <cstdint>
#include <cstring>
#include <iostream>
#include <string>
#include <vector>

#include "UE4/Objects/UObject/ObjectResource.h"
#include "UE4/Assets/Readers/FAssetArchive.h"
#include "UE4/Assets/IPackage.h"
#include "UE4/Assets/Exports/EObjectFlags.h"
#include "UE4/Readers/FByteArchive.h"
#include "UE4/Versions/VersionContainer.h"
#include "UE4/Versions/ObjectVersion.h"

using namespace CUE4Parse::UE4::Objects::UObject;
using namespace CUE4Parse::UE4::Readers;
using namespace CUE4Parse::UE4::Versions;
using namespace CUE4Parse::UE4::Assets::Exports;
using CUE4Parse::UE4::Assets::IPackage;
using CUE4Parse::UE4::Assets::Readers::FAssetArchive;
using CUE4Parse::UE4::Objects::Core::Misc::FGuid;

static int g_failures = 0;
#define CHECK(cond)                                                            \
    do {                                                                       \
        if (!(cond)) {                                                         \
            std::cerr << "FAIL: " << #cond << " (line " << __LINE__ << ")\n";  \
            ++g_failures;                                                      \
        }                                                                      \
    } while (0)

template <typename T>
static void AppendLE(std::vector<uint8_t>& buf, T value)
{
    uint8_t tmp[sizeof(T)];
    std::memcpy(tmp, &value, sizeof(T));
    buf.insert(buf.end(), tmp, tmp + sizeof(T));
}

// An FName in an FAssetArchive is (nameIndex, extraIndex) — extraIndex is read because the pinned UE3
// version is past FNAME_CHANGE_NAME_SPLIT.
static void AppendFName(std::vector<uint8_t>& buf, int32_t nameIndex, int32_t extraIndex = 0)
{
    AppendLE<int32_t>(buf, nameIndex);
    AppendLE<int32_t>(buf, extraIndex);
}

class TestPackage : public IPackage
{
public:
    std::vector<FNameEntrySerialized> Names;
    uint32_t Flags = 0;
    std::string PkgName = "TestPackage";

    const std::string& GetName() const override { return PkgName; }
    const std::vector<FNameEntrySerialized>& NameMap() const override { return Names; }
    bool HasFlags(EPackageFlags flags) const override { return (Flags & static_cast<uint32_t>(flags)) != 0; }
    // This fixture exercises the resource *readers*, not resolution, so indices don't resolve (Name -> "None").
    CUE4Parse::UE4::Assets::ResolvedObject* ResolvePackageIndex(const FPackageIndex*) override { return nullptr; }
};

static FNameEntrySerialized Entry(const std::string& s) { return FNameEntrySerialized(s); }

int main()
{
    const int32_t UE4_AUTO = static_cast<int32_t>(EUnrealEngineObjectUE4Version::AUTOMATIC_VERSION);
    // Pin ue3=864 (past FNAME_CHANGE_NAME_SPLIT), ue4=AUTOMATIC (all UE4 gates on), ue5=0 (all UE5 gates off).
    const VersionContainer VC(GAME_UE4_LATEST, ETexturePlatform::DesktopMobile, FPackageFileVersion(864, UE4_AUTO, 0));

    TestPackage pkg;
    pkg.Names = {Entry("None"), Entry("Core"), Entry("Class"), Entry("MyImport"), Entry("MyPackage"), Entry("MyExport")};

    // --- FPackageIndex sign semantics ---
    {
        FByteArchive base("idx", {}, VC);
        FAssetArchive ar(base, &pkg);
        FPackageIndex nul(&pkg, 0);
        FPackageIndex exp(&pkg, 3);
        FPackageIndex imp(&pkg, -2);
        CHECK(nul.IsNull() && !nul.IsExport() && !nul.IsImport());
        CHECK(exp.IsExport() && !exp.IsNull() && !exp.IsImport());
        CHECK(imp.IsImport() && !imp.IsNull() && !imp.IsExport());
        CHECK(exp.ToString() == "3");
        CHECK(FPackageIndex(&pkg, 5) == FPackageIndex(&pkg, 5));
        CHECK(FPackageIndex(&pkg, 5) != FPackageIndex(&pkg, 6));
    }

    // --- FObjectImport ---
    {
        std::vector<uint8_t> buf;
        AppendFName(buf, 1);   // ClassPackage = "Core"
        AppendFName(buf, 2);   // ClassName    = "Class"
        AppendLE<int32_t>(buf, -5); // OuterIndex
        AppendFName(buf, 3);   // ObjectName   = "MyImport"
        AppendFName(buf, 4);   // PackageName  = "MyPackage" (read: NON_OUTER_PACKAGE_IMPORT && !filter-editor-only)
        // ImportOptional not read (OPTIONAL_RESOURCES is a UE5 gate, off here).

        FByteArchive base("import", buf, VC);
        FAssetArchive ar(base, &pkg);
        FObjectImport imp(ar);

        CHECK(imp.ClassPackage.Text() == "Core");
        CHECK(imp.ClassName.Text() == "Class");
        CHECK(imp.OuterIndex.Index == -5 && imp.OuterIndex.IsImport());
        CHECK(imp.ObjectName.Text() == "MyImport");
        CHECK(imp.PackageName.Text() == "MyPackage");
        CHECK(!imp.ImportOptional);
        CHECK(imp.ToString() == "MyImport");
        CHECK(ar.Position == static_cast<int64_t>(buf.size())); // consumed exactly the whole record
    }

    // --- FObjectImport with editor-only filtered: PackageName is skipped ---
    {
        TestPackage filtered;
        filtered.Names = pkg.Names;
        filtered.Flags = static_cast<uint32_t>(PKG_FilterEditorOnly);

        std::vector<uint8_t> buf;
        AppendFName(buf, 1);   // ClassPackage
        AppendFName(buf, 2);   // ClassName
        AppendLE<int32_t>(buf, 0); // OuterIndex
        AppendFName(buf, 3);   // ObjectName
        // no PackageName

        FByteArchive base("import2", buf, VC);
        FAssetArchive ar(base, &filtered);
        CHECK(ar.IsFilterEditorOnly());
        FObjectImport imp(ar);
        CHECK(imp.ObjectName.Text() == "MyImport");
        CHECK(imp.PackageName.IsNone()); // default FName -> "None"
        CHECK(ar.Position == static_cast<int64_t>(buf.size()));
    }

    // --- FObjectExport ---
    {
        std::vector<uint8_t> buf;
        AppendLE<int32_t>(buf, -1);   // ClassIndex (import)
        AppendLE<int32_t>(buf, 0);    // SuperIndex
        AppendLE<int32_t>(buf, 0);    // TemplateIndex (read: TemplateIndex_IN_COOKED_EXPORTS)
        AppendLE<int32_t>(buf, 0);    // OuterIndex
        AppendFName(buf, 5);          // ObjectName = "MyExport"
        AppendLE<uint32_t>(buf, static_cast<uint32_t>(RF_Public)); // ObjectFlags
        AppendLE<int64_t>(buf, 1024); // SerialSize (64-bit: e64BIT_EXPORTMAP_SERIALSIZES)
        AppendLE<int64_t>(buf, 4096); // SerialOffset
        AppendLE<int32_t>(buf, 0);    // ForcedExport (Game >= GAME_UE4_0)
        AppendLE<int32_t>(buf, 1);    // NotForClient
        AppendLE<int32_t>(buf, 0);    // NotForServer
        AppendLE<uint32_t>(buf, 0x11111111u); // PackageGuid (read: < REMOVE_OBJECT_EXPORT_PACKAGE_GUID)
        AppendLE<uint32_t>(buf, 0x22222222u);
        AppendLE<uint32_t>(buf, 0x33333333u);
        AppendLE<uint32_t>(buf, 0x44444444u);
        // IsInheritedInstance not read (UE5 gate off)
        AppendLE<uint32_t>(buf, 0);   // PackageFlags
        AppendLE<int32_t>(buf, 1);    // NotAlwaysLoadedForEditorGame (LOAD_FOR_EDITOR_GAME)
        AppendLE<int32_t>(buf, 1);    // IsAsset (COOKED_ASSETS_IN_EDITOR_SUPPORT)
        // GeneratePublicHash not read (OPTIONAL_RESOURCES UE5 gate off)
        AppendLE<int32_t>(buf, 7);    // FirstExportDependency (PRELOAD_DEPENDENCIES_IN_COOKED_EXPORTS)
        AppendLE<int32_t>(buf, 1);    // SerializationBeforeSerializationDependencies
        AppendLE<int32_t>(buf, 2);    // CreateBeforeSerializationDependencies
        AppendLE<int32_t>(buf, 3);    // SerializationBeforeCreateDependencies
        AppendLE<int32_t>(buf, 4);    // CreateBeforeCreateDependencies
        // Script serialization offsets not read (UE5 gate off)

        FByteArchive base("export", buf, VC);
        FAssetArchive ar(base, &pkg);
        FObjectExport exp(ar);

        CHECK(exp.ClassIndex.Index == -1 && exp.ClassIndex.IsImport());
        CHECK(exp.TemplateIndex.IsNull());
        CHECK(exp.ObjectName.Text() == "MyExport");
        CHECK(exp.ObjectFlags == static_cast<uint32_t>(RF_Public));
        CHECK(exp.SerialSize == 1024);
        CHECK(exp.SerialOffset == 4096);
        CHECK(!exp.ForcedExport && exp.NotForClient && !exp.NotForServer);
        CHECK(exp.PackageGuid == FGuid(0x11111111u, 0x22222222u, 0x33333333u, 0x44444444u));
        CHECK(!exp.IsInheritedInstance);
        CHECK(exp.PackageFlags == 0);
        CHECK(exp.NotAlwaysLoadedForEditorGame && exp.IsAsset);
        CHECK(exp.FirstExportDependency == 7);
        CHECK(exp.CreateBeforeCreateDependencies == 4);
        CHECK(exp.ScriptSerializationStartOffset == 0);
        // Class resolution is deferred -> ClassName / ClassIndex.Name() are "None".
        CHECK(exp.ClassName == "None");
        CHECK(exp.ToString() == "MyExport (None)");
        CHECK(ar.Position == static_cast<int64_t>(buf.size()));
    }

    // --- SerialSize as 32-bit on an older archive (< e64BIT_EXPORTMAP_SERIALSIZES) ---
    {
        const int32_t OLD_UE4 = static_cast<int32_t>(EUnrealEngineObjectUE4Version::OLDEST_LOADABLE_PACKAGE);
        const VersionContainer OLD_VC(GAME_UE4_0, ETexturePlatform::DesktopMobile, FPackageFileVersion(864, OLD_UE4, 0));

        std::vector<uint8_t> buf;
        AppendLE<int32_t>(buf, 0);    // ClassIndex
        AppendLE<int32_t>(buf, 0);    // SuperIndex
        // TemplateIndex NOT read (< TemplateIndex_IN_COOKED_EXPORTS)
        AppendLE<int32_t>(buf, 0);    // OuterIndex
        AppendFName(buf, 5);          // ObjectName
        AppendLE<uint32_t>(buf, 0);   // ObjectFlags
        AppendLE<int32_t>(buf, 512);  // SerialSize (32-bit)
        AppendLE<int32_t>(buf, 64);   // SerialOffset (32-bit)
        AppendLE<int32_t>(buf, 0);    // ForcedExport (Game == GAME_UE4_0, still >= GAME_UE4_0)
        AppendLE<int32_t>(buf, 0);    // NotForClient
        AppendLE<int32_t>(buf, 0);    // NotForServer
        AppendLE<uint32_t>(buf, 0);   // PackageGuid x4
        AppendLE<uint32_t>(buf, 0);
        AppendLE<uint32_t>(buf, 0);
        AppendLE<uint32_t>(buf, 0);
        AppendLE<uint32_t>(buf, 0);   // PackageFlags
        // LOAD_FOR_EDITOR_GAME / COOKED_ASSETS_IN_EDITOR_SUPPORT / PRELOAD gates all off at this version.

        FByteArchive base("export_old", buf, OLD_VC);
        FAssetArchive ar(base, &pkg);
        FObjectExport exp(ar);
        CHECK(exp.SerialSize == 512);
        CHECK(exp.SerialOffset == 64);
        CHECK(exp.TemplateIndex.IsNull());
        CHECK(exp.FirstExportDependency == -1); // default when PRELOAD gate is off
        CHECK(ar.Position == static_cast<int64_t>(buf.size()));
    }

    if (g_failures == 0)
    {
        std::cout << "All import/export map tests passed.\n";
        return 0;
    }
    std::cout << g_failures << " check(s) failed.\n";
    return 1;
}
