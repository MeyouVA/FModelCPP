// Ported from CUE4Parse/Utils/StringUtils.cs
//
// The substring helpers are ordinal (std::string is byte-oriented), matching the C# default
// StringComparison.Ordinal overloads. The Span<char> overload and the culture-aware comparisonType
// parameters are dropped as they have no direct std::string equivalent. ParseAesKey/TryParseAesKey
// arrive with the Encryption/Aes layer (FAesKey not ported yet).
#pragma once

#include <string>

namespace CUE4Parse::Utils
{
    inline std::string SubstringBefore(const std::string& s, char delimiter)
    {
        auto index = s.find(delimiter);
        return index == std::string::npos ? s : s.substr(0, index);
    }

    inline std::string SubstringBefore(const std::string& s, const std::string& delimiter)
    {
        auto index = s.find(delimiter);
        return index == std::string::npos ? s : s.substr(0, index);
    }

    inline std::string SubstringAfter(const std::string& s, char delimiter)
    {
        auto index = s.find(delimiter);
        return index == std::string::npos ? s : s.substr(index + 1);
    }

    inline std::string SubstringAfter(const std::string& s, const std::string& delimiter)
    {
        auto index = s.find(delimiter);
        return index == std::string::npos ? s : s.substr(index + delimiter.size());
    }

    inline std::string SubstringBeforeLast(const std::string& s, char delimiter)
    {
        auto index = s.rfind(delimiter);
        return index == std::string::npos ? s : s.substr(0, index);
    }

    inline std::string SubstringBeforeWithLast(const std::string& s, char delimiter)
    {
        auto index = s.rfind(delimiter);
        return index == std::string::npos ? s : s.substr(0, index + 1);
    }

    inline std::string SubstringBeforeLast(const std::string& s, const std::string& delimiter)
    {
        auto index = s.rfind(delimiter);
        return index == std::string::npos ? s : s.substr(0, index);
    }

    inline std::string SubstringAfterLast(const std::string& s, char delimiter)
    {
        auto index = s.rfind(delimiter);
        return index == std::string::npos ? s : s.substr(index + 1);
    }

    inline std::string SubstringAfterWithLast(const std::string& s, char delimiter)
    {
        auto index = s.rfind(delimiter);
        return index == std::string::npos ? s : s.substr(index);
    }

    inline std::string SubstringAfterLast(const std::string& s, const std::string& delimiter)
    {
        auto index = s.rfind(delimiter);
        return index == std::string::npos ? s : s.substr(index + delimiter.size());
    }

    inline bool Contains(const std::string& orig, const std::string& value)
    {
        return orig.find(value) != std::string::npos;
    }
}
