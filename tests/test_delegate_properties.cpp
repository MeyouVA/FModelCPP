// Tests the delegate / interface / field-path / string-variant property layer: build tagged
// DelegateProperty / Multicast[Inline/Sparse]DelegateProperty / InterfaceProperty / FieldPathProperty /
// AnsiStrProperty / Utf8StrProperty / VerseStringProperty blobs by hand, read them with
// UObject::DeserializePropertiesTagged, and verify the parsed values.
#include <cstdint>
#include <cstring>
#include <iostream>
#include <string>
#include <vector>

#include "UE4/Assets/Exports/UObject.h"
#include "UE4/Assets/Objects/FPropertyTag.h"
#include "UE4/Assets/Objects/Properties/FPropertyTagType.h"
#include "UE4/Assets/Objects/Properties/DelegateProperty.h"
#include "UE4/Assets/Objects/Properties/MulticastDelegateProperty.h"
#include "UE4/Assets/Objects/Properties/InterfaceProperty.h"
#include "UE4/Assets/Objects/Properties/FieldPathProperty.h"
#include "UE4/Assets/Objects/Properties/AnsiStrProperty.h"
#include "UE4/Assets/Objects/Properties/Utf8StrProperty.h"
#include "UE4/Assets/Objects/Properties/VerseStringProperty.h"
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

