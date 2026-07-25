// Ported from CUE4Parse/UE4/Wwise/Objects/AkBankHeader.cs
#pragma once

#include <cstdint>
#include <vector>

#include "../WwiseArchive.h"
#include "../Enums/EAkBankTypeEnum.h"
#include "../Enums/Flags/EAltValuesFlags.h"

namespace CUE4Parse::UE4::Wwise::Objects
{
    using CUE4Parse::UE4::Wwise::Enums::EAkBankTypeEnum;
    using CUE4Parse::UE4::Wwise::Enums::Flags::EAltValuesFlags;

    struct AkBankHeader
    {
        uint32_t Version = 0;
        uint32_t SoundBankId = 0;
        uint32_t LanguageId = 0;
        bool FeedbackInBank = false;
        EAltValuesFlags AltValues = static_cast<EAltValuesFlags>(0);
        uint32_t ProjectId = 0;
        EAkBankTypeEnum SoundBankType = static_cast<EAkBankTypeEnum>(0);
        std::vector<uint8_t> BankHash;

        AkBankHeader() = default;

        // CAkBankMgr::ProcessBankHeader
        AkBankHeader(FWwiseArchive& Ar, int sectionLength)
        {
            Version = Ar.Read<uint32_t>(); // If version is less than 26 there's two params before this read
            SoundBankId = Ar.Read<uint32_t>();
            LanguageId = Ar.Read<uint32_t>();

            // C#'s switch on Version. The <= 134 and default arms are identical in the source; kept apart
            // so the version boundary stays visible.
            if (Version <= 26)
            {
                Ar.Read<uint64_t>(); // timestamp
            }
            else if (Version <= 126)
            {
                FeedbackInBank = (Ar.Read<uint32_t>() & 1) != 0;
            }
            else if (Version <= 134)
            {
                AltValues = Ar.Read<EAltValuesFlags>();
            }
            else
            {
                AltValues = Ar.Read<EAltValuesFlags>();
            }

            if (Version > 76)
            {
                ProjectId = Ar.Read<uint32_t>();
            }

            if (Version > 141)
            {
                SoundBankType = Ar.Read<EAkBankTypeEnum>();
                BankHash = Ar.ReadBytes(0x10);
            }

            // Determine padding size
            int gapSize;
            if (Version <= 26)       gapSize = sectionLength - 0x18;
            else if (Version <= 76)  gapSize = sectionLength - 0x10;
            else if (Version <= 141) gapSize = sectionLength - 0x14;
            else                     gapSize = sectionLength - 0x14 - 0x04 - 0x10;

            if (gapSize > 0)
                Ar.Position += gapSize;
        }
    };

    struct FAKPKHeader
    {
        bool Endianness = false;
        uint32_t NamesSectionLength = 0;
        uint32_t BanksSectionLength = 0;
        uint32_t SoundsSectionLength = 0;
        uint32_t ExternalSoundsSectionLength = 0;

        FAKPKHeader() = default;

        explicit FAKPKHeader(FWwiseArchive& Ar)
        {
            // FArchive::ReadBoolean reads four bytes, matching C# -- not FWwiseArchive::ReadBool.
            Endianness = Ar.ReadBoolean();
            NamesSectionLength = Ar.Read<uint32_t>();
            BanksSectionLength = Ar.Read<uint32_t>();
            SoundsSectionLength = Ar.Read<uint32_t>();
            ExternalSoundsSectionLength = Ar.Read<uint32_t>();
        }

        static constexpr int64_t NamesOffset = 28; // sectionHeader + sizeof(FAKPKHeader)
        int64_t BanksOffset() const { return NamesOffset + NamesSectionLength; }
        int64_t WemsOffset() const { return BanksOffset() + BanksSectionLength; }
        int64_t ExternalWemsOffset() const { return WemsOffset() + SoundsSectionLength; }
    };
}
