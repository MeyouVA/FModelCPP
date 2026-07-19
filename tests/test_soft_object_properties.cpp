// Tests the soft / object-ish property layer: build tagged SoftObjectProperty / LazyObjectProperty /
// AssetObjectProperty / WeakObjectProperty / ClassProperty blobs by hand, read them with
// UObject::DeserializePropertiesTagged, and verify the parsed values.
#include <cstdint>
#include <cstring>
#include <iostream>
#include <string>
#include <vector>

#include "UE4/Assets/Exports/UObject.h"
#include "UE4/Assets/Objects/FPropertyTag.h"
#include "UE4/Assets/Objects/Properties/FPropertyTagType.h"
#include "UE4/Assets/Objects/Properties/SoftObjectProperty.h"
#include "UE4/Assets/Objects/Properties/LazyObjectProperty.h"
#include "UE4/Assets/Objects/Properties/AssetObjectProperty.h"
#include "UE4/Assets/Objects/Properties/WeakObjectProperty.h"
#include "UE4/Assets/Objects/Properties/ClassProperty.h"
#include "UE4/Assets/Objects/Properties/ObjectProperty.h"
#include "UE4/Assets/Readers/FAssetArchive.h"
#include "UE4/Assets/IPackage.h"
#include "UE4/Objects/UObject/FNameEntrySerialized.h"
#include "UE4/Readers/FByteArchive.h"
#include "UE4/Versions/VersionContainer.h"
#include "UE4/Versions/ObjectVersion.h"

using namespace CUE4Parse::UE4::Assets;
using namespace CUE4Parse::UE4::Assets::Exports;
using namespace CUE4Parse::UE4::Assets::Objects;
using namespace CUE4Parse::UE4::Assets::Objects::Properties;
using namespace CUE4Parse::UE4::Readers;
using namespace CUE4Parse::UE4::Versions;
using CUE4Parse::UE4::Objects::UObject::FNameEntrySerialized;

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

static void AppendFString(std::vector<uint8_t>& buf, const std::string& s)
{
    AppendLE<int32_t>(buf, static_cast<int32_t>(s.size() + 1));
    buf.insert(buf.end(), s.begin(), s.end());
    buf.push_back(0);
}

static void AppendFName(std::vector<uint8_t>& buf, int32_t nameIndex, int32_t extraIndex = 0)
{
    AppendLE<int32_t>(buf, nameIndex);
    AppendLE<int32_t>(buf, extraIndex);
}

static void AppendTag(std::vector<uint8_t>& buf, int32_t nameIdx, int32_t typeIdx,
                      const std::vector<uint8_t>& tagDataAndValue, int32_t valueSize)
{
    AppendFName(buf, nameIdx);
    AppendFName(buf, typeIdx);
    AppendLE<int32_t>(buf, valueSize);
    AppendLE<int32_t>(buf, 0);
    buf.insert(buf.end(), tagDataAndValue.begin(), tagDataAndValue.end());
}

class TestPackage : public IPackage
{
public:
    std::vector<FNameEntrySerialized> Names;
    std::string PkgName = "TestPackage";
    const std::string& GetName() const override { return PkgName; }
    const std::vector<FNameEntrySerialized>& NameMap() const override { return Names; }
    bool HasFlags(CUE4Parse::UE4::Objects::UObject::EPackageFlags) const override { return false; }
    ResolvedObject* ResolvePackageIndex(const CUE4Parse::UE4::Objects::UObject::FPackageIndex*) override { return nullptr; }
};

static FNameEntrySerialized Entry(const std::string& s) { return FNameEntrySerialized(s); }

static bool EndsWith(const std::string& s, const std::string& suffix)
{
    return s.size() >= suffix.size() && s.compare(s.size() - suffix.size(), suffix.size(), suffix) == 0;
}

