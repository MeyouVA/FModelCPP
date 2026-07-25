// Ported from CUE4Parse/UE4/FMod/Metadata/SoundTable.cs
// The STBL chunk: a binary-searchable table of (Jenkins hash key -> subsound index) for localized audio.
#pragma once

#include <vector>

#include "../Objects/FUInt24.h"
#include "../FModReader.h"

namespace CUE4Parse::UE4::FMod::Metadata
{
    class SoundTable
    {
    public:
        uint32_t HeaderFlag = 0;
        int32_t SoundbankIndex = 0;
        std::vector<uint64_t> Keys;
        std::vector<Objects::FUInt24> Indices;
        uint32_t Flags = 0;

        explicit SoundTable(Readers::FArchive& Ar)
        {
            HeaderFlag = Ar.Read<uint32_t>();
            SoundbankIndex = Ar.Read<int32_t>();
            Keys = ReadSimpleArrayImp(Ar);
            Indices = FModReader::ReadSimpleArray24(Ar);
            if (FModReader::Version() >= 0x7c)
                Flags = Ar.Read<uint32_t>();
        }

        // FMOD::SoundTable::Packed::find -- binary search over the sorted key array.
        int Find(uint64_t key) const
        {
            int mSize = static_cast<int>(Keys.size());
            int low = 0;
            int high = mSize - 1;

            if (high < 0)
                return -1;

            while (low <= high)
            {
                int mid = (high + low) >> 1;
                uint64_t currentKey = Keys[mid];

                if (key == currentKey)
                    return static_cast<int>(Indices[mid].Value);

                if (key < currentKey)
                    high = mid - 1;
                else
                    low = mid + 1;
            }

            return -1;
        }

    private:
        static std::vector<uint64_t> ReadSimpleArrayImp(Readers::FArchive& Ar)
        {
            uint32_t count = FModReader::ReadX16(Ar);
            std::vector<uint64_t> list(count);
            for (uint32_t i = 0; i < count; i++)
                list[i] = Ar.Read<uint64_t>();
            return list;
        }
    };
}
