// No C# counterpart file: this stands in for System.StringComparer, which the VFS layer threads through
// Mount(StringComparer pathComparer) to decide whether pak paths compare case-sensitively.
//
// Modelled as a stateful std::map comparator rather than an interface, because that is what a C++ ordered
// map needs: `std::map<std::string, T, StringComparer>` reproduces C#'s `Dictionary<string, T>(comparer)`
// including the "last write wins on a case-insensitive collision" behaviour paths depend on.
#pragma once

#include <cctype>
#include <string>

namespace CUE4Parse::Utils
{
    class StringComparer
    {
    public:
        bool IgnoreCase = false;

        StringComparer() = default;
        explicit StringComparer(bool ignoreCase) : IgnoreCase(ignoreCase) {}

        static StringComparer Ordinal() { return StringComparer(false); }
        static StringComparer OrdinalIgnoreCase() { return StringComparer(true); }

        // Strict-weak ordering, so this doubles as the map comparator.
        bool operator()(const std::string& a, const std::string& b) const { return Compare(a, b) < 0; }

        int Compare(const std::string& a, const std::string& b) const
        {
            if (!IgnoreCase) return a.compare(b) < 0 ? -1 : (a == b ? 0 : 1);

            const size_t n = a.size() < b.size() ? a.size() : b.size();
            for (size_t i = 0; i < n; ++i)
            {
                const unsigned char ca = Upper(a[i]);
                const unsigned char cb = Upper(b[i]);
                if (ca != cb) return ca < cb ? -1 : 1;
            }
            if (a.size() == b.size()) return 0;
            return a.size() < b.size() ? -1 : 1;
        }

        bool Equals(const std::string& a, const std::string& b) const { return Compare(a, b) == 0; }

    private:
        // ASCII-only, like StringComparer.OrdinalIgnoreCase for the ASCII range; pak paths are ASCII in
        // practice and a locale-aware fold would make the ordering depend on the process locale.
        static unsigned char Upper(char c)
        {
            const unsigned char u = static_cast<unsigned char>(c);
            return (u >= 'a' && u <= 'z') ? static_cast<unsigned char>(u - 32) : u;
        }
    };
}
