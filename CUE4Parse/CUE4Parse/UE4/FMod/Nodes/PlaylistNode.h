// Ported from CUE4Parse/UE4/FMod/Nodes/PlaylistNode.cs
#pragma once

#include <vector>

#include "../Objects/FPlaylistEntry.h"
#include "../Enums/EPlaylistPlayMode.h"
#include "../Enums/EPlaylistSelectionMode.h"
#include "../FModReader.h"

namespace CUE4Parse::UE4::FMod::Nodes
{
    class PlaylistNode
    {
    public:
        Enums::EPlaylistPlayMode PlayMode{};
        Enums::EPlaylistSelectionMode SelectionMode{};
        std::vector<Objects::FPlaylistEntry> Entries;

        explicit PlaylistNode(Readers::FArchive& Ar)
        {
            PlayMode = static_cast<Enums::EPlaylistPlayMode>(Ar.Read<int32_t>());
            SelectionMode = static_cast<Enums::EPlaylistSelectionMode>(Ar.Read<int32_t>());
            Entries = FModReader::ReadElemListImp<Objects::FPlaylistEntry>(Ar);
            if (FModReader::Version() >= 0x65 && FModReader::Version() <= 0x67) (void) Ar.Read<uint8_t>();
        }
    };
}
