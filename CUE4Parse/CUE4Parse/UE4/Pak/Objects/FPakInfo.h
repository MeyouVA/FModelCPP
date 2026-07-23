// Ported from CUE4Parse/UE4/Pak/Objects/FPakInfo.cs
// The pak trailer: magic, version, where the index lives, whether it is encrypted, and the table of
// compression method names. It is found by seeking back from the end of the file and trying a list of
// candidate trailer sizes until one of them yields the expected magic.
//
// Deliberate differences from C#:
//   * Three game branches are NOT ported and throw instead: ValorantSource and the Chinese
//     ArenaBreakoutMobile variant need CUE4Parse.GameTypes decryption (ValorantSourceRSA/Aes, ABIDecryption),
//     and InZOI needs the DecryptInZOIFPakInfo partial-class method. None of GameTypes is ported. Every
//     other game branch is ported as-is. TODO with the GameTypes layer.
//   * The Serilog warning for an unknown compression method name is dropped (no logging layer); the method
//     still falls back to CompressionMethod::Unknown exactly as in C#.
#pragma once

#include <cstdint>
#include <vector>

#include "../../../Compression/CompressionMethod.h"
#include "../../Objects/Core/Misc/FGuid.h"
#include "../../Objects/Core/Misc/FSHAHash.h"
#include "../../Readers/FArchive.h"

namespace CUE4Parse::UE4::Pak::Objects
{
    enum class EPakFileVersion : int32_t
    {
        PakFile_Version_Initial = 1,
        PakFile_Version_NoTimestamps = 2,
        PakFile_Version_CompressionEncryption = 3,
        PakFile_Version_IndexEncryption = 4,
        PakFile_Version_RelativeChunkOffsets = 5,
        PakFile_Version_DeleteRecords = 6,
        PakFile_Version_EncryptionKeyGuid = 7,
        PakFile_Version_FNameBasedCompressionMethod = 8,
        PakFile_Version_FrozenIndex = 9,
        PakFile_Version_PathHashIndex = 10,
        PakFile_Version_Fnv64BugFix = 11,
        PakFile_Version_Utf8PakDirectory = 12,
        PakFile_Version_SortedDirectoryIndex = 13, // FullDirectoryIndex stored as a flat FPakFlatDirectoryIndex.
        PakFile_Version_PakchunkIndex = 14, // PakchunkIndex stored in the trailer so it doesn't have to be derived from the filename.

        PakFile_Version_Last,
        PakFile_Version_Invalid,
        PakFile_Version_Latest = PakFile_Version_Last - 1
    };

    class FPakInfo
    {
    public:
        static constexpr uint32_t PAK_FILE_MAGIC = 0x5A6F12E1;
        static constexpr uint32_t PAK_FILE_MAGIC_OutlastTrials = 0xA590ED1E;
        static constexpr uint32_t PAK_FILE_MAGIC_TorchlightInfinite = 0x6B2A56B8;
        static constexpr uint32_t PAK_FILE_MAGIC_WildAssault = 0xA4CCD123;
        static constexpr uint32_t PAK_FILE_MAGIC_Gameloop_Undawn = 0x5A6F12EC;
        static constexpr uint32_t PAK_FILE_MAGIC_FridayThe13th = 0x65617441;
        static constexpr uint32_t PAK_FILE_MAGIC_DreamStar = 0x1B6A32F1;
        static constexpr uint32_t PAK_FILE_MAGIC_GameForPeace = 0xff67ff70;
        static constexpr uint32_t PAK_FILE_MAGIC_KartRiderDrift = 0x81c4b35b;
        static constexpr uint32_t PAK_FILE_MAGIC_RacingMaster = 0x9a51da3f;
        static constexpr uint32_t PAK_FILE_MAGIC_CrystalOfAtlan = 0x22ce976a;
        static constexpr uint32_t PAK_FILE_MAGIC_PromiseMascotAgency = 0x11adde11;
        static constexpr uint32_t PAK_FILE_MAGIC_ArenaBreakoutInfinite = 0x53647586;
        static constexpr uint32_t PAK_FILE_MAGIC_ArenaBreakoutMobile = 0x57647587;
        static constexpr uint32_t PAK_FILE_MAGIC_AssaultFireFuture = 0x4F6FAE86;
        static constexpr uint32_t PAK_FILE_MAGIC_Back4Blood = 0x18772;
        static constexpr uint32_t PAK_FILE_MAGIC_SilverPalace = 0x12E15A6F;
        static constexpr uint32_t PAK_FILE_MAGIC_ValorantSource = 0x167C2AB4;

        static constexpr int COMPRESSION_METHOD_NAME_LEN = 32;

        uint32_t Magic = 0;
        EPakFileVersion Version = static_cast<EPakFileVersion>(0);
        bool IsSubVersion = false;
        int64_t IndexOffset = 0;
        int64_t IndexSize = 0;
        UE4::Objects::Core::Misc::FSHAHash IndexHash;
        // When new fields are added to FPakInfo, they're serialized before 'Magic' to keep compatibility
        // with older pak file versions. At the same time, structure size grows.
        bool EncryptedIndex = false;
        bool IndexIsFrozen = false;
        UE4::Objects::Core::Misc::FGuid EncryptionKeyGuid;
        std::vector<Compression::CompressionMethod> CompressionMethods;
        int32_t PakchunkIndex = -1; // INDEX_NONE
        std::vector<uint8_t> CustomEncryptionData;

        static FPakInfo ReadFPakInfo(Readers::FArchive& Ar);

    private:
        // C#'s private enum of candidate trailer sizes. Kept as a plain int64 enum because the values are
        // compared against Ar.Length and used as seek offsets.
        enum class OffsetsToTry : int64_t
        {
            Size = sizeof(int32_t) * 2 + sizeof(int64_t) * 2 + 20 + /* new fields */ 1 + 16, // sizeof(FGuid)
            // Just to be sure
            SizeGameForPeace = 45,
            Size8_1 = Size + 32,
            Size8_2 = Size8_1 + 32,
            Size8_3 = Size8_2 + 32,
            Size8 = Size8_3 + 32, // added size of CompressionMethods as char[32]
            Size8a = Size8 + 32, // UE4.23 - also has version 8 (like 4.22) but different pak file structure
            Size9 = Size8a + 1, // UE4.25
            Size9a = Size9 + 4, // UE6.0 - Added pakchunk index int32
            SizeB1 = Size9 + 1, // plus 1

            SizeRacingMaster = Size8 + 4, // additional int
            SizeFTT = Size + 4, // additional int for extra magic
            SizeHotta = Size8a + 4, // additional int for custom pak version
            SizeARKSurvivalAscended = Size8a + 8, // additional 8 bytes
            SizeFarlight = Size8a + 9, // additional long and byte
            SizeDreamStar = Size8a + 10,
            SizeRennsport = Size8a + 16,
            SizeQQ = Size8a + 26,
            SizeDbD = Size8a + 32, // additional 28 bytes for encryption key and 4 bytes for unknown uint

            SizeLast,
            SizeMax = SizeLast - 1,
            SizeBack4Blood = 222,
            SizeArenaBreakoutMobile = 205,
            SizeDuneAwakening = 261,
            SizeValorantSource = 286, // For older versions it was 282
            SizeKartRiderDrift = 397, // don't let this be SizeMax, it's way above average and cause issues
        };

        FPakInfo() = default;
        FPakInfo(Readers::FArchive& Ar, OffsetsToTry offsetToTry);
    };
}
