// Ported from CUE4Parse/UE4/Objects/UObject/FNameEntrySerialized.cs (FArchive-dependent members).
#include "FNameEntrySerialized.h"

#include "../../Readers/FArchive.h"
#include "../../Versions/ObjectVersion.h"
#include "../../Versions/EGame.h"
#include "../../Assets/Exports/EObjectFlags.h"

namespace CUE4Parse::UE4::Objects::UObject
{
    using namespace CUE4Parse::UE4::Versions;
    using Readers::FArchive;

    namespace
    {
        // Latin-1 (ISO-8859-1) -> UTF-8 (mirrors FArchive.cpp's helper).
        std::string Latin1ToUtf8(const uint8_t* data, size_t len)
        {
            std::string out;
            out.reserve(len);
            for (size_t i = 0; i < len; i++)
            {
                const uint8_t c = data[i];
                if (c < 0x80) { out.push_back(static_cast<char>(c)); }
                else
                {
                    out.push_back(static_cast<char>(0xC0 | (c >> 6)));
                    out.push_back(static_cast<char>(0x80 | (c & 0x3F)));
                }
            }
            return out;
        }

        // UTF-16LE -> UTF-8 (handles surrogate pairs). units is the number of 16-bit code units.
        std::string Utf16LeToUtf8(const uint8_t* bytes, size_t units)
        {
            std::string out;
            out.reserve(units);
            for (size_t i = 0; i < units; i++)
            {
                uint32_t cp = static_cast<uint32_t>(bytes[i * 2]) | (static_cast<uint32_t>(bytes[i * 2 + 1]) << 8);
                if (cp >= 0xD800 && cp <= 0xDBFF && i + 1 < units)
                {
                    const uint32_t lo = static_cast<uint32_t>(bytes[(i + 1) * 2]) | (static_cast<uint32_t>(bytes[(i + 1) * 2 + 1]) << 8);
                    if (lo >= 0xDC00 && lo <= 0xDFFF)
                    {
                        cp = 0x10000 + ((cp - 0xD800) << 10) + (lo - 0xDC00);
                        i++;
                    }
                }

                if (cp < 0x80) { out.push_back(static_cast<char>(cp)); }
                else if (cp < 0x800)
                {
                    out.push_back(static_cast<char>(0xC0 | (cp >> 6)));
                    out.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
                }
                else if (cp < 0x10000)
                {
                    out.push_back(static_cast<char>(0xE0 | (cp >> 12)));
                    out.push_back(static_cast<char>(0x80 | ((cp >> 6) & 0x3F)));
                    out.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
                }
                else
                {
                    out.push_back(static_cast<char>(0xF0 | (cp >> 18)));
                    out.push_back(static_cast<char>(0x80 | ((cp >> 12) & 0x3F)));
                    out.push_back(static_cast<char>(0x80 | ((cp >> 6) & 0x3F)));
                    out.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
                }
            }
            return out;
        }

        // C#'s string.Trim() — here ASCII whitespace only (see the header's deliberate-difference note).
        std::string TrimAscii(std::string s)
        {
            const auto isWs = [](char c) { return c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == '\f' || c == '\v'; };
            size_t begin = 0, end = s.size();
            while (begin < end && isWs(s[begin])) ++begin;
            while (end > begin && isWs(s[end - 1])) --end;
            return s.substr(begin, end - begin);
        }
    }

    FNameEntrySerialized::FNameEntrySerialized(FArchive& Ar)
    {
        const bool bHasNameHashes = Ar.Ver() >= EUnrealEngineObjectUE4Version::NAME_HASHES_SERIALIZED
            || Ar.Game() == GAME_GearsOfWar4 || Ar.Game() == GAME_DaysGone;

        Name = TrimAscii(Ar.ReadFString());

        // TODO: PlayerUnknownsBattlegrounds remaps names through an embedded PUBGNameHashMap.json resource,
        // which is not vendored in this port (matches other deferred embedded-resource lookups).

        if (Ar.Game() < GAME_UE4_0)
        {
            // flags — read and discarded (C# `_ = ...`).
            if (Ar.Ver() >= EUnrealEngineObjectUE3Version::Use64BitFlag)
                Ar.Read<int64_t>();
            else
                Ar.Read<Assets::Exports::EObjectFlags>();
        }

        if (bHasNameHashes)
        {
            // NAME_HASHES is not defined in this port; skip the two ushort hashes (the C# #else path).
            Ar.Position += 4;
        }
    }

    std::vector<FNameEntrySerialized> FNameEntrySerialized::LoadNameBatch(FArchive& nameAr, int nameCount)
    {
        std::vector<FNameEntrySerialized> result;
        result.reserve(static_cast<size_t>(nameCount));
        for (int i = 0; i < nameCount; i++)
            result.push_back(LoadNameHeader(nameAr));
        return result;
    }

    std::vector<FNameEntrySerialized> FNameEntrySerialized::LoadNameBatch(FArchive& Ar)
    {
        const int num = Ar.Read<int32_t>();
        if (num == 0) return {};

        Ar.Position += sizeof(uint32_t);          // numStringBytes
        Ar.Position += sizeof(uint64_t);          // hashVersion
        Ar.Position += static_cast<int64_t>(num) * sizeof(uint64_t); // hashes

        const auto headers = Ar.ReadArray<FSerializedNameHeader>(num);
        std::vector<FNameEntrySerialized> entries;
        entries.reserve(static_cast<size_t>(num));
        for (int i = 0; i < num; i++)
        {
            const FSerializedNameHeader& header = headers[i];
            const int length = static_cast<int>(header.Length());
            if (header.IsUtf16())
            {
                const auto bytes = Ar.ReadBytes(length * 2);
                entries.emplace_back(Utf16LeToUtf8(bytes.data(), static_cast<size_t>(length)));
            }
            else
            {
                const auto bytes = Ar.ReadBytes(length);
                // C# decodes this batch variant as UTF-8; our std::string is already UTF-8.
                entries.emplace_back(std::string(reinterpret_cast<const char*>(bytes.data()), static_cast<size_t>(length)));
            }
        }

        return entries;
    }

    FNameEntrySerialized FNameEntrySerialized::LoadNameHeader(FArchive& Ar)
    {
        const auto header = Ar.Read<FSerializedNameHeader>();
        const int length = static_cast<int>(header.Length());

        if (header.IsUtf16())
        {
            if (Ar.Position % 2 == 1) Ar.Position++;
            const auto bytes = Ar.ReadBytes(length * 2);
            return FNameEntrySerialized(Utf16LeToUtf8(bytes.data(), static_cast<size_t>(length)));
        }

        const auto bytes = Ar.ReadBytes(length);
        return FNameEntrySerialized(Latin1ToUtf8(bytes.data(), static_cast<size_t>(length)));
    }
}
