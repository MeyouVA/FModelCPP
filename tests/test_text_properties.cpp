// Tests the Text / Optional property layer: build tagged TextProperty (None + Base history) and
// OptionalProperty (present + absent) blobs by hand, read them with UObject::DeserializePropertiesTagged,
// and verify the parsed values.
#include <cstdint>
#include <cstring>
#include <iostream>
#include <string>
#include <vector>

#include "UE4/Assets/Exports/UObject.h"
#include "UE4/Assets/Objects/FPropertyTag.h"
#include "UE4/Assets/Objects/Properties/FPropertyTagType.h"
#include "UE4/Assets/Objects/Properties/TextProperty.h"
#include "UE4/Assets/Objects/Properties/OptionalProperty.h"
#include "UE4/Assets/Objects/Properties/IntProperty.h"
#include "UE4/Objects/Core/i18N/FText.h"
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
namespace i18N = CUE4Parse::UE4::Objects::Core::i18N;
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
        N_None = 0, N_TextProperty, N_OptionalProperty, N_IntProperty,
        N_MyText, N_MyBase, N_MyOptInt, N_MyOptEmpty,
    };
    pkg.Names = {
        Entry("None"),              // 0
        Entry("TextProperty"),      // 1
        Entry("OptionalProperty"),  // 2
        Entry("IntProperty"),       // 3
        Entry("MyText"),            // 4
        Entry("MyBase"),            // 5
        Entry("MyOptInt"),          // 6
        Entry("MyOptEmpty"),        // 7
    };

    std::vector<uint8_t> buf;

    // --- MyText : TextProperty (None history, culture-invariant "HelloText") ---
    {
        std::vector<uint8_t> v; v.push_back(0); // guid flag (no tag data)
        std::vector<uint8_t> value;
        AppendLE<uint32_t>(value, 0);                        // Flags
        AppendLE<int8_t>(value, static_cast<int8_t>(-1));    // HistoryType = None
        AppendLE<int32_t>(value, 1);                         // bHasCultureInvariantString (ReadBoolean = int32)
        AppendFString(value, "HelloText");
        v.insert(v.end(), value.begin(), value.end());
        AppendTag(buf, N_MyText, N_TextProperty, v, static_cast<int32_t>(value.size()));
    }

    // --- MyBase : TextProperty (Base history) ---
    {
        std::vector<uint8_t> v; v.push_back(0);
        std::vector<uint8_t> value;
        AppendLE<uint32_t>(value, 0);                        // Flags
        AppendLE<int8_t>(value, static_cast<int8_t>(0));     // HistoryType = Base
        AppendFString(value, "NS");                          // Namespace
        AppendFString(value, "KEY");                         // Key
        AppendFString(value, "SourceStr");                   // SourceString
        AppendFString(value, "dev");                         // dev notes (read since !IsFilterEditorOnly)
        v.insert(v.end(), value.begin(), value.end());
        AppendTag(buf, N_MyBase, N_TextProperty, v, static_cast<int32_t>(value.size()));
    }

    // --- MyOptInt : OptionalProperty<IntProperty> present, inner Int=42 ---
    {
        std::vector<uint8_t> v;
        AppendFName(v, N_IntProperty);   // tag data: inner type
        v.push_back(0);                  // guid flag
        std::vector<uint8_t> value;
        AppendLE<int32_t>(value, 1);     // presence (ReadBoolean = int32)
        AppendLE<int32_t>(value, 42);    // inner IntProperty value
        v.insert(v.end(), value.begin(), value.end());
        AppendTag(buf, N_MyOptInt, N_OptionalProperty, v, static_cast<int32_t>(value.size()));
    }

    // --- MyOptEmpty : OptionalProperty absent ---
    {
        std::vector<uint8_t> v;
        AppendFName(v, N_IntProperty);   // tag data: inner type
        v.push_back(0);                  // guid flag
        std::vector<uint8_t> value;
        AppendLE<int32_t>(value, 0);     // presence = false
        v.insert(v.end(), value.begin(), value.end());
        AppendTag(buf, N_MyOptEmpty, N_OptionalProperty, v, static_cast<int32_t>(value.size()));
    }

    // Terminating "None" tag.
    AppendFName(buf, N_None);

    FByteArchive base("text", buf, VC);
    FAssetArchive ar(base, &pkg);

    std::vector<FPropertyTag> props;
    UObject::DeserializePropertiesTagged(props, ar, false);

    CHECK(props.size() == 4);
    CHECK(ar.Position == static_cast<int64_t>(buf.size()));

    // MyText (None history)
    auto* textProp = dynamic_cast<TextProperty*>(props[0].Tag.get());
    CHECK(textProp != nullptr);
    if (textProp)
    {
        CHECK(textProp->Value.HistoryType == i18N::ETextHistoryType::None);
        CHECK(textProp->Value.Text() == "HelloText");
        auto* none = dynamic_cast<i18N::FTextHistory::None*>(textProp->Value.TextHistory.get());
        CHECK(none != nullptr);
        if (none) CHECK(none->CultureInvariantString.has_value() && *none->CultureInvariantString == "HelloText");
    }
    CHECK(props[0].ToString() == "MyText  -->  HelloText (TextProperty)");

    // MyBase (Base history) — LocalizedString stays empty without a provider.
    auto* baseProp = dynamic_cast<TextProperty*>(props[1].Tag.get());
    CHECK(baseProp != nullptr);
    if (baseProp)
    {
        CHECK(baseProp->Value.HistoryType == i18N::ETextHistoryType::Base);
        auto* baseHist = dynamic_cast<i18N::FTextHistory::Base*>(baseProp->Value.TextHistory.get());
        CHECK(baseHist != nullptr);
        if (baseHist)
        {
            CHECK(baseHist->Namespace == "NS");
            CHECK(baseHist->Key == "KEY");
            CHECK(baseHist->SourceString == "SourceStr");
            CHECK(baseHist->LocalizedString.empty());
        }
        CHECK(baseProp->Value.Text().empty());
    }

    // MyOptInt (present)
    auto* optInt = dynamic_cast<OptionalProperty*>(props[2].Tag.get());
    CHECK(optInt != nullptr);
    if (optInt)
    {
        CHECK(optInt->Value != nullptr);
        auto* inner = dynamic_cast<IntProperty*>(optInt->Value.get());
        CHECK(inner != nullptr);
        if (inner) CHECK(inner->Value == 42);
    }
    CHECK(props[2].ToString() == "MyOptInt  -->  42 (IntProperty) (OptionalProperty)");

    // MyOptEmpty (absent) — null inner, empty ToString.
    auto* optEmpty = dynamic_cast<OptionalProperty*>(props[3].Tag.get());
    CHECK(optEmpty != nullptr);
    if (optEmpty) CHECK(optEmpty->Value == nullptr);
    CHECK(props[3].ToString() == "MyOptEmpty  -->  ");

    if (g_failures == 0)
    {
        std::cout << "All text/optional property tests passed.\n";
        return 0;
    }
    std::cout << g_failures << " check(s) failed.\n";
    return 1;
}
