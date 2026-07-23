#include "FPakInfo.h"

#include <algorithm>
#include <string>

#include "../../Exceptions/ParserException.h"
#include "../../Readers/FPointerArchive.h"

namespace CUE4Parse::UE4::Pak::Objects
{
    using namespace CUE4Parse::UE4::Versions;
    using CUE4Parse::Compression::CompressionMethod;
    using CUE4Parse::UE4::Objects::Core::Misc::FGuid;
    using CUE4Parse::UE4::Objects::Core::Misc::FSHAHash;

    FPakInfo::FPakInfo(Readers::FArchive& Ar, OffsetsToTry offsetToTry)
    {
        const int64_t startPosition = Ar.Position;

        uint32_t hottaVersion = 0u;
        if (Ar.Game() == GAME_TowerOfFantasy && offsetToTry == OffsetsToTry::SizeHotta)
        {
            hottaVersion = Ar.Read<uint32_t>();
            // Dirty way to keep backwards compatibility
            // This will work if the data at the end is compressed or encrypted which we don't know yet at this point
            if (hottaVersion > 255) hottaVersion = 0;
        }

        if (Ar.Game() == GAME_TorchlightInfinite || Ar.Game() == GAME_EtheriaRestart) Ar.Position += 3;

        if (Ar.Game() == GAME_GameForPeace)
        {
            EncryptionKeyGuid = FGuid();
            EncryptedIndex = Ar.Read<uint8_t>() != 0x6c;
            Magic = Ar.Read<uint32_t>();
            if (Magic != PAK_FILE_MAGIC_GameForPeace) return;
            Version = Ar.Read<EPakFileVersion>();
            if (Version >= EPakFileVersion::PakFile_Version_PathHashIndex)
            {
                Version = EPakFileVersion::PakFile_Version_FNameBasedCompressionMethod; // Override to force readIndexLegacy
            }
            IndexHash = FSHAHash(Ar);
            IndexSize = static_cast<int64_t>(Ar.Read<uint64_t>() ^ 0x8924b0e3298b7069ULL);
            IndexOffset = static_cast<int64_t>(Ar.Read<uint64_t>() ^ 0xd74af37faa6b020dULL);
            CompressionMethods = {
                CompressionMethod::None, CompressionMethod::Zlib, CompressionMethod::Gzip,
                CompressionMethod::Oodle, CompressionMethod::LZ4, CompressionMethod::Zstd
            };
            return;
        }

        if (Ar.Game() == GAME_ArenaBreakoutMobile)
        {
            Magic = Ar.Read<uint32_t>();
            // Global or maybe older versions
            if (Magic == PAK_FILE_MAGIC_ArenaBreakoutInfinite)
            {
                EncryptionKeyGuid = FGuid();
                EncryptedIndex = Ar.Read<uint8_t>() != 0;
                IndexSize = Ar.Read<int64_t>();
                IndexOffset = Ar.Read<int64_t>();
                IndexHash = FSHAHash(Ar);
                Version = Ar.Read<EPakFileVersion>();
                goto beforeCompression;
            }

            // Chinese mobile version — needs GameTypes/ABI's DecryptAbiMobilePakInfo, which is not ported.
            if (Magic == PAK_FILE_MAGIC_ArenaBreakoutMobile)
                throw Exceptions::ParserException(Ar, "ArenaBreakoutMobile (CN) paks need the unported ABI decryption");
        }

        if (Ar.Game() == GAME_ArenaBreakoutInfinite)
        {
            EncryptionKeyGuid = Ar.Read<FGuid>();
            Magic = Ar.Read<uint32_t>();
            if (Magic != PAK_FILE_MAGIC_ArenaBreakoutInfinite) return;
            EncryptedIndex = Ar.Read<uint8_t>() != 0;
            IndexSize = Ar.Read<int64_t>();
            IndexOffset = Ar.Read<int64_t>();
            IndexHash = FSHAHash(Ar);
            Version = Ar.Read<EPakFileVersion>();
            goto beforeCompression;
        }

        if (Ar.Game() == GAME_DragonQuestXI)
        {
            EncryptionKeyGuid = FGuid();
            EncryptedIndex = Ar.Read<uint8_t>() != 0;
            Magic = Ar.Read<uint32_t>();
            if (Magic != PAK_FILE_MAGIC) return;
            Version = Ar.Read<EPakFileVersion>();
            IndexOffset = Ar.Read<int64_t>();
            IndexSize = Ar.Read<int64_t>();
            IndexHash = FSHAHash(Ar);
            goto beforeCompression;
        }

        if (Ar.Game() == GAME_RacingMaster)
        {
            EncryptedIndex = Ar.ReadFlag();
            EncryptionKeyGuid = Ar.Read<FGuid>();
            CustomEncryptionData = Ar.ReadBytes(4);
            Magic = Ar.Read<uint32_t>();
            if (Magic != PAK_FILE_MAGIC_RacingMaster) return;
            IndexSize = Ar.Read<int64_t>();
            IndexHash = FSHAHash(Ar);
            Version = Ar.Read<EPakFileVersion>();
            IndexOffset = Ar.Read<int64_t>();
            goto beforeCompression;
        }

        if (Ar.Game() == GAME_PromiseMascotAgency)
        {
            EncryptionKeyGuid = Ar.Read<FGuid>();
            EncryptedIndex = Ar.ReadFlag();
            Magic = Ar.Read<uint32_t>();
            if (Magic != PAK_FILE_MAGIC_PromiseMascotAgency) return;
            IndexHash = FSHAHash(Ar);
            Version = static_cast<EPakFileVersion>(11 + (Ar.Read<int32_t>() ^ 0x0A4FFC11));
            Ar.Position += 8;
            IndexSize = Ar.Read<int64_t>() ^ static_cast<int64_t>(0x0BBEFB6F91D3B57BULL);
            IndexOffset = Ar.Read<int64_t>();
            goto beforeCompression;
        }

        if (Ar.Game() == GAME_CrystalOfAtlan)
        {
            EncryptedIndex = Ar.ReadFlag();
            Version = Ar.Read<EPakFileVersion>();
            IndexSize = Ar.Read<int64_t>();
            IndexHash = FSHAHash(Ar);
            IndexOffset = Ar.Read<int64_t>();
            Magic = Ar.Read<uint32_t>();
            if (Magic != PAK_FILE_MAGIC_CrystalOfAtlan) return;
            EncryptionKeyGuid = Ar.Read<FGuid>();
            goto beforeCompression;
        }

        if (Ar.Game() == GAME_DuneAwakening)
        {
            if (Ar.Read<uint32_t>() != 0xA590ED1E) return;
            IndexOffset = Ar.Read<int64_t>();
            IndexSize = Ar.Read<int64_t>();
            IndexHash = FSHAHash(Ar);
            EncryptionKeyGuid = Ar.Read<FGuid>();
            EncryptedIndex = Ar.ReadFlag();
            Magic = Ar.Read<uint32_t>();
            if (Magic != PAK_FILE_MAGIC) return;
            Version = Ar.Read<EPakFileVersion>();
            Ar.Position += 36; // another index size/offset/hash
            goto beforeCompression;
        }

        if (Ar.Game() == GAME_Back4Blood) // Reversed by Spiritovod
        {
            Version = Ar.Read<EPakFileVersion>();
            Magic = Ar.Read<uint32_t>();
            if (Magic != PAK_FILE_MAGIC_Back4Blood) return;
            EncryptedIndex = Ar.Read<uint8_t>() != 0;
            EncryptionKeyGuid = Ar.Read<FGuid>();
            IndexOffset = Ar.Read<int64_t>();
            IndexSize = Ar.Read<int64_t>();
            IndexHash = FSHAHash(Ar);

            if (IndexSize > Ar.Length || IndexSize < 0)
            {
                Ar.Position = startPosition + 4;
                Magic = Ar.Read<uint32_t>();
                if (Magic != PAK_FILE_MAGIC_Back4Blood) return;
                EncryptionKeyGuid = FGuid();
                Ar.Position += 16;
                EncryptedIndex = Ar.Read<uint8_t>() != 0;
                IndexHash = FSHAHash(Ar);
                IndexSize = Ar.Read<int64_t>();
                IndexOffset = Ar.Read<int64_t>();
            }

            if (Ar.Position < Ar.Length)
            {
                if (Ar.Read<uint8_t>() > 1) Ar.Position--;
            }

            goto beforeCompression;
        }

        // ValorantSource needs GameTypes/Tencent's ValorantSourceAes + ValorantSourceRSA, neither ported.
        if (Ar.Game() == GAME_ValorantSource)
            throw Exceptions::ParserException(Ar, "ValorantSource paks need the unported Tencent encryption");

        // New FPakInfo fields.
        EncryptionKeyGuid = Ar.Read<FGuid>();          // PakFile_Version_EncryptionKeyGuid
        EncryptedIndex = Ar.Read<uint8_t>() != 0;      // Do not replace by ReadFlag

        // Old FPakInfo fields
        Magic = Ar.Read<uint32_t>();
        if (Magic != PAK_FILE_MAGIC)
        {
            if ((Ar.Game() == GAME_OutlastTrials && Magic == PAK_FILE_MAGIC_OutlastTrials) ||
                ((Ar.Game() == GAME_TorchlightInfinite || Ar.Game() == GAME_EtheriaRestart) &&
                 Magic == PAK_FILE_MAGIC_TorchlightInfinite) ||
                (Ar.Game() == GAME_WildAssault && Magic == PAK_FILE_MAGIC_WildAssault) ||
                (Ar.Game() == GAME_Undawn && Magic == PAK_FILE_MAGIC_Gameloop_Undawn) ||
                (Ar.Game() == GAME_FridayThe13th && Magic == PAK_FILE_MAGIC_FridayThe13th) ||
                (Ar.Game() == GAME_DreamStar && Magic == PAK_FILE_MAGIC_DreamStar) ||
                (Ar.Game() == GAME_AssaultFireFuture && Magic == PAK_FILE_MAGIC_AssaultFireFuture) ||
                (Ar.Game() == GAME_KartRiderDrift && Magic == PAK_FILE_MAGIC_KartRiderDrift) ||
                (Ar.Game() == GAME_SilverPalace && Magic == PAK_FILE_MAGIC_SilverPalace))
                goto afterMagic;
            // Stop immediately when magic is wrong
            return;
        }

        afterMagic:
        Version = hottaVersion >= 2 ? static_cast<EPakFileVersion>(Ar.Read<int32_t>() ^ 2) : Ar.Read<EPakFileVersion>();
        if (Ar.Game() == GAME_LordOfMysteries && (static_cast<uint32_t>(Version) & 0x80000000u) != 0)
        {
            Version = static_cast<EPakFileVersion>(static_cast<uint32_t>(Version) & 0x7FFFFFFFu);
            IndexHash = FSHAHash(Ar);
            IndexOffset = Ar.Read<int64_t>();
            IndexSize = Ar.Read<int64_t>() >> 1;
            goto beforeCompression;
        }

        if (Ar.Game() == GAME_StateOfDecay2)
            Version = static_cast<EPakFileVersion>(static_cast<int32_t>(Version) & 0xFFFF);

        if (Ar.Game() == GAME_KartRiderDrift)
            Version = static_cast<EPakFileVersion>(static_cast<int32_t>(Version) & 0x0F);

        if (Ar.Game() == GAME_FridayThe13th)
        {
            if (!EncryptedIndex && Magic == 0 && static_cast<uint32_t>(Version) == PAK_FILE_MAGIC)
            {
                Magic = PAK_FILE_MAGIC;
                Version = Ar.Read<EPakFileVersion>();
            }

            if (Version >= EPakFileVersion::PakFile_Version_RelativeChunkOffsets) // PakFile_Version_IllFonic
            {
                Version = EPakFileVersion::PakFile_Version_IndexEncryption; // Actual version
                Ar.Position += 4; // ExtraMagic
            }
        }

        IsSubVersion = Version == EPakFileVersion::PakFile_Version_FNameBasedCompressionMethod && offsetToTry == OffsetsToTry::Size8a;
        if (Ar.Game() == GAME_TorchlightInfinite || Ar.Game() == GAME_EtheriaRestart) Ar.Position += 1;
        if (Ar.Game() == GAME_BlackMythWukong) Ar.Position += 2;
        IndexOffset = Ar.Read<int64_t>();
        if (Ar.Game() == GAME_Farlight84) Ar.Position += 8; // unknown long
        if (Ar.Game() == GAME_Snowbreak) IndexOffset ^= 0x1C1D1E1F;
        if (Ar.Game() == GAME_KartRiderDrift) IndexOffset ^= 0x3009EB;
        if (Ar.Game() == GAME_NevernessToEverness || Ar.Game() == GAME_NevernessToEverness_CBT2) IndexOffset -= 1;
        IndexSize = Ar.Read<int64_t>();
        IndexHash = FSHAHash(Ar);

        if (Ar.Game() == GAME_DreamStar || Ar.Game() == GAME_AssaultFireFuture)
            std::swap(IndexOffset, IndexSize);

        if (Ar.Game() == GAME_MeetYourMaker && offsetToTry == OffsetsToTry::SizeHotta &&
            Version >= EPakFileVersion::PakFile_Version_Fnv64BugFix)
        {
            (void) Ar.Read<uint32_t>(); // I assume this is a version, only 0 right now.
        }

        if (Ar.Game() == GAME_WildAssault)
        {
            EncryptionKeyGuid = FGuid();
            IndexOffset = static_cast<int64_t>(static_cast<uint64_t>(IndexOffset) ^ 0xD5B9B05CE8143A3CULL) - 0xAA;
            IndexSize = static_cast<int64_t>(static_cast<uint64_t>(IndexSize) ^ 0x6DB425B4BC084B4BULL) - 0xA8;
        }

        if (Ar.Game() == GAME_SilverPalace)
        {
            IndexOffset = static_cast<int64_t>(static_cast<uint64_t>(IndexOffset) ^ 0x8b3c9f2a5e1d7046ULL);
            IndexSize = static_cast<int64_t>(static_cast<uint64_t>(IndexSize) ^ 0x8b3c9f2a5e1d7046ULL);
        }

        if (Ar.Game() == GAME_DeadByDaylight || Ar.Game() == GAME_DeadByDaylight_Old)
        {
            CustomEncryptionData = Ar.ReadBytes(28);
            (void) Ar.Read<uint32_t>();
        }

        if (Ar.Game() == GAME_OnePieceAmbition)
        {
            const int64_t currentPosition = Ar.Position;
            Ar.Position = IndexOffset;
            int64_t shift = Ar.Read<int64_t>();
            IndexOffset = Ar.Read<int64_t>();
            shift = ~shift;
            IndexOffset ^= shift;
            IndexSize = startPosition - IndexOffset - 17;
            Ar.Position = currentPosition;
        }

        if (Version == EPakFileVersion::PakFile_Version_FrozenIndex)
            IndexIsFrozen = Ar.Read<uint8_t>() != 0;

        beforeCompression:
        if (Version < EPakFileVersion::PakFile_Version_FNameBasedCompressionMethod)
        {
            CompressionMethods = {
                CompressionMethod::None, CompressionMethod::Zlib, CompressionMethod::Gzip,
                CompressionMethod::Oodle, CompressionMethod::LZ4, CompressionMethod::Zstd
            };
        }
        else
        {
            int maxNumCompressionMethods;
            switch (offsetToTry)
            {
                case OffsetsToTry::Size8a:
                case OffsetsToTry::SizeHotta:
                case OffsetsToTry::SizeDbD:
                case OffsetsToTry::SizeRennsport:
                case OffsetsToTry::SizeBack4Blood:
                case OffsetsToTry::SizeArenaBreakoutMobile:
                case OffsetsToTry::SizeValorantSource: maxNumCompressionMethods = 5; break;
                case OffsetsToTry::Size8: maxNumCompressionMethods = 4; break;
                case OffsetsToTry::Size8_1: maxNumCompressionMethods = 1; break;
                case OffsetsToTry::Size8_2: maxNumCompressionMethods = 2; break;
                case OffsetsToTry::Size8_3: maxNumCompressionMethods = 3; break;
                default: maxNumCompressionMethods = 4; break;
            }

            const int length = Ar.Game() == GAME_KartRiderDrift ? 48 : COMPRESSION_METHOD_NAME_LEN;
            const int bufferSize = length * maxNumCompressionMethods;
            std::vector<uint8_t> buffer(static_cast<size_t>(bufferSize));
            Ar.Serialize(buffer.data(), bufferSize);

            CompressionMethods.clear();
            CompressionMethods.reserve(static_cast<size_t>(maxNumCompressionMethods) + 1);
            CompressionMethods.push_back(CompressionMethod::None);
            for (int i = 0; i < maxNumCompressionMethods; i++)
            {
                // C# builds the string from all `length` bytes and then trims the trailing NULs, so an
                // interior NUL does not terminate it. Same here.
                std::string name(reinterpret_cast<const char*>(buffer.data()) + static_cast<size_t>(i) * length,
                                 static_cast<size_t>(length));
                while (!name.empty() && name.back() == '\0') name.pop_back();
                if (name.empty()) continue;

                CompressionMethod method;
                if (!TryParseCompressionMethod(name, method))
                {
                    // C# logs "Unknown compression method '{name}' in {archive}" here.
                    method = CompressionMethod::Unknown;
                }
                CompressionMethods.push_back(method);
            }
            if (hottaVersion >= 3)
            {
                // List<T>.Remove(0) drops the *first* None, not every one.
                const auto it = std::find(CompressionMethods.begin(), CompressionMethods.end(), CompressionMethod::None);
                if (it != CompressionMethods.end()) CompressionMethods.erase(it);
            }
        }

        // Written at the tail so the trailer for older versions remains byte-compatible. Paks authored before
        // this version leave PakchunkIndex at INDEX_NONE, and the reader falls back to deriving it from the filename.
        if (Version >= EPakFileVersion::PakFile_Version_PakchunkIndex && Ar.Game() >= GAME_UE5_9)
            PakchunkIndex = Ar.Read<int32_t>();

        // Reset new fields to their default states when seralizing older pak format.
        if (Version < EPakFileVersion::PakFile_Version_IndexEncryption) EncryptedIndex = false;
        if (Version < EPakFileVersion::PakFile_Version_EncryptionKeyGuid) EncryptionKeyGuid = FGuid();
    }

