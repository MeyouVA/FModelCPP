// Tests the tagged-property layer: build a name pool + a small tagged-property blob by hand, read it with
// UObject::DeserializePropertiesTagged, and verify each scalar property parsed to the right value.
#include <cstdint>
#include <cstring>
#include <iostream>
#include <string>
#include <vector>

#include "UE4/Assets/Exports/UObject.h"
#include "UE4/Assets/Objects/FPropertyTag.h"
#include "UE4/Assets/Objects/Properties/FPropertyTagType.h"
#include "UE4/Assets/Objects/Properties/IntProperty.h"
#include "UE4/Assets/Objects/Properties/BoolProperty.h"
#include "UE4/Assets/Objects/Properties/FloatProperty.h"
#include "UE4/Assets/Objects/Properties/NameProperty.h"
#include "UE4/Assets/Objects/Properties/StrProperty.h"
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

// FName reference in an asset archive: (nameIndex, extraIndex).
static void AppendFName(std::vector<uint8_t>& buf, int32_t nameIndex, int32_t extraIndex = 0)
{
    AppendLE<int32_t>(buf, nameIndex);
    AppendLE<int32_t>(buf, extraIndex);
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

// A property tag (classic layout): name, type, size, arrayIndex, [tagData], propertyGuidFlag(byte), value.
// hasPropertyGuid flag is a single byte (ReadFlag) since PROPERTY_GUID_IN_PROPERTY_TAG is satisfied.
static void AppendTag(std::vector<uint8_t>& buf, int32_t nameIdx, int32_t typeIdx,
                      const std::vector<uint8_t>& tagDataAndValue, int32_t valueSize)
{
    AppendFName(buf, nameIdx);            // Name
    AppendFName(buf, typeIdx);            // PropertyType
    AppendLE<int32_t>(buf, valueSize);    // Size (bytes of value only)
    AppendLE<int32_t>(buf, 0);            // ArrayIndex
    buf.insert(buf.end(), tagDataAndValue.begin(), tagDataAndValue.end()); // tag data (if any) then guid flag + value
}

int main()
{
    const int32_t UE4_AUTO = static_cast<int32_t>(EUnrealEngineObjectUE4Version::AUTOMATIC_VERSION);
    const VersionContainer VC(GAME_UE4_LATEST, ETexturePlatform::DesktopMobile, FPackageFileVersion(864, UE4_AUTO, 0));

    TestPackage pkg;
    // Name pool indices used below.
    pkg.Names = {
        Entry("None"),          // 0
        Entry("IntProperty"),   // 1
        Entry("BoolProperty"),  // 2
        Entry("FloatProperty"), // 3
        Entry("NameProperty"),  // 4
        Entry("StrProperty"),   // 5
        Entry("MyInt"),         // 6
        Entry("MyBool"),        // 7
        Entry("MyFloat"),       // 8
        Entry("MyName"),        // 9
        Entry("MyStr"),         // 10
        Entry("HelloName"),     // 11
    };

    std::vector<uint8_t> buf;

    // MyInt : IntProperty = 42.  No tag data; guid flag byte = 0; then int32 value.
    {
        std::vector<uint8_t> v;
        v.push_back(0);                 // hasPropertyGuid flag
        AppendLE<int32_t>(v, 42);       // value
        AppendTag(buf, 6, 1, v, 4);
    }
    // MyBool : BoolProperty = true.  Tag data is the bool flag byte (read by FPropertyTagData), value size 0.
    {
        std::vector<uint8_t> v;
        v.push_back(1);                 // FPropertyTagData bool (ReadFlag)
        v.push_back(0);                 // hasPropertyGuid flag
        // BoolProperty value comes from tag data, so on-disk value size is 0.
        AppendTag(buf, 7, 2, v, 0);
    }
    // MyFloat : FloatProperty = 1.5.
    {
        std::vector<uint8_t> v;
        v.push_back(0);
        AppendLE<float>(v, 1.5f);
        AppendTag(buf, 8, 3, v, 4);
    }
    // MyName : NameProperty = "HelloName" (index 11).
    {
        std::vector<uint8_t> v;
        v.push_back(0);
        AppendFName(v, 11);
        AppendTag(buf, 9, 4, v, 8);
    }
    // MyStr : StrProperty = "hi".
    {
        std::vector<uint8_t> v;
        v.push_back(0);
        std::vector<uint8_t> s;
        AppendFString(s, "hi");
        v.insert(v.end(), s.begin(), s.end());
        AppendTag(buf, 10, 5, v, static_cast<int32_t>(s.size()));
    }
    // Terminating "None" tag.
    AppendFName(buf, 0);

    FByteArchive base("props", buf, VC);
    FAssetArchive ar(base, &pkg);

    std::vector<FPropertyTag> props;
    UObject::DeserializePropertiesTagged(props, ar, false);

    CHECK(props.size() == 5);
    CHECK(ar.Position == static_cast<int64_t>(buf.size())); // consumed everything incl. the None terminator

    // Verify names/types.
    CHECK(props[0].Name.Text() == "MyInt" && props[0].PropertyType.Text() == "IntProperty");
    CHECK(props[1].Name.Text() == "MyBool" && props[1].PropertyType.Text() == "BoolProperty");
    CHECK(props[2].Name.Text() == "MyFloat");
    CHECK(props[3].Name.Text() == "MyName");
    CHECK(props[4].Name.Text() == "MyStr");

    // Verify parsed values via dynamic_cast to the concrete property type.
    auto* ip = dynamic_cast<IntProperty*>(props[0].Tag.get());
    CHECK(ip != nullptr && ip->Value == 42);
    auto* bp = dynamic_cast<BoolProperty*>(props[1].Tag.get());
    CHECK(bp != nullptr && bp->Value == true);
    auto* fp = dynamic_cast<FloatProperty*>(props[2].Tag.get());
    CHECK(fp != nullptr && fp->Value == 1.5f);
    auto* np = dynamic_cast<NameProperty*>(props[3].Tag.get());
    CHECK(np != nullptr && np->Value.Text() == "HelloName");
    auto* sp = dynamic_cast<StrProperty*>(props[4].Tag.get());
    CHECK(sp != nullptr && sp->Value == "hi");

    // ToString smoke check.
    CHECK(props[0].ToString() == "MyInt  -->  42 (IntProperty)");

    if (g_failures == 0)
    {
        std::cout << "All tagged-property tests passed.\n";
        return 0;
    }
    std::cout << g_failures << " check(s) failed.\n";
    return 1;
}
