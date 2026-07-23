// Tests the aggregate-property layer: build tagged StructProperty / ArrayProperty (of scalars and of structs)
// blobs by hand, read them with UObject::DeserializePropertiesTagged, and verify the nested values.
#include <cstdint>
#include <cstring>
#include <iostream>
#include <string>
#include <vector>

#include "UE4/Assets/Exports/UObject.h"
#include "UE4/Assets/Objects/FPropertyTag.h"
#include "UE4/Assets/Objects/FScriptStruct.h"
#include "UE4/Assets/Objects/FStructFallback.h"
#include "UE4/Assets/Objects/UScriptArray.h"
#include "UE4/Assets/Objects/Properties/FPropertyTagType.h"
#include "UE4/Assets/Objects/Properties/IntProperty.h"
#include "UE4/Assets/Objects/Properties/StructProperty.h"
#include "UE4/Assets/Objects/Properties/ArrayProperty.h"
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

// FName reference in an asset archive: (nameIndex, extraIndex).
static void AppendFName(std::vector<uint8_t>& buf, int32_t nameIndex, int32_t extraIndex = 0)
{
    AppendLE<int32_t>(buf, nameIndex);
    AppendLE<int32_t>(buf, extraIndex);
}

// StructProperty tag data (classic, all UE4 gates on): FName(structType) + FGuid(16 bytes).
static void AppendStructTagData(std::vector<uint8_t>& buf, int32_t structTypeIdx)
{
    AppendFName(buf, structTypeIdx);
    for (int i = 0; i < 16; i++) buf.push_back(0); // StructGuid
}

// A full classic property tag: Name, Type, Size(value bytes), ArrayIndex, [tagData], guidFlag(byte), value.
static void AppendTag(std::vector<uint8_t>& buf, int32_t nameIdx, int32_t typeIdx,
                      const std::vector<uint8_t>& tagDataAndValue, int32_t valueSize)
{
    AppendFName(buf, nameIdx);
    AppendFName(buf, typeIdx);
    AppendLE<int32_t>(buf, valueSize);
    AppendLE<int32_t>(buf, 0);
    buf.insert(buf.end(), tagDataAndValue.begin(), tagDataAndValue.end());
}