// A raw ANSI/UTF8 FString as the C++ reader expects: int32 byte-count + that many bytes (no null).
static void AppendRawString(std::vector<uint8_t>& buf, const std::string& s)
{
    AppendLE<int32_t>(buf, static_cast<int32_t>(s.size()));
    buf.insert(buf.end(), s.begin(), s.end());
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
        N_None = 0,
        N_DelegateProperty, N_MulticastDelegateProperty, N_MulticastInlineDelegateProperty,
        N_MulticastSparseDelegateProperty, N_InterfaceProperty, N_FieldPathProperty,
        N_AnsiStrProperty, N_Utf8StrProperty, N_VerseStringProperty,
        N_MyDelegate, N_MyMulticast, N_MyInline, N_MySparse, N_MyInterface, N_MyFieldPath,
        N_MyAnsi, N_MyUtf8, N_MyVerse,
        N_OnFire, N_Cb, N_MyField,
    };
    pkg.Names = {
        Entry("None"),                              // 0
        Entry("DelegateProperty"),                  // 1
        Entry("MulticastDelegateProperty"),         // 2
        Entry("MulticastInlineDelegateProperty"),   // 3
        Entry("MulticastSparseDelegateProperty"),   // 4
        Entry("InterfaceProperty"),                 // 5
        Entry("FieldPathProperty"),                 // 6
        Entry("AnsiStrProperty"),                   // 7
        Entry("Utf8StrProperty"),                   // 8
        Entry("VerseStringProperty"),               // 9
        Entry("MyDelegate"),                        // 10
        Entry("MyMulticast"),                       // 11
        Entry("MyInline"),                          // 12
        Entry("MySparse"),                          // 13
        Entry("MyInterface"),                       // 14
        Entry("MyFieldPath"),                       // 15
        Entry("MyAnsi"),                            // 16
        Entry("MyUtf8"),                            // 17
        Entry("MyVerse"),                           // 18
        Entry("OnFire"),                            // 19
        Entry("Cb"),                                // 20
        Entry("MyField"),                           // 21
    };

    std::vector<uint8_t> buf;

    auto appendDelegate = [](std::vector<uint8_t>& v, int32_t objIndex, int32_t fnNameIdx) {
        AppendLE<int32_t>(v, objIndex);
        AppendFName(v, fnNameIdx);
    };

    // --- MyDelegate : DelegateProperty = { Object=3, FunctionName="OnFire" } ---
    {
        std::vector<uint8_t> v;
        v.push_back(0);                      // guid flag (no tag data)
        std::vector<uint8_t> value;
        appendDelegate(value, 3, N_OnFire);
        v.insert(v.end(), value.begin(), value.end());
        AppendTag(buf, N_MyDelegate, N_DelegateProperty, v, static_cast<int32_t>(value.size()));
    }

    // Helper: a multicast value with one delegate { Object=1, FunctionName="Cb" }.
    auto oneDelegateMulticast = [&](std::vector<uint8_t>& value) {
        AppendLE<int32_t>(value, 1);         // invocation count
        appendDelegate(value, 1, N_Cb);
    };

    // --- MyMulticast : MulticastDelegateProperty (base) ---
    {
        std::vector<uint8_t> v; v.push_back(0);
        std::vector<uint8_t> value; oneDelegateMulticast(value);
        v.insert(v.end(), value.begin(), value.end());
        AppendTag(buf, N_MyMulticast, N_MulticastDelegateProperty, v, static_cast<int32_t>(value.size()));
    }

    // --- MyInline : MulticastInlineDelegateProperty ---
    {
        std::vector<uint8_t> v; v.push_back(0);
        std::vector<uint8_t> value; oneDelegateMulticast(value);
        v.insert(v.end(), value.begin(), value.end());
        AppendTag(buf, N_MyInline, N_MulticastInlineDelegateProperty, v, static_cast<int32_t>(value.size()));
    }

    // --- MySparse : MulticastSparseDelegateProperty ---
    {
        std::vector<uint8_t> v; v.push_back(0);
        std::vector<uint8_t> value; oneDelegateMulticast(value);
        v.insert(v.end(), value.begin(), value.end());
        AppendTag(buf, N_MySparse, N_MulticastSparseDelegateProperty, v, static_cast<int32_t>(value.size()));
    }

    // --- MyInterface : InterfaceProperty = { Object=-2 } ---
    {
        std::vector<uint8_t> v; v.push_back(0);
        AppendLE<int32_t>(v, -2);
        AppendTag(buf, N_MyInterface, N_InterfaceProperty, v, 4);
    }

    // --- MyFieldPath : FieldPathProperty = { Path=["MyField"], ResolvedOwner=4 } ---
    {
        std::vector<uint8_t> v; v.push_back(0);
        std::vector<uint8_t> value;
        AppendLE<int32_t>(value, 1);         // path length
        AppendFName(value, N_MyField);       // Path[0]
        AppendLE<int32_t>(value, 4);         // ResolvedOwner
        v.insert(v.end(), value.begin(), value.end());
        AppendTag(buf, N_MyFieldPath, N_FieldPathProperty, v, static_cast<int32_t>(value.size()));
    }

    // --- MyAnsi : AnsiStrProperty = "hello" ---
    {
        std::vector<uint8_t> v; v.push_back(0);
        std::vector<uint8_t> value; AppendRawString(value, "hello");
        v.insert(v.end(), value.begin(), value.end());
        AppendTag(buf, N_MyAnsi, N_AnsiStrProperty, v, static_cast<int32_t>(value.size()));
    }

    // --- MyUtf8 : Utf8StrProperty = "utf8" ---
    {
        std::vector<uint8_t> v; v.push_back(0);
        std::vector<uint8_t> value; AppendRawString(value, "utf8");
        v.insert(v.end(), value.begin(), value.end());
        AppendTag(buf, N_MyUtf8, N_Utf8StrProperty, v, static_cast<int32_t>(value.size()));
    }

    // --- MyVerse : VerseStringProperty = "verse" ---
    {
        std::vector<uint8_t> v; v.push_back(0);
        std::vector<uint8_t> value; AppendRawString(value, "verse");
        v.insert(v.end(), value.begin(), value.end());
        AppendTag(buf, N_MyVerse, N_VerseStringProperty, v, static_cast<int32_t>(value.size()));
    }

    // Terminating "None" tag.
    AppendFName(buf, N_None);

    FByteArchive base("delegate", buf, VC);
    FAssetArchive ar(base, &pkg);

    std::vector<FPropertyTag> props;
    UObject::DeserializePropertiesTagged(props, ar, false);

    CHECK(props.size() == 9);
    CHECK(ar.Position == static_cast<int64_t>(buf.size()));

    // MyDelegate
    auto* delProp = dynamic_cast<DelegateProperty*>(props[0].Tag.get());
    CHECK(delProp != nullptr);
    if (delProp)
    {
        CHECK(delProp->Value.Object.Index == 3);
        CHECK(delProp->Value.FunctionName.Text() == "OnFire");
    }
    CHECK(props[0].ToString() == "MyDelegate  -->  OnFire (DelegateProperty)");

    // MyMulticast (base) — TypeName should be the base name.
    auto* mcProp = dynamic_cast<MulticastDelegateProperty*>(props[1].Tag.get());
    CHECK(mcProp != nullptr);
    if (mcProp)
    {
        CHECK(mcProp->Value.InvocationList.size() == 1);
        if (!mcProp->Value.InvocationList.empty())
        {
            CHECK(mcProp->Value.InvocationList[0].Object.Index == 1);
            CHECK(mcProp->Value.InvocationList[0].FunctionName.Text() == "Cb");
        }
    }
    CHECK(props[1].ToString() == "MyMulticast  -->  [1 invocations] (MulticastDelegateProperty)");

    // MyInline — a subclass; both casts succeed, TypeName is the subclass name.
    CHECK(dynamic_cast<MulticastInlineDelegateProperty*>(props[2].Tag.get()) != nullptr);
    CHECK(dynamic_cast<MulticastDelegateProperty*>(props[2].Tag.get()) != nullptr);
    CHECK(EndsWith(props[2].ToString(), " (MulticastInlineDelegateProperty)"));

    // MySparse
    CHECK(dynamic_cast<MulticastSparseDelegateProperty*>(props[3].Tag.get()) != nullptr);
    CHECK(dynamic_cast<MulticastDelegateProperty*>(props[3].Tag.get()) != nullptr);
    CHECK(EndsWith(props[3].ToString(), " (MulticastSparseDelegateProperty)"));

    // MyInterface
    auto* ifProp = dynamic_cast<InterfaceProperty*>(props[4].Tag.get());
    CHECK(ifProp != nullptr);
    if (ifProp)
    {
        CHECK(ifProp->Value.Object.Index == -2);
        CHECK(ifProp->Value.Object.IsImport());
    }
    CHECK(EndsWith(props[4].ToString(), " (InterfaceProperty)"));

    // MyFieldPath
    auto* fpProp = dynamic_cast<FieldPathProperty*>(props[5].Tag.get());
    CHECK(fpProp != nullptr);
    if (fpProp)
    {
        CHECK(fpProp->Value.Path.size() == 1);
        if (!fpProp->Value.Path.empty()) CHECK(fpProp->Value.Path[0].Text() == "MyField");
        CHECK(fpProp->Value.ResolvedOwner.Index == 4);
    }
    CHECK(props[5].ToString() == "MyFieldPath  -->  MyField (FieldPathProperty)");

    // MyAnsi — an AnsiStrProperty is-a StrProperty.
    auto* ansiProp = dynamic_cast<AnsiStrProperty*>(props[6].Tag.get());
    CHECK(ansiProp != nullptr);
    CHECK(dynamic_cast<StrProperty*>(props[6].Tag.get()) != nullptr);
    if (ansiProp) CHECK(ansiProp->Value == "hello");
    CHECK(props[6].ToString() == "MyAnsi  -->  hello (AnsiStrProperty)");

    // MyUtf8
    auto* utf8Prop = dynamic_cast<Utf8StrProperty*>(props[7].Tag.get());
    CHECK(utf8Prop != nullptr);
    if (utf8Prop) CHECK(utf8Prop->Value == "utf8");
    CHECK(props[7].ToString() == "MyUtf8  -->  utf8 (Utf8StrProperty)");

    // MyVerse
    auto* verseProp = dynamic_cast<VerseStringProperty*>(props[8].Tag.get());
    CHECK(verseProp != nullptr);
    if (verseProp) CHECK(verseProp->Value == "verse");
    CHECK(props[8].ToString() == "MyVerse  -->  verse (VerseStringProperty)");

    if (g_failures == 0)
    {
        std::cout << "All delegate/interface/field-path/string-variant property tests passed.\n";
        return 0;
    }
    std::cout << g_failures << " check(s) failed.\n";
    return 1;
}
