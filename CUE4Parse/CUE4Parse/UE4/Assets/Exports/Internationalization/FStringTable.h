// Ported from CUE4Parse/UE4/Assets/Exports/Internationalization/FStringTable.cs.
// The namespace + key->string table serialized inside a UStringTable export.
//
// Deliberate differences from C#:
//   * KeysToMetaData's inner key is C#'s FName; we key it by the name's text (std::string) since the metadata
//     is only consumed via string keys here.
//   * The per-game value quirks (CodeVein2 decryption, MarvelRivals / LostRecordsBloomAndRage extra strings,
//     Wildgate skipping the metadata map entirely) are game-specific overrides and omitted, like other
//     per-game branches elsewhere in the port. TODO.
#pragma once

#include <map>
#include <optional>
#include <string>

namespace CUE4Parse::UE4::Assets::Readers { class FAssetArchive; }

namespace CUE4Parse::UE4::Assets::Exports::Internationalization
{
    class FStringTable
    {
    public:
        std::string TableNamespace;
        std::map<std::string, std::string> KeysToEntries;
        std::optional<std::map<std::string, std::map<std::string, std::string>>> KeysToMetaData;

        FStringTable() = default;
        explicit FStringTable(Readers::FAssetArchive& Ar);
    };
}
