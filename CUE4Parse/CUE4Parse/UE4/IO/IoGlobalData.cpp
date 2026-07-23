// Ported from CUE4Parse/UE4/IO/IoGlobalData.cs
#include "IoGlobalData.h"

#include <stdexcept>

#include "IoStoreReader.h"
#include "Objects/FIoChunkId.h"
#include "../Readers/FByteArchive.h"
#include "../Versions/EGame.h"

namespace CUE4Parse::UE4::IO
{
    using namespace CUE4Parse::UE4::Versions;
    using CUE4Parse::UE4::Objects::UObject::FNameEntrySerialized;
    using CUE4Parse::UE4::Readers::FByteArchive;
    using Objects::FIoChunkId;
    using Objects::EIoChunkType;
    using Objects::EIoChunkType5;

    IoGlobalData::IoGlobalData(IoStoreReader& globalReader)
    {
        // The meta archive holding the script-object table (UE5: ScriptObjects; UE4.26+: LoaderInitialLoadMeta).
        std::unique_ptr<FByteArchive> metaAr;
        if (globalReader.Game() >= GAME_UE5_0)
        {
            metaAr = std::make_unique<FByteArchive>(
                "ScriptObjects",
                globalReader.Read(FIoChunkId(0, 0, static_cast<uint8_t>(EIoChunkType5::ScriptObjects))),
                globalReader.Versions);
            GlobalNameMap = FNameEntrySerialized::LoadNameBatch(*metaAr);
        }
        else // UE4.26+
        {
            Objects::FIoOffsetAndLength nameHashesChunk;
            if (!globalReader.TryResolve(FIoChunkId(0, 0, static_cast<uint8_t>(EIoChunkType::LoaderGlobalNameHashes)), nameHashesChunk))
            {
                throw std::out_of_range("Couldn't find LoaderGlobalNameHashes chunk in IoStore " + globalReader.Name());
            }

            const int nameCount = static_cast<int>(nameHashesChunk.Length() / sizeof(uint64_t) - 1);

            FByteArchive nameAr("LoaderGlobalNames",
                globalReader.Read(FIoChunkId(0, 0, static_cast<uint8_t>(EIoChunkType::LoaderGlobalNames))),
                globalReader.Versions);
            GlobalNameMap = FNameEntrySerialized::LoadNameBatch(nameAr, nameCount);

            metaAr = std::make_unique<FByteArchive>(
                "LoaderInitialLoadMeta",
                globalReader.Read(FIoChunkId(0, 0, static_cast<uint8_t>(EIoChunkType::LoaderInitialLoadMeta))),
                globalReader.Versions);
        }

        const int numScriptObjects = metaAr->Read<int32_t>();
        const auto scriptObjectEntries = metaAr->ReadArray<Objects::FScriptObjectEntry>(numScriptObjects);
        for (const auto& scriptObjectEntry : scriptObjectEntries)
        {
            ScriptObjectEntriesMap[scriptObjectEntry.GlobalIndex] = scriptObjectEntry;
        }
    }
}