    FPakInfo FPakInfo::ReadFPakInfo(Readers::FArchive& Ar)
    {
        const int64_t length = Ar.Length;
        int64_t maxOffset;
        switch (Ar.Game())
        {
            case GAME_Back4Blood: maxOffset = static_cast<int64_t>(OffsetsToTry::SizeBack4Blood); break;
            case GAME_DuneAwakening: maxOffset = static_cast<int64_t>(OffsetsToTry::SizeDuneAwakening); break;
            case GAME_KartRiderDrift: maxOffset = static_cast<int64_t>(OffsetsToTry::SizeKartRiderDrift); break;
            case GAME_ArenaBreakoutMobile: maxOffset = static_cast<int64_t>(OffsetsToTry::SizeArenaBreakoutMobile); break;
            case GAME_ValorantSource: maxOffset = static_cast<int64_t>(OffsetsToTry::SizeValorantSource); break;
            default: maxOffset = std::min(length, static_cast<int64_t>(OffsetsToTry::SizeMax)); break;
        }

        Ar.Seek(-maxOffset, Readers::ESeekOrigin::End);
        std::vector<uint8_t> buffer(static_cast<size_t>(maxOffset));
        Ar.Serialize(buffer.data(), static_cast<int>(maxOffset));

        // GAME_InZOI decrypts the footer in place here via DecryptInZOIFPakInfo (GameTypes, not ported).
        if (Ar.Game() == GAME_InZOI)
            throw Exceptions::ParserException(Ar, "InZOI paks need the unported footer decryption");

        Readers::FPointerArchive reader(Ar.Name(), buffer.data(), maxOffset, Ar.Versions);

        static const OffsetsToTry defaultOffsets[] = {
            OffsetsToTry::Size8a,
            OffsetsToTry::Size8,
            OffsetsToTry::Size,
            OffsetsToTry::Size9,
            OffsetsToTry::Size9a,

            OffsetsToTry::Size8_1,
            OffsetsToTry::Size8_2,
            OffsetsToTry::Size8_3
        };

        std::vector<OffsetsToTry> offsetsToTry;
        switch (Ar.Game())
        {
            case GAME_TowerOfFantasy:
            case GAME_MeetYourMaker:
            case GAME_TorchlightInfinite:
            case GAME_EtheriaRestart: offsetsToTry = {OffsetsToTry::SizeHotta}; break;
            case GAME_FridayThe13th: offsetsToTry = {OffsetsToTry::SizeFTT}; break;
            case GAME_DeadByDaylight:
            case GAME_DeadByDaylight_Old: offsetsToTry = {OffsetsToTry::SizeDbD}; break;
            case GAME_Farlight84: offsetsToTry = {OffsetsToTry::SizeFarlight}; break;
            case GAME_QQ:
            case GAME_DreamStar: offsetsToTry = {OffsetsToTry::SizeDreamStar, OffsetsToTry::SizeQQ}; break;
            case GAME_GameForPeace:
            case GAME_DragonQuestXI: offsetsToTry = {OffsetsToTry::SizeGameForPeace}; break;
            case GAME_BlackMythWukong: offsetsToTry = {OffsetsToTry::SizeB1}; break;
            case GAME_Rennsport: offsetsToTry = {OffsetsToTry::SizeRennsport}; break;
            case GAME_RacingMaster: offsetsToTry = {OffsetsToTry::SizeRacingMaster}; break;
            case GAME_ARKSurvivalAscended:
            case GAME_PromiseMascotAgency: offsetsToTry = {OffsetsToTry::SizeARKSurvivalAscended}; break;
            case GAME_KartRiderDrift:
                offsetsToTry.assign(std::begin(defaultOffsets), std::end(defaultOffsets));
                offsetsToTry.push_back(OffsetsToTry::SizeKartRiderDrift);
                break;
            case GAME_DuneAwakening: offsetsToTry = {OffsetsToTry::SizeDuneAwakening}; break;
            case GAME_Back4Blood: offsetsToTry = {OffsetsToTry::SizeBack4Blood}; break;
            case GAME_ArenaBreakoutMobile: offsetsToTry = {OffsetsToTry::SizeArenaBreakoutMobile, OffsetsToTry::Size8a}; break;
            case GAME_ValorantSource: offsetsToTry = {OffsetsToTry::SizeValorantSource}; break;
            default: offsetsToTry.assign(std::begin(defaultOffsets), std::end(defaultOffsets)); break;
        }

        for (const OffsetsToTry offset : offsetsToTry)
        {
            if (static_cast<int64_t>(offset) > maxOffset) continue;

            reader.Seek(-static_cast<int64_t>(offset), Readers::ESeekOrigin::End);
            FPakInfo info;
            if (Ar.Game() == GAME_OnePieceAmbition)
            {
                const int64_t currentOffset = Ar.Position;
                Ar.Position -= static_cast<int64_t>(offset);
                info = FPakInfo(Ar, offset);
                Ar.Position = currentOffset;
            }
            else
            {
                info = FPakInfo(reader, offset);
            }

            bool found;
            switch (Ar.Game())
            {
                case GAME_FridayThe13th: found = info.Magic == PAK_FILE_MAGIC_FridayThe13th; break;
                case GAME_GameForPeace: found = info.Magic == PAK_FILE_MAGIC_GameForPeace; break;
                case GAME_Undawn: found = info.Magic == PAK_FILE_MAGIC_Gameloop_Undawn; break;
                case GAME_TorchlightInfinite:
                case GAME_EtheriaRestart: found = info.Magic == PAK_FILE_MAGIC_TorchlightInfinite; break;
                case GAME_DreamStar: found = info.Magic == PAK_FILE_MAGIC_DreamStar; break;
                case GAME_RacingMaster: found = info.Magic == PAK_FILE_MAGIC_RacingMaster; break;
                case GAME_OutlastTrials: found = info.Magic == PAK_FILE_MAGIC_OutlastTrials; break;
                case GAME_KartRiderDrift: found = info.Magic == PAK_FILE_MAGIC_KartRiderDrift; break;
                case GAME_CrystalOfAtlan: found = info.Magic == PAK_FILE_MAGIC_CrystalOfAtlan; break;
                case GAME_PromiseMascotAgency: found = info.Magic == PAK_FILE_MAGIC_PromiseMascotAgency; break;
                case GAME_WildAssault: found = info.Magic == PAK_FILE_MAGIC_WildAssault; break;
                case GAME_ArenaBreakoutInfinite: found = info.Magic == PAK_FILE_MAGIC_ArenaBreakoutInfinite; break;
                case GAME_ArenaBreakoutMobile:
                    found = info.Magic == PAK_FILE_MAGIC_ArenaBreakoutInfinite || info.Magic == PAK_FILE_MAGIC_ArenaBreakoutMobile;
                    break;
                case GAME_AssaultFireFuture: found = info.Magic == PAK_FILE_MAGIC_AssaultFireFuture; break;
                case GAME_Back4Blood: found = info.Magic == PAK_FILE_MAGIC_Back4Blood; break;
                case GAME_SilverPalace: found = info.Magic == PAK_FILE_MAGIC_SilverPalace; break;
                case GAME_ValorantSource: found = info.Magic == PAK_FILE_MAGIC_ValorantSource; break;
                default: found = info.Magic == PAK_FILE_MAGIC; break;
            }

            // C# derives the ValorantSource pak key from CustomEncryptionData here (not ported; that branch
            // throws long before this point).
            if (found) return info;
        }

        throw Exceptions::ParserException("File " + Ar.Name() + " has an unknown format");
    }
}