int main()
{
    const int32_t UE4_AUTO = static_cast<int32_t>(EUnrealEngineObjectUE4Version::AUTOMATIC_VERSION);
    const VersionContainer VC(GAME_UE4_LATEST, ETexturePlatform::DesktopMobile, FPackageFileVersion(864, UE4_AUTO, 0));

    TestPackage pkg;
    enum : int32_t {
        N_None = 0, N_SoftObjectProperty, N_LazyObjectProperty, N_AssetObjectProperty,
        N_WeakObjectProperty, N_ClassProperty,
        N_MySoft, N_MyLazy, N_MyAsset, N_MyWeak, N_MyClass, N_AssetPath,
    };
    pkg.Names = {
        Entry("None"),                  // 0
        Entry("SoftObjectProperty"),    // 1
        Entry("LazyObjectProperty"),    // 2
        Entry("AssetObjectProperty"),   // 3
        Entry("WeakObjectProperty"),    // 4
        Entry("ClassProperty"),         // 5
        Entry("MySoft"),                // 6
        Entry("MyLazy"),                // 7
        Entry("MyAsset"),               // 8
        Entry("MyWeak"),                // 9
        Entry("MyClass"),               // 10
        Entry("/Game/Item.Item"),       // 11 (soft object asset path name)
    };

    std::vector<uint8_t> buf;

    // --- MySoft : SoftObjectProperty = { AssetPathName="/Game/Item.Item", SubPathString="SubObj" } ---
    {
        std::vector<uint8_t> v;
        v.push_back(0);                      // guid flag (no tag data)
        std::vector<uint8_t> value;
        AppendFName(value, N_AssetPath);     // AssetPathName
        AppendFString(value, "SubObj");      // SubPathString
        v.insert(v.end(), value.begin(), value.end());
        AppendTag(buf, N_MySoft, N_SoftObjectProperty, v, static_cast<int32_t>(value.size()));
    }

    // --- MyLazy : LazyObjectProperty = FUniqueObjectGuid{ FGuid(1,2,3,4) } ---
    {
        std::vector<uint8_t> v;
        v.push_back(0);                      // guid flag
        std::vector<uint8_t> value;
        AppendLE<uint32_t>(value, 1);
        AppendLE<uint32_t>(value, 2);
        AppendLE<uint32_t>(value, 3);
        AppendLE<uint32_t>(value, 4);
        v.insert(v.end(), value.begin(), value.end());
        AppendTag(buf, N_MyLazy, N_LazyObjectProperty, v, static_cast<int32_t>(value.size()));
    }

    // --- MyAsset : AssetObjectProperty = "/Game/Path" ---
    {
        std::vector<uint8_t> v;
        v.push_back(0);                      // guid flag
        std::vector<uint8_t> value;
        AppendFString(value, "/Game/Path");
        v.insert(v.end(), value.begin(), value.end());
        AppendTag(buf, N_MyAsset, N_AssetObjectProperty, v, static_cast<int32_t>(value.size()));
    }

    // --- MyWeak : WeakObjectProperty = FPackageIndex(-3) (import) ---
    {
        std::vector<uint8_t> v;
        v.push_back(0);                      // guid flag
        AppendLE<int32_t>(v, -3);
        AppendTag(buf, N_MyWeak, N_WeakObjectProperty, v, 4);
    }

    // --- MyClass : ClassProperty = FPackageIndex(5) (export) ---
    {
        std::vector<uint8_t> v;
        v.push_back(0);                      // guid flag
        AppendLE<int32_t>(v, 5);
        AppendTag(buf, N_MyClass, N_ClassProperty, v, 4);
    }

    // Terminating "None" tag.
    AppendFName(buf, N_None);

    FByteArchive base("softobj", buf, VC);
    FAssetArchive ar(base, &pkg);

    std::vector<FPropertyTag> props;
    UObject::DeserializePropertiesTagged(props, ar, false);

    CHECK(props.size() == 5);
    CHECK(ar.Position == static_cast<int64_t>(buf.size()));

    // MySoft
    CHECK(props[0].Name.Text() == "MySoft" && props[0].PropertyType.Text() == "SoftObjectProperty");
    auto* softProp = dynamic_cast<SoftObjectProperty*>(props[0].Tag.get());
    CHECK(softProp != nullptr);
    if (softProp)
    {
        CHECK(softProp->Value.AssetPathName.Text() == "/Game/Item.Item");
        CHECK(softProp->Value.SubPathString == "SubObj");
        CHECK(softProp->Value.Owner == &pkg);
    }
    CHECK(props[0].ToString() == "MySoft  -->  /Game/Item.Item:SubObj (SoftObjectProperty)");

    // MyLazy
    CHECK(props[1].Name.Text() == "MyLazy" && props[1].PropertyType.Text() == "LazyObjectProperty");
    auto* lazyProp = dynamic_cast<LazyObjectProperty*>(props[1].Tag.get());
    CHECK(lazyProp != nullptr);
    if (lazyProp)
    {
        CHECK(lazyProp->Value.Guid.A == 1 && lazyProp->Value.Guid.B == 2 &&
              lazyProp->Value.Guid.C == 3 && lazyProp->Value.Guid.D == 4);
    }
    CHECK(props[1].ToString() == "MyLazy  -->  00000001000000020000000300000004 (LazyObjectProperty)");

    // MyAsset
    CHECK(props[2].Name.Text() == "MyAsset" && props[2].PropertyType.Text() == "AssetObjectProperty");
    auto* assetProp = dynamic_cast<AssetObjectProperty*>(props[2].Tag.get());
    CHECK(assetProp != nullptr);
    if (assetProp) CHECK(assetProp->Value == "/Game/Path");
    CHECK(props[2].ToString() == "MyAsset  -->  /Game/Path (AssetObjectProperty)");

    // MyWeak: a WeakObjectProperty is-an ObjectProperty; both casts should succeed.
    CHECK(props[3].Name.Text() == "MyWeak" && props[3].PropertyType.Text() == "WeakObjectProperty");
    auto* weakProp = dynamic_cast<WeakObjectProperty*>(props[3].Tag.get());
    CHECK(weakProp != nullptr);
    CHECK(dynamic_cast<ObjectProperty*>(props[3].Tag.get()) != nullptr);
    if (weakProp)
    {
        CHECK(weakProp->Value.Index == -3);
        CHECK(weakProp->Value.IsImport());
    }
    CHECK(EndsWith(props[3].ToString(), " (WeakObjectProperty)"));

    // MyClass
    CHECK(props[4].Name.Text() == "MyClass" && props[4].PropertyType.Text() == "ClassProperty");
    auto* classProp = dynamic_cast<ClassProperty*>(props[4].Tag.get());
    CHECK(classProp != nullptr);
    CHECK(dynamic_cast<ObjectProperty*>(props[4].Tag.get()) != nullptr);
    if (classProp)
    {
        CHECK(classProp->Value.Index == 5);
        CHECK(classProp->Value.IsExport());
    }
    CHECK(EndsWith(props[4].ToString(), " (ClassProperty)"));

    if (g_failures == 0)
    {
        std::cout << "All soft/object-ish property tests passed.\n";
        return 0;
    }
    std::cout << g_failures << " check(s) failed.\n";
    return 1;
}
