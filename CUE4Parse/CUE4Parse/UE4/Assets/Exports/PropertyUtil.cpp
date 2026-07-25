#include "PropertyUtil.h"

#include <algorithm>

namespace CUE4Parse::UE4::Assets::Exports::PropertyUtil
{
    namespace
    {
        bool NameEquals(const std::string& a, const std::string& b, bool ignoreCase)
        {
            if (!ignoreCase) return a == b;
            if (a.size() != b.size()) return false;
            return std::equal(a.begin(), a.end(), b.begin(), [](char x, char y)
            {
                return std::tolower(static_cast<unsigned char>(x)) == std::tolower(static_cast<unsigned char>(y));
            });
        }
    }

    bool TryAnyInteger(const FPropertyTagType& tag, int64_t& out)
    {
        // Deliberately signedness-agnostic, unlike NarrowerInteger: an enum's stored value is whatever
        // width the property happened to use, and the enum's own underlying type decides the meaning.
        const auto probe = [&](auto sample) -> bool
        {
            using U = decltype(sample);
            if (const auto* p = dynamic_cast<const TPropertyTagType<U>*>(&tag))
            {
                out = static_cast<int64_t>(p->Value);
                return true;
            }
            return false;
        };
        return probe(uint8_t{}) || probe(int8_t{}) || probe(uint16_t{}) || probe(int16_t{}) ||
               probe(uint32_t{}) || probe(int32_t{}) || probe(uint64_t{}) || probe(int64_t{});
    }

    const FPropertyTag* FindTag(const std::vector<FPropertyTag>& properties, const std::string& name, bool ignoreCase)
    {
        // C# takes the FIRST match, which matters for a static array: the element at index 0 wins.
        for (const FPropertyTag& prop : properties)
        {
            if (NameEquals(prop.Name.Text(), name, ignoreCase)) return &prop;
        }
        return nullptr;
    }
}
