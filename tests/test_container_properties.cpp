// Tests the container-property layer: build tagged MapProperty / SetProperty / ObjectProperty blobs by hand,
// read them with UObject::DeserializePropertiesTagged, and verify the parsed entries/elements/index.
#include <cstdint>
#include <cstring>
#include <iostream>
#include <string>
#include <vector>

#include "UE4/Assets/Exports/UObject.h"
#include "UE4/Assets/Objects/FPropertyTag.h"
#include "UE4/Assets/Objects/UScriptMap.h"
#include "UE4/Assets/Objects/UScriptSet.h"
#include "UE4/Assets/Objects/Properties/FPropertyTagType.h"
#include "UE4/Assets/Objects/Properties/IntProperty.h"
#include "UE4/Assets/Objects/Properties/StrProperty.h"
#include "UE4/Assets/Objects/Properties/MapProperty.h"
#include "UE4/Assets/Objects/Properties/SetProperty.h"
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

int main()
{
    const int32_t UE4_AUTO = static_cast<int32_t>(EUnrealEngineObjectUE4Version::AUTOMATIC_VERSION);
    const VersionContainer VC(GAME_UE4_LATEST, ETexturePlatform::DesktopMobile, FPackageFileVersion(864, UE4_AUTO, 0));

    TestPackage pkg;
    enum : int32_t {
        N_None = 0, N_MapProperty, N_SetProperty, N_ObjectProperty, N_IntProperty, N_StrProperty,
        N_MyMap, N_MySet, N_MyObject,
    };
    pkg.Names = {
        Entry("None"),            // 0
        Entry("MapProperty"),     // 1
        Entry("SetProperty"),     // 2
        Entry("ObjectProperty"),  // 3
        Entry("IntProperty"),     // 4
        Entry("StrProperty"),     // 5
        Entry("MyMap"),           // 6
        Entry("MySet"),           // 7
        Entry("MyObject"),        // 8
    };

    std::vector<uint8_t> buf;

    // --- MyMap : MapProperty<IntProperty, StrProperty> = { 100:"aaa", 200:"bb" } ---
    {
        std::vector<uint8_t> v;
        AppendFName(v, N_IntProperty);       // tag data: key (inner) type
        AppendFName(v, N_StrProperty);       // tag data: value type
        v.push_back(0);                      // guid flag
        std::vector<uint8_t> value;
        AppendLE<int32_t>(value, 0);         // numKeysToRemove
        AppendLE<int32_t>(value, 2);         // numEntries
        AppendLE<int32_t>(value, 100); AppendFString(value, "aaa");
        AppendLE<int32_t>(value, 200); AppendFString(value, "bb");
        v.insert(v.end(), value.begin(), value.end());
        AppendTag(buf, N_MyMap, N_MapProperty, v, static_cast<int32_t>(value.size()));
    }

    // --- MySet : SetProperty<IntProperty> = { 11, 22, 33 } ---
    {
        std::vector<uint8_t> v;
        AppendFName(v, N_IntProperty);       // tag data: inner type
        v.push_back(0);                      // guid flag
        std::vector<uint8_t> value;
        AppendLE<int32_t>(value, 0);         // numElementsToRemove
        AppendLE<int32_t>(value, 3);         // count
        AppendLE<int32_t>(value, 11);
        AppendLE<int32_t>(value, 22);
        AppendLE<int32_t>(value, 33);
        v.insert(v.end(), value.begin(), value.end());
        AppendTag(buf, N_MySet, N_SetProperty, v, static_cast<int32_t>(value.size()));
    }

    // --- MyObject : ObjectProperty = FPackageIndex(7) ---
    {
        std::vector<uint8_t> v;
        v.push_back(0);                      // guid flag (no tag data for ObjectProperty)
        AppendLE<int32_t>(v, 7);             // package index
        AppendTag(buf, N_MyObject, N_ObjectProperty, v, 4);
    }

    // Terminating "None" tag.
    AppendFName(buf, N_None);

    FByteArchive base("container", buf, VC);
    FAssetArchive ar(base, &pkg);

    std::vector<FPropertyTag> props;
    UObject::DeserializePropertiesTagged(props, ar, false);

    CHECK(props.size() == 3);
    CHECK(ar.Position == static_cast<int64_t>(buf.size()));

    // MyMap
    CHECK(props[0].Name.Text() == "MyMap" && props[0].PropertyType.Text() == "MapProperty");
    auto* mapProp = dynamic_cast<MapProperty*>(props[0].Tag.get());
    CHECK(mapProp != nullptr);
    if (mapProp)
    {
        auto& entries = mapProp->Value.Properties;
        CHECK(entries.size() == 2);
        if (entries.size() == 2)
        {
            auto* k0 = dynamic_cast<IntProperty*>(entries[0].first.get());
            auto* v0 = dynamic_cast<StrProperty*>(entries[0].second.get());
            auto* k1 = dynamic_cast<IntProperty*>(entries[1].first.get());
            auto* v1 = dynamic_cast<StrProperty*>(entries[1].second.get());
            CHECK(k0 != nullptr && k0->Value == 100);
            CHECK(v0 != nullptr && v0->Value == "aaa");
            CHECK(k1 != nullptr && k1->Value == 200);
            CHECK(v1 != nullptr && v1->Value == "bb");
        }
    }
    CHECK(props[0].ToString() == "MyMap  -->  {2 entries} (MapProperty)");

    // MySet
    CHECK(props[1].Name.Text() == "MySet" && props[1].PropertyType.Text() == "SetProperty");
    auto* setProp = dynamic_cast<SetProperty*>(props[1].Tag.get());
    CHECK(setProp != nullptr);
    if (setProp)
    {
        CHECK(setProp->Value.Properties.size() == 3);
        const int32_t expected[3] = {11, 22, 33};
        for (size_t i = 0; i < setProp->Value.Properties.size() && i < 3; i++)
        {
            auto* e = dynamic_cast<IntProperty*>(setProp->Value.Properties[i].get());
            CHECK(e != nullptr && e->Value == expected[i]);
        }
    }
    CHECK(props[1].ToString() == "MySet  -->  [3 elements] (SetProperty)");

    // MyObject
    CHECK(props[2].Name.Text() == "MyObject" && props[2].PropertyType.Text() == "ObjectProperty");
    auto* objProp = dynamic_cast<ObjectProperty*>(props[2].Tag.get());
    CHECK(objProp != nullptr);
    if (objProp)
    {
        CHECK(objProp->Value.Index == 7);
        CHECK(objProp->Value.IsExport());
    }

    if (g_failures == 0)
    {
        std::cout << "All container-property tests passed.\n";
        return 0;
    }
    std::cout << g_failures << " check(s) failed.\n";
    return 1;
}
