// Ported from CUE4Parse/UE4/Objects/UObject/FName.cs
// A name reference: either compared by its resolved text (classic packages) or by its index into a name
// pool (IO Store / name-map path). The resolved entry is always stored, so both paths can print the text.
//
// Deliberate difference: OrdinalIgnoreCase text comparison is done as ASCII case folding (names are ASCII
// in practice); the .NET culture-invariant Unicode case fold is not reproduced. The C# JSON converter,
// GetHashCode, and IComparable boilerplate are omitted; CompareTo is kept for ordering.
#pragma once

#include <cstdint>
#include <string>
#include <utility>
#include <vector>

#include "FNameEntrySerialized.h"
#include "../../IO/Objects/FMappedName.h"

namespace CUE4Parse::UE4::Objects::UObject
{
    enum class FNameComparisonMethod : uint8_t
    {
        Text,
        Index
    };

    struct FName
    {
        // Index into the Names array (used with the Index comparison method).
        int32_t Index = 0;
        // Number portion of the string/number pair (stored as 1 more than actual, so 0 == default/no-instance).
        int32_t Number = 0;
        FNameComparisonMethod ComparisonMethod = FNameComparisonMethod::Text;

        FName() = default;

        FName(std::optional<std::string> name, int32_t index = 0, int32_t number = 0,
              FNameComparisonMethod compare = FNameComparisonMethod::Text)
            : Index(index), Number(number), ComparisonMethod(compare), _name(std::move(name)) {}

        FName(FNameEntrySerialized name, int32_t index, int32_t number,
              FNameComparisonMethod compare = FNameComparisonMethod::Index)
            : Index(index), Number(number), ComparisonMethod(compare), _name(std::move(name)) {}

        FName(const std::vector<FNameEntrySerialized>& nameMap, int32_t index, int32_t number,
              FNameComparisonMethod compare = FNameComparisonMethod::Index)
            : FName(nameMap[static_cast<size_t>(index)], index, number, compare) {}

        FName(IO::Objects::FMappedName mappedName, const std::vector<FNameEntrySerialized>& nameMap,
              FNameComparisonMethod compare = FNameComparisonMethod::Index)
            : FName(nameMap, static_cast<int32_t>(mappedName.NameIndex()), static_cast<int32_t>(mappedName.ExtraIndex), compare) {}

        const std::string& PlainText() const { return _name.Name.has_value() ? *_name.Name : None; }

        std::string Text() const
        {
            return Number == 0 ? PlainText() : PlainText() + "_" + std::to_string(Number - 1);
        }

        bool IsNone() const { return Text() == "None"; }

        std::string ToString() const { return Text(); }

        // Case sensitive in the editor, case insensitive in runtime — the runtime (OrdinalIgnoreCase) path.
        bool operator==(const FName& other) const
        {
            switch (ComparisonMethod)
            {
                case FNameComparisonMethod::Index: return Index == other.Index && Number == other.Number;
                case FNameComparisonMethod::Text:  return EqualsIgnoreCase(Text(), other.Text());
            }
            return false;
        }
        bool operator!=(const FName& other) const { return !(*this == other); }

        bool operator==(int32_t b) const { return Index == b; }
        bool operator!=(int32_t b) const { return Index != b; }
        bool operator==(uint32_t b) const { return static_cast<uint32_t>(Index) == b; }
        bool operator!=(uint32_t b) const { return static_cast<uint32_t>(Index) != b; }

        int CompareTo(const FName& other) const { return CompareIgnoreCase(Text(), other.Text()); }

    private:
        FNameEntrySerialized _name;
        static inline const std::string None = "None";

        static char FoldAscii(char c) { return (c >= 'a' && c <= 'z') ? static_cast<char>(c - ('a' - 'A')) : c; }

        static bool EqualsIgnoreCase(const std::string& a, const std::string& b)
        {
            if (a.size() != b.size()) return false;
            for (size_t i = 0; i < a.size(); i++)
                if (FoldAscii(a[i]) != FoldAscii(b[i])) return false;
            return true;
        }

        static int CompareIgnoreCase(const std::string& a, const std::string& b)
        {
            const size_t n = a.size() < b.size() ? a.size() : b.size();
            for (size_t i = 0; i < n; i++)
            {
                const unsigned char ca = static_cast<unsigned char>(FoldAscii(a[i]));
                const unsigned char cb = static_cast<unsigned char>(FoldAscii(b[i]));
                if (ca != cb) return ca < cb ? -1 : 1;
            }
            if (a.size() == b.size()) return 0;
            return a.size() < b.size() ? -1 : 1;
        }
    };
}
