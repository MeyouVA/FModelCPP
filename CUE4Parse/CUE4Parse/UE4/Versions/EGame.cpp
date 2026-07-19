// Ported from GameUtils.GetVersion in CUE4Parse/UE4/Versions/EGame.cs
// The C# switch expressions match top-to-bottom (first match wins), so the ordering of the
// if/else chains below is significant and mirrors the source exactly.
#include "EGame.h"

namespace CUE4Parse::UE4::Versions
{
    FPackageFileVersion GetVersion(EGame game)
    {
        // Custom UE Games. If a game needs an even more specific custom version than the major
        // release version you can add it below.
        if (game >= GAME_UE5_0)
        {
            if (game == GAME_UE5_EA)             return FPackageFileVersion(522, 1002);
            if (game <  GAME_UE5_1)              return FPackageFileVersion(522, 1004);
            if (game <  GAME_UE5_2)              return FPackageFileVersion(522, 1008);
            if (game == GAME_TheFirstDescendant) return FPackageFileVersion(522, 1002);
            if (game <  GAME_UE5_4)              return FPackageFileVersion(522, 1009);
            if (game <  GAME_UE5_5)              return FPackageFileVersion(522, 1012);
            if (game <  GAME_UE5_6)              return FPackageFileVersion(522, 1013);
            if (game <  GAME_UE5_7)              return FPackageFileVersion(522, 1017);
            return FPackageFileVersion(static_cast<int32_t>(EUnrealEngineObjectUE4Version::AUTOMATIC_VERSION),
                                       static_cast<int32_t>(EUnrealEngineObjectUE5Version::AUTOMATIC_VERSION));
        }

        if (game >= GAME_UE4_0)
        {
            int32_t v;
            if      (game < GAME_UE4_1)  v = 342;
            else if (game < GAME_UE4_2)  v = 352;
            else if (game < GAME_UE4_3)  v = 363;
            else if (game < GAME_UE4_4)  v = 382;
            else if (game < GAME_UE4_5)  v = 385;
            else if (game < GAME_UE4_6)  v = 401;
            else if (game < GAME_UE4_7)  v = 413;
            else if (game < GAME_UE4_8)  v = 434;
            else if (game < GAME_UE4_9)  v = 451;
            else if (game < GAME_UE4_10) v = 482;
            else if (game < GAME_UE4_11) v = 482;
            else if (game < GAME_UE4_12) v = 498;
            else if (game < GAME_UE4_13) v = 504;
            else if (game < GAME_UE4_14) v = 505;
            else if (game < GAME_UE4_15) v = 508;
            else if (game < GAME_UE4_16) v = 510;
            else if (game < GAME_UE4_17) v = 513;
            else if (game < GAME_UE4_18) v = 513;
            else if (game < GAME_UE4_19) v = 514;
            else if (game < GAME_UE4_20) v = 516;
            else if (game < GAME_UE4_21) v = 516;
            else if (game < GAME_UE4_22) v = 517;
            else if (game < GAME_UE4_23) v = 517;
            else if (game < GAME_UE4_24) v = 517;
            else if (game < GAME_UE4_25) v = 518;
            else if (game < GAME_UE4_26) v = 518;
            else if (game < GAME_UE4_27) v = 522;
            else                         v = static_cast<int32_t>(EUnrealEngineObjectUE4Version::AUTOMATIC_VERSION);
            return FPackageFileVersion::CreateUE4Version(v);
        }

        return FPackageFileVersion::CreateUE3Version(
            static_cast<int32_t>(EUnrealEngineObjectUE3Version::AUTOMATIC_VERSION));
    }
}
