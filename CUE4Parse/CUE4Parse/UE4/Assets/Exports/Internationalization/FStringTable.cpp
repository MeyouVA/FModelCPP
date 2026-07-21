// Ported from CUE4Parse/UE4/Assets/Exports/Internationalization/FStringTable.cs.
#include "FStringTable.h"

#include "../../Readers/FAssetArchive.h"

namespace CUE4Parse::UE4::Assets::Exports::Internationalization
{
    using Readers::FAssetArchive;

    FStringTable::FStringTable(FAssetArchive& Ar)
    {
        TableNamespace = Ar.ReadFString();

        // C#: KeysToEntries = Ar.ReadMap(Ar.ReadFString, () => Ar.ReadFString()) — only the default (non
        // per-game) value path is ported.
        const int32_t entryCount = Ar.Read<int32_t>();
        for (int32_t i = 0; i < entryCount; i++)
        {
            std::string key = Ar.ReadFString();
            KeysToEntries[std::move(key)] = Ar.ReadFString();
        }

        // C#: KeysToMetaData = Ar.ReadMap(Ar.ReadFString, () => Ar.ReadMap(Ar.ReadFName, Ar.ReadFString)).
        std::map<std::string, std::map<std::string, std::string>> meta;
        const int32_t metaCount = Ar.Read<int32_t>();
        for (int32_t i = 0; i < metaCount; i++)
        {
            std::string key = Ar.ReadFString();
            std::map<std::string, std::string> inner;
            const int32_t innerCount = Ar.Read<int32_t>();
            for (int32_t j = 0; j < innerCount; j++)
            {
                std::string metaKey = Ar.ReadFName().Text();
                inner[std::move(metaKey)] = Ar.ReadFString();
            }
            meta[std::move(key)] = std::move(inner);
        }
        KeysToMetaData = std::move(meta);
    }
}