// A self-contained IntProperty tag (no tag data): 29 bytes.
static void AppendIntTag(std::vector<uint8_t>& v, int32_t nameIdx, int32_t intTypeIdx, int32_t value)
{
    std::vector<uint8_t> inner;
    inner.push_back(0);                 // guid flag
    AppendLE<int32_t>(inner, value);    // value
    AppendTag(v, nameIdx, intTypeIdx, inner, 4);
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
        N_None = 0, N_StructProperty, N_ArrayProperty, N_IntProperty,
        N_MyStruct, N_MyIntArray, N_MyStructArray, N_InnerInt, N_StructA, N_Elem,
    };
    pkg.Names = {
        Entry("None"),            // 0
        Entry("StructProperty"),  // 1
        Entry("ArrayProperty"),   // 2
        Entry("IntProperty"),     // 3
        Entry("MyStruct"),        // 4
        Entry("MyIntArray"),      // 5
        Entry("MyStructArray"),   // 6
        Entry("InnerInt"),        // 7
        Entry("StructA"),         // 8
        Entry("Elem"),            // 9
    };

    std::vector<uint8_t> buf;

    // --- MyStruct : StructProperty { InnerInt : IntProperty = 7 } ---
    {
        std::vector<uint8_t> v;
        AppendStructTagData(v, N_StructA);   // tag data: struct type + guid
        v.push_back(0);                      // guid flag
        std::vector<uint8_t> value;
        AppendIntTag(value, N_InnerInt, N_IntProperty, 7); // nested property
        AppendFName(value, N_None);          // struct terminator
        v.insert(v.end(), value.begin(), value.end());
        AppendTag(buf, N_MyStruct, N_StructProperty, v, static_cast<int32_t>(value.size()));
    }

    // --- MyIntArray : ArrayProperty<IntProperty> = [10, 20, 30] ---
    {
        std::vector<uint8_t> v;
        AppendFName(v, N_IntProperty);       // tag data: inner type
        v.push_back(0);                      // guid flag
        std::vector<uint8_t> value;
        AppendLE<int32_t>(value, 3);         // element count
        AppendLE<int32_t>(value, 10);
        AppendLE<int32_t>(value, 20);
        AppendLE<int32_t>(value, 30);
        v.insert(v.end(), value.begin(), value.end());
        AppendTag(buf, N_MyIntArray, N_ArrayProperty, v, static_cast<int32_t>(value.size()));
    }

    // --- MyStructArray : ArrayProperty<StructProperty> = [ {Elem=100}, {Elem=200} ] ---
    {
        std::vector<uint8_t> v;
        AppendFName(v, N_StructProperty);    // tag data: inner type = StructProperty
        v.push_back(0);                      // guid flag
        std::vector<uint8_t> value;
        AppendLE<int32_t>(value, 2);         // element count
        // Inline inner tag (INNER_ARRAY_TAG_INFO, read with readData=false): carries the element struct type.
        AppendFName(value, N_MyStructArray); // inner tag Name
        AppendFName(value, N_StructProperty);// inner tag PropertyType
        AppendLE<int32_t>(value, 0);         // inner tag Size
        AppendLE<int32_t>(value, 0);         // inner tag ArrayIndex
        AppendStructTagData(value, N_StructA);// inner tag StructProperty tag data
        value.push_back(0);                  // inner tag guid flag
        // Element structs (each an FStructFallback: tagged props + None).
        for (int32_t elem : {100, 200})
        {
            AppendIntTag(value, N_Elem, N_IntProperty, elem);
            AppendFName(value, N_None);
        }
        v.insert(v.end(), value.begin(), value.end());
        AppendTag(buf, N_MyStructArray, N_ArrayProperty, v, static_cast<int32_t>(value.size()));
    }

    // Terminating "None" tag.
    AppendFName(buf, N_None);

    FByteArchive base("aggregate", buf, VC);
    FAssetArchive ar(base, &pkg);

    std::vector<FPropertyTag> props;
    UObject::DeserializePropertiesTagged(props, ar, false);

    CHECK(props.size() == 3);
    CHECK(ar.Position == static_cast<int64_t>(buf.size())); // consumed everything incl. the None terminator

    // MyStruct
    CHECK(props[0].Name.Text() == "MyStruct" && props[0].PropertyType.Text() == "StructProperty");
    auto* structProp = dynamic_cast<StructProperty*>(props[0].Tag.get());
    CHECK(structProp != nullptr);
    if (structProp && structProp->Value.AsFallback())
    {
        auto& inner = structProp->Value.AsFallback()->Properties;
        CHECK(inner.size() == 1);
        auto* innerInt = inner.empty() ? nullptr : dynamic_cast<IntProperty*>(inner[0].Tag.get());
        CHECK(innerInt != nullptr && innerInt->Value == 7);
        CHECK(inner.empty() ? false : inner[0].Name.Text() == "InnerInt");
    }
    CHECK(props[0].ToString() == "MyStruct  -->  [1 properties] (FStructFallback, StructProperty)");

    // MyIntArray
    CHECK(props[1].Name.Text() == "MyIntArray" && props[1].PropertyType.Text() == "ArrayProperty");
    auto* intArray = dynamic_cast<ArrayProperty*>(props[1].Tag.get());
    CHECK(intArray != nullptr);
    if (intArray)
    {
        CHECK(intArray->Value.InnerType == "IntProperty");
        CHECK(intArray->Value.Properties.size() == 3);
        const int32_t expected[3] = {10, 20, 30};
        for (size_t i = 0; i < intArray->Value.Properties.size() && i < 3; i++)
        {
            auto* e = dynamic_cast<IntProperty*>(intArray->Value.Properties[i].get());
            CHECK(e != nullptr && e->Value == expected[i]);
        }
    }
    CHECK(props[1].ToString() == "MyIntArray  -->  IntProperty[3] (ArrayProperty)");

    // MyStructArray
    CHECK(props[2].Name.Text() == "MyStructArray");
    auto* structArray = dynamic_cast<ArrayProperty*>(props[2].Tag.get());
    CHECK(structArray != nullptr);
    if (structArray)
    {
        CHECK(structArray->Value.InnerType == "StructProperty");
        CHECK(structArray->Value.Properties.size() == 2);
        const int32_t expected[2] = {100, 200};
        for (size_t i = 0; i < structArray->Value.Properties.size() && i < 2; i++)
        {
            auto* sp = dynamic_cast<StructProperty*>(structArray->Value.Properties[i].get());
            CHECK(sp != nullptr);
            if (sp && sp->Value.AsFallback() && !sp->Value.AsFallback()->Properties.empty())
            {
                auto* e = dynamic_cast<IntProperty*>(sp->Value.AsFallback()->Properties[0].Tag.get());
                CHECK(e != nullptr && e->Value == expected[i]);
            }
            else
            {
                CHECK(false); // struct element missing its property
            }
        }
    }

    if (g_failures == 0)
    {
        std::cout << "All aggregate-property tests passed.\n";
        return 0;
    }
    std::cout << g_failures << " check(s) failed.\n";
    return 1;
}
