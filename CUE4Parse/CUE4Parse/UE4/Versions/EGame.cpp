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

    // Generated from the EGame declaration order in EGame.cs, with alias members (several names sharing one
    // value) collapsed to the first-declared name -- the same choice .NET's Enum.ToString() makes.
    const char* EGameName(EGame game)
    {
        switch (game)
        {
            case GAME_UE4_0: return "GAME_UE4_0";
            case GAME_UE4_1: return "GAME_UE4_1";
            case GAME_UE4_2: return "GAME_UE4_2";
            case GAME_UE4_3: return "GAME_UE4_3";
            case GAME_UE4_4: return "GAME_UE4_4";
            case GAME_UE4_5: return "GAME_UE4_5";
            case GAME_ArkSurvivalEvolved: return "GAME_ArkSurvivalEvolved";
            case GAME_UE4_6: return "GAME_UE4_6";
            case GAME_UE4_7: return "GAME_UE4_7";
            case GAME_UE4_8: return "GAME_UE4_8";
            case GAME_UE4_9: return "GAME_UE4_9";
            case GAME_UE4_10: return "GAME_UE4_10";
            case GAME_SeaOfThieves: return "GAME_SeaOfThieves";
            case GAME_UE4_11: return "GAME_UE4_11";
            case GAME_GearsOfWar4: return "GAME_GearsOfWar4";
            case GAME_DaysGone: return "GAME_DaysGone";
            case GAME_UE4_12: return "GAME_UE4_12";
            case GAME_Abzu: return "GAME_Abzu";
            case GAME_UE4_13: return "GAME_UE4_13";
            case GAME_StateOfDecay2: return "GAME_StateOfDecay2";
            case GAME_WeHappyFew: return "GAME_WeHappyFew";
            case Game_StyxShardsofDarkness: return "Game_StyxShardsofDarkness";
            case GAME_UE4_14: return "GAME_UE4_14";
            case GAME_TEKKEN7: return "GAME_TEKKEN7";
            case GAME_TransformersOnline: return "GAME_TransformersOnline";
            case GAME_UE4_15: return "GAME_UE4_15";
            case GAME_ConanExiles: return "GAME_ConanExiles";
            case GAME_UE4_16: return "GAME_UE4_16";
            case GAME_PlayerUnknownsBattlegrounds: return "GAME_PlayerUnknownsBattlegrounds";
            case GAME_TrainSimWorld2020: return "GAME_TrainSimWorld2020";
            case GAME_NarutotoBorutoShinobiStriker: return "GAME_NarutotoBorutoShinobiStriker";
            case GAME_UE4_17: return "GAME_UE4_17";
            case GAME_AWayOut: return "GAME_AWayOut";
            case GAME_UE4_18: return "GAME_UE4_18";
            case GAME_KingdomHearts3: return "GAME_KingdomHearts3";
            case GAME_FinalFantasy7Remake: return "GAME_FinalFantasy7Remake";
            case GAME_AceCombat7: return "GAME_AceCombat7";
            case GAME_FridayThe13th: return "GAME_FridayThe13th";
            case GAME_GameForPeace: return "GAME_GameForPeace";
            case GAME_DragonQuestXI: return "GAME_DragonQuestXI";
            case GAME_CodeVein: return "GAME_CodeVein";
            case GAME_UE4_19: return "GAME_UE4_19";
            case GAME_Paragon: return "GAME_Paragon";
            case GAME_Ashen: return "GAME_Ashen";
            case GAME_UE4_20: return "GAME_UE4_20";
            case GAME_Borderlands3: return "GAME_Borderlands3";
            case GAME_UE4_21: return "GAME_UE4_21";
            case GAME_StarWarsJediFallenOrder: return "GAME_StarWarsJediFallenOrder";
            case GAME_Undawn: return "GAME_Undawn";
            case GAME_UE4_22: return "GAME_UE4_22";
            case GAME_UE4_23: return "GAME_UE4_23";
            case GAME_ApexLegendsMobile: return "GAME_ApexLegendsMobile";
            case GAME_UE4_24: return "GAME_UE4_24";
            case GAME_TonyHawkProSkater12: return "GAME_TonyHawkProSkater12";
            case GAME_BigRumbleBoxingCreedChampions: return "GAME_BigRumbleBoxingCreedChampions";
            case GAME_AssaultFireFuture: return "GAME_AssaultFireFuture";
            case GAME_UE4_25: return "GAME_UE4_25";
            case GAME_UE4_25_Plus: return "GAME_UE4_25_Plus";
            case GAME_RogueCompany: return "GAME_RogueCompany";
            case GAME_DeadIsland2: return "GAME_DeadIsland2";
            case GAME_KenaBridgeofSpirits: return "GAME_KenaBridgeofSpirits";
            case GAME_Strinova: return "GAME_Strinova";
            case GAME_SYNCED: return "GAME_SYNCED";
            case GAME_OperationApocalypse: return "GAME_OperationApocalypse";
            case GAME_Farlight84: return "GAME_Farlight84";
            case GAME_StarWarsHunters: return "GAME_StarWarsHunters";
            case GAME_ThePathless: return "GAME_ThePathless";
            case GAME_SuicideSquad: return "GAME_SuicideSquad";
            case GAME_HellLetLoose: return "GAME_HellLetLoose";
            case GAME_AliensFireteamElite: return "GAME_AliensFireteamElite";
            case GAME_Back4Blood: return "GAME_Back4Blood";
            case GAME_NiNoKuniCrossWorlds: return "GAME_NiNoKuniCrossWorlds";
            case GAME_UE4_26: return "GAME_UE4_26";
            case GAME_GTATheTrilogyDefinitiveEdition: return "GAME_GTATheTrilogyDefinitiveEdition";
            case GAME_ReadyOrNot: return "GAME_ReadyOrNot";
            case GAME_BladeAndSoul: return "GAME_BladeAndSoul";
            case GAME_TowerOfFantasy: return "GAME_TowerOfFantasy";
            case GAME_FinalFantasy7Rebirth: return "GAME_FinalFantasy7Rebirth";
            case GAME_TheDivisionResurgence: return "GAME_TheDivisionResurgence";
            case GAME_StarWarsJediSurvivor: return "GAME_StarWarsJediSurvivor";
            case GAME_Snowbreak: return "GAME_Snowbreak";
            case GAME_TorchlightInfinite: return "GAME_TorchlightInfinite";
            case GAME_QQ: return "GAME_QQ";
            case GAME_WutheringWaves: return "GAME_WutheringWaves";
            case GAME_DreamStar: return "GAME_DreamStar";
            case GAME_MidnightSuns: return "GAME_MidnightSuns";
            case GAME_FragPunk: return "GAME_FragPunk";
            case GAME_RacingMaster: return "GAME_RacingMaster";
            case GAME_StellarBlade: return "GAME_StellarBlade";
            case GAME_EtheriaRestart: return "GAME_EtheriaRestart";
            case GAME_EvilWest: return "GAME_EvilWest";
            case GAME_ArenaBreakoutInfinite: return "GAME_ArenaBreakoutInfinite";
            case GAME_Psychonauts2: return "GAME_Psychonauts2";
            case GAME_OctopathTravelerCoTC: return "GAME_OctopathTravelerCoTC";
            case GAME_DarkPicturesAnthologyHouseOfAshes: return "GAME_DarkPicturesAnthologyHouseOfAshes";
            case GAME_DarkPicturesAnthologyManofMedan: return "GAME_DarkPicturesAnthologyManofMedan";
            case GAME_DarkPicturesAnthologyTheDevilinMe: return "GAME_DarkPicturesAnthologyTheDevilinMe";
            case GAME_DarkPicturesAnthologyLittleHope: return "GAME_DarkPicturesAnthologyLittleHope";
            case GAME_TheQuarry: return "GAME_TheQuarry";
            case GAME_RocoKingdomWorld: return "GAME_RocoKingdomWorld";
            case GAME_HonorofKingsWorld: return "GAME_HonorofKingsWorld";
            case GAME_eFootball: return "GAME_eFootball";
            case GAME_ArenaBreakoutMobile: return "GAME_ArenaBreakoutMobile";
            case GAME_ValorantSource: return "GAME_ValorantSource";
            case GAME_UE4_27: return "GAME_UE4_27";
            case GAME_Splitgate: return "GAME_Splitgate";
            case GAME_HYENAS: return "GAME_HYENAS";
            case GAME_HogwartsLegacy: return "GAME_HogwartsLegacy";
            case GAME_OutlastTrials: return "GAME_OutlastTrials";
            case GAME_Valorant_PRE_11_2: return "GAME_Valorant_PRE_11_2";
            case GAME_Gollum: return "GAME_Gollum";
            case GAME_Grounded: return "GAME_Grounded";
            case GAME_DeltaForce: return "GAME_DeltaForce";
            case GAME_MortalKombat1: return "GAME_MortalKombat1";
            case GAME_VisionsofMana: return "GAME_VisionsofMana";
            case GAME_Spectre: return "GAME_Spectre";
            case GAME_KartRiderDrift: return "GAME_KartRiderDrift";
            case GAME_ThroneAndLiberty: return "GAME_ThroneAndLiberty";
            case GAME_MotoGP24: return "GAME_MotoGP24";
            case GAME_Stray: return "GAME_Stray";
            case GAME_CrystalOfAtlan: return "GAME_CrystalOfAtlan";
            case GAME_PromiseMascotAgency: return "GAME_PromiseMascotAgency";
            case GAME_TerminullBrigade: return "GAME_TerminullBrigade";
            case GAME_AshEchoes: return "GAME_AshEchoes";
            case GAME_NeedForSpeedMobile: return "GAME_NeedForSpeedMobile";
            case GAME_TonyHawkProSkater34: return "GAME_TonyHawkProSkater34";
            case GAME_OnePieceAmbition: return "GAME_OnePieceAmbition";
            case GAME_UnchartedWatersOrigin: return "GAME_UnchartedWatersOrigin";
            case GAME_LostSoulAside: return "GAME_LostSoulAside";
            case GAME_GhostsofTabor: return "GAME_GhostsofTabor";
            case GAME_BlueProtocol: return "GAME_BlueProtocol";
            case GAME_LittleNightmares3: return "GAME_LittleNightmares3";
            case GAME_Raven2: return "GAME_Raven2";
            case GAME_DuetNightAbyss: return "GAME_DuetNightAbyss";
            case GAME_LiesofP: return "GAME_LiesofP";
            case GAME_BloodBowl3: return "GAME_BloodBowl3";
            case GAME_ChasingKaleidoRIDER: return "GAME_ChasingKaleidoRIDER";
            case GAME_Lego2KDrive: return "GAME_Lego2KDrive";
            case GAME_CenturyAgeofAshes: return "GAME_CenturyAgeofAshes";
            case GAME_EmbersofTheUncrowned: return "GAME_EmbersofTheUncrowned";
            case GAME_eBaseballProSpirit: return "GAME_eBaseballProSpirit";
            case GAME_UE4_28: return "GAME_UE4_28";
            // GAME_UE4_LATEST == GAME_UE4_28 (alias)
            case GAME_UE5_0: return "GAME_UE5_0";
            case GAME_MeetYourMaker: return "GAME_MeetYourMaker";
            case GAME_BlackMythWukong: return "GAME_BlackMythWukong";
            case GAME_UE5_EA: return "GAME_UE5_EA";
            case GAME_UE5_1: return "GAME_UE5_1";
            case GAME_3on3FreeStyleRebound: return "GAME_3on3FreeStyleRebound";
            case GAME_Stalker2: return "GAME_Stalker2";
            case GAME_TheCastingofFrankStone: return "GAME_TheCastingofFrankStone";
            case GAME_SilentHill2Remake: return "GAME_SilentHill2Remake";
            case GAME_Dauntless: return "GAME_Dauntless";
            case GAME_WorldofJadeDynasty: return "GAME_WorldofJadeDynasty";
            case GAME_LordsoftheFallen: return "GAME_LordsoftheFallen";
            case GAME_Palworld: return "GAME_Palworld";
            case GAME_UE5_2: return "GAME_UE5_2";
            case GAME_Placeholder5: return "GAME_Placeholder5";
            case GAME_PaxDei: return "GAME_PaxDei";
            case GAME_TheFirstDescendant: return "GAME_TheFirstDescendant";
            case GAME_MetroAwakening: return "GAME_MetroAwakening";
            case GAME_LostRecordsBloomAndRage: return "GAME_LostRecordsBloomAndRage";
            case GAME_DuneAwakening: return "GAME_DuneAwakening";
            case GAME_Placeholder4: return "GAME_Placeholder4";
            case GAME_PUBGBlackBudget: return "GAME_PUBGBlackBudget";
            case GAME_UE5_3: return "GAME_UE5_3";
            case GAME_MarvelRivals: return "GAME_MarvelRivals";
            case GAME_BlackStigma: return "GAME_BlackStigma";
            case GAME_Valorant: return "GAME_Valorant";
            case GAME_ArcRaiders: return "GAME_ArcRaiders";
            case GAME_Aion2: return "GAME_Aion2";
            case GAME_TheFinals: return "GAME_TheFinals";
            case GAME_Avowed: return "GAME_Avowed";
            case GAME_MetalGearSolidDelta: return "GAME_MetalGearSolidDelta";
            case GAME_Highguard: return "GAME_Highguard";
            case GAME_DragonSwordAwakening: return "GAME_DragonSwordAwakening";
            case GAME_UE5_4: return "GAME_UE5_4";
            case GAME_FunkoFusion: return "GAME_FunkoFusion";
            case GAME_InfinityNikki: return "GAME_InfinityNikki";
            case GAME_SilverPalace: return "GAME_SilverPalace";
            case GAME_Gothic1Remake: return "GAME_Gothic1Remake";
            case GAME_SplitFiction: return "GAME_SplitFiction";
            case GAME_WildAssault: return "GAME_WildAssault";
            case GAME_InZOI: return "GAME_InZOI";
            case GAME_TempestRising: return "GAME_TempestRising";
            case GAME_MindsEye: return "GAME_MindsEye";
            case GAME_DeadByDaylight_Old: return "GAME_DeadByDaylight_Old";
            case GAME_Placeholder1: return "GAME_Placeholder1";
            case GAME_MafiaTheOldCountry: return "GAME_MafiaTheOldCountry";
            case GAME_2XKO: return "GAME_2XKO";
            case GAME_Reanimal: return "GAME_Reanimal";
            case GAME_VEIN: return "GAME_VEIN";
            case GAME_Placeholder2: return "GAME_Placeholder2";
            case GAME_OuterWorlds2: return "GAME_OuterWorlds2";
            case GAME_OctopathTraveler0: return "GAME_OctopathTraveler0";
            case GAME_CodeVein2: return "GAME_CodeVein2";
            case GAME_UE5_5: return "GAME_UE5_5";
            case GAME_Brickadia: return "GAME_Brickadia";
            case GAME_Splitgate2: return "GAME_Splitgate2";
            case GAME_DeadzoneRogue: return "GAME_DeadzoneRogue";
            case GAME_Directive8020: return "GAME_Directive8020";
            case GAME_Wildgate: return "GAME_Wildgate";
            case GAME_ARKSurvivalAscended: return "GAME_ARKSurvivalAscended";
            case GAME_NevernessToEverness_CBT2: return "GAME_NevernessToEverness_CBT2";
            case GAME_FateTrigger: return "GAME_FateTrigger";
            case GAME_Placeholder6: return "GAME_Placeholder6";
            case GAME_Borderlands4: return "GAME_Borderlands4";
            case GAME_Rennsport: return "GAME_Rennsport";
            case GAME_GrayZoneWarfare: return "GAME_GrayZoneWarfare";
            case GAME_IntotheRadius2: return "GAME_IntotheRadius2";
            case GAME_HighOnLife2: return "GAME_HighOnLife2";
            case GAME_MongilStarDive: return "GAME_MongilStarDive";
            case GAME_UE5_6: return "GAME_UE5_6";
            case GAME_Grounded2: return "GAME_Grounded2";
            case GAME_AshesOfCreation: return "GAME_AshesOfCreation";
            case GAME_Solasta2: return "GAME_Solasta2";
            case GAME_NevernessToEverness: return "GAME_NevernessToEverness";
            case GAME_DeadByDaylight: return "GAME_DeadByDaylight";
            case GAME_ConanExilesEnhanced: return "GAME_ConanExilesEnhanced";
            case GAME_Subnautica2: return "GAME_Subnautica2";
            case GAME_LEGOBatmanLegacyoftheDarkKnight: return "GAME_LEGOBatmanLegacyoftheDarkKnight";
            case GAME_Fatekeeper: return "GAME_Fatekeeper";
            case GAME_Enginefall: return "GAME_Enginefall";
            case GAME_UE5_7: return "GAME_UE5_7";
            case GAME_TitanQuest2: return "GAME_TitanQuest2";
            case GAME_Squad: return "GAME_Squad";
            case GAME_Empulse: return "GAME_Empulse";
            case GAME_LordOfMysteries: return "GAME_LordOfMysteries";
            case GAME_UE5_8: return "GAME_UE5_8";
            case GAME_WutheringWavesFastGeo: return "GAME_WutheringWavesFastGeo";
            case GAME_UE5_9: return "GAME_UE5_9";
            // GAME_UE5_LATEST == GAME_UE5_9 (alias)
            case GAME_UE6_0: return "GAME_UE6_0";
            // GAME_UE6_LATEST == GAME_UE6_0 (alias)
        }
        return nullptr;
    }

    const EGame* EGameValues(size_t& count)
    {
        // Kept in the same order, and in lockstep, with the EGameName switch above: both are the enum's
        // member list, so a member added to one without the other is a bug the tests catch.
        static const EGame values[] = {
        GAME_UE4_0, GAME_UE4_1, GAME_UE4_2, GAME_UE4_3,
        GAME_UE4_4, GAME_UE4_5, GAME_ArkSurvivalEvolved, GAME_UE4_6,
        GAME_UE4_7, GAME_UE4_8, GAME_UE4_9, GAME_UE4_10,
        GAME_SeaOfThieves, GAME_UE4_11, GAME_GearsOfWar4, GAME_DaysGone,
        GAME_UE4_12, GAME_Abzu, GAME_UE4_13, GAME_StateOfDecay2,
        GAME_WeHappyFew, Game_StyxShardsofDarkness, GAME_UE4_14, GAME_TEKKEN7,
        GAME_TransformersOnline, GAME_UE4_15, GAME_ConanExiles, GAME_UE4_16,
        GAME_PlayerUnknownsBattlegrounds, GAME_TrainSimWorld2020, GAME_NarutotoBorutoShinobiStriker, GAME_UE4_17,
        GAME_AWayOut, GAME_UE4_18, GAME_KingdomHearts3, GAME_FinalFantasy7Remake,
        GAME_AceCombat7, GAME_FridayThe13th, GAME_GameForPeace, GAME_DragonQuestXI,
        GAME_CodeVein, GAME_UE4_19, GAME_Paragon, GAME_Ashen,
        GAME_UE4_20, GAME_Borderlands3, GAME_UE4_21, GAME_StarWarsJediFallenOrder,
        GAME_Undawn, GAME_UE4_22, GAME_UE4_23, GAME_ApexLegendsMobile,
        GAME_UE4_24, GAME_TonyHawkProSkater12, GAME_BigRumbleBoxingCreedChampions, GAME_AssaultFireFuture,
        GAME_UE4_25, GAME_UE4_25_Plus, GAME_RogueCompany, GAME_DeadIsland2,
        GAME_KenaBridgeofSpirits, GAME_Strinova, GAME_SYNCED, GAME_OperationApocalypse,
        GAME_Farlight84, GAME_StarWarsHunters, GAME_ThePathless, GAME_SuicideSquad,
        GAME_HellLetLoose, GAME_AliensFireteamElite, GAME_Back4Blood, GAME_NiNoKuniCrossWorlds,
        GAME_UE4_26, GAME_GTATheTrilogyDefinitiveEdition, GAME_ReadyOrNot, GAME_BladeAndSoul,
        GAME_TowerOfFantasy, GAME_FinalFantasy7Rebirth, GAME_TheDivisionResurgence, GAME_StarWarsJediSurvivor,
        GAME_Snowbreak, GAME_TorchlightInfinite, GAME_QQ, GAME_WutheringWaves,
        GAME_DreamStar, GAME_MidnightSuns, GAME_FragPunk, GAME_RacingMaster,
        GAME_StellarBlade, GAME_EtheriaRestart, GAME_EvilWest, GAME_ArenaBreakoutInfinite,
        GAME_Psychonauts2, GAME_OctopathTravelerCoTC, GAME_DarkPicturesAnthologyHouseOfAshes, GAME_DarkPicturesAnthologyManofMedan,
        GAME_DarkPicturesAnthologyTheDevilinMe, GAME_DarkPicturesAnthologyLittleHope, GAME_TheQuarry, GAME_RocoKingdomWorld,
        GAME_HonorofKingsWorld, GAME_eFootball, GAME_ArenaBreakoutMobile, GAME_ValorantSource,
        GAME_UE4_27, GAME_Splitgate, GAME_HYENAS, GAME_HogwartsLegacy,
        GAME_OutlastTrials, GAME_Valorant_PRE_11_2, GAME_Gollum, GAME_Grounded,
        GAME_DeltaForce, GAME_MortalKombat1, GAME_VisionsofMana, GAME_Spectre,
        GAME_KartRiderDrift, GAME_ThroneAndLiberty, GAME_MotoGP24, GAME_Stray,
        GAME_CrystalOfAtlan, GAME_PromiseMascotAgency, GAME_TerminullBrigade, GAME_AshEchoes,
        GAME_NeedForSpeedMobile, GAME_TonyHawkProSkater34, GAME_OnePieceAmbition, GAME_UnchartedWatersOrigin,
        GAME_LostSoulAside, GAME_GhostsofTabor, GAME_BlueProtocol, GAME_LittleNightmares3,
        GAME_Raven2, GAME_DuetNightAbyss, GAME_LiesofP, GAME_BloodBowl3,
        GAME_ChasingKaleidoRIDER, GAME_Lego2KDrive, GAME_CenturyAgeofAshes, GAME_EmbersofTheUncrowned,
        GAME_eBaseballProSpirit, GAME_UE4_28, GAME_UE5_0, GAME_MeetYourMaker,
        GAME_BlackMythWukong, GAME_UE5_EA, GAME_UE5_1, GAME_3on3FreeStyleRebound,
        GAME_Stalker2, GAME_TheCastingofFrankStone, GAME_SilentHill2Remake, GAME_Dauntless,
        GAME_WorldofJadeDynasty, GAME_LordsoftheFallen, GAME_Palworld, GAME_UE5_2,
        GAME_Placeholder5, GAME_PaxDei, GAME_TheFirstDescendant, GAME_MetroAwakening,
        GAME_LostRecordsBloomAndRage, GAME_DuneAwakening, GAME_Placeholder4, GAME_PUBGBlackBudget,
        GAME_UE5_3, GAME_MarvelRivals, GAME_BlackStigma, GAME_Valorant,
        GAME_ArcRaiders, GAME_Aion2, GAME_TheFinals, GAME_Avowed,
        GAME_MetalGearSolidDelta, GAME_Highguard, GAME_DragonSwordAwakening, GAME_UE5_4,
        GAME_FunkoFusion, GAME_InfinityNikki, GAME_SilverPalace, GAME_Gothic1Remake,
        GAME_SplitFiction, GAME_WildAssault, GAME_InZOI, GAME_TempestRising,
        GAME_MindsEye, GAME_DeadByDaylight_Old, GAME_Placeholder1, GAME_MafiaTheOldCountry,
        GAME_2XKO, GAME_Reanimal, GAME_VEIN, GAME_Placeholder2,
        GAME_OuterWorlds2, GAME_OctopathTraveler0, GAME_CodeVein2, GAME_UE5_5,
        GAME_Brickadia, GAME_Splitgate2, GAME_DeadzoneRogue, GAME_Directive8020,
        GAME_Wildgate, GAME_ARKSurvivalAscended, GAME_NevernessToEverness_CBT2, GAME_FateTrigger,
        GAME_Placeholder6, GAME_Borderlands4, GAME_Rennsport, GAME_GrayZoneWarfare,
        GAME_IntotheRadius2, GAME_HighOnLife2, GAME_MongilStarDive, GAME_UE5_6,
        GAME_Grounded2, GAME_AshesOfCreation, GAME_Solasta2, GAME_NevernessToEverness,
        GAME_DeadByDaylight, GAME_ConanExilesEnhanced, GAME_Subnautica2, GAME_LEGOBatmanLegacyoftheDarkKnight,
        GAME_Fatekeeper, GAME_Enginefall, GAME_UE5_7, GAME_TitanQuest2,
        GAME_Squad, GAME_Empulse, GAME_LordOfMysteries, GAME_UE5_8,
        GAME_WutheringWavesFastGeo, GAME_UE5_9, GAME_UE6_0
        };

        count = sizeof(values) / sizeof(values[0]);
        return values;
    }
}
