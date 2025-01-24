#pragma once

static bool bStartedBus = false;

#include "UE4.h"
#include "GameModes/GameModes.hpp"

namespace Game
{
    inline std::unique_ptr<AbstractGameModeBase> Mode;

    void Start()
    {
        GetPlayerController()->SwitchLevel(L"Athena_Terrain?game=/Game/Athena/Athena_GameMode.Athena_GameMode_C");
        bTraveled = true;
    }

    void OnReadyToStartMatch()
    {
        LOG_INFO("Initializing the match for the server!");
        auto GameState = reinterpret_cast<SDK::AAthena_GameState_C*>(GetWorld()->GameState);
        auto GameMode = reinterpret_cast<SDK::AFortGameModeAthena*>(GetWorld()->AuthorityGameMode);
        auto InProgress = GetKismetString()->STATIC_Conv_StringToName(L"InProgress");

        GameState->bGameModeWillSkipAircraft = true;
        GameState->AircraftStartTime = 9999.9f;
        GameState->WarmupCountdownEndTime = 99999.9f;

        GameState->GamePhase = SDK::EAthenaGamePhase::Warmup;
        GameState->OnRep_GamePhase(SDK::EAthenaGamePhase::None);

        GameMode->bDisableGCOnServerDuringMatch = true;
        GameMode->bAllowSpectateAfterDeath = true;
        GameMode->bEnableReplicationGraph = true;

        GameMode->MatchState = InProgress;
        GameMode->K2_OnSetMatchState(InProgress);

        Mode->BaseInitialize();

        GameMode->MinRespawnDelay = 5.0f;
        GameMode->StartPlay();

        GameState->bReplicatedHasBegunPlay = true;
        GameState->OnRep_ReplicatedHasBegunPlay();
        GameMode->StartMatch();
        GameMode->bWorldIsReady = true;
    }

    auto GetDeathCause(SDK::FFortPlayerDeathReport DeathReport)
    {
        static std::map<std::string, SDK::EDeathCause> DeathCauses {
            { "weapon.ranged.shotgun", SDK::EDeathCause::Shotgun },
            { "weapon.ranged.assault", SDK::EDeathCause::Rifle },
            { "Gameplay.Damage.Environment.Falling", SDK::EDeathCause::FallDamage },
            { "weapon.ranged.sniper", SDK::EDeathCause::Sniper },
            { "Weapon.Ranged.SMG", SDK::EDeathCause::SMG },
            { "weapon.ranged.heavy.rocket_launcher", SDK::EDeathCause::RocketLauncher },
            { "weapon.ranged.heavy.grenade_launcher", SDK::EDeathCause::GrenadeLauncher },
            { "Weapon.ranged.heavy.grenade", SDK::EDeathCause::Grenade },
            { "Weapon.Ranged.Heavy.Minigun", SDK::EDeathCause::Minigun },
            { "Weapon.Ranged.Crossbow", SDK::EDeathCause::Bow },
            { "trap.floor", SDK::EDeathCause::Trap },
            { "weapon.ranged.pistol", SDK::EDeathCause::Pistol },
            { "Gameplay.Damage.OutsideSafeZone", SDK::EDeathCause::OutsideSafeZone },
            { "Weapon.Melee.Impact.Pickaxe", SDK::EDeathCause::Melee }
        };

        for (int i = 0; i < DeathReport.Tags.GameplayTags.Num(); i++)
        {
            auto TagName = DeathReport.Tags.GameplayTags[i].TagName.ToString();

            for (auto Map : DeathCauses)
            {
                if (TagName == Map.first)
                    return Map.second;
                else
                    continue;
            }
        }

        return SDK::EDeathCause::Unspecified;
    }

    // Can't be in utils 
    static SDK::UFortResourceItemDefinition* EFortResourceTypeToItemDef(SDK::EFortResourceType ResourceType)
    {
        SDK::UFortResourceItemDefinition* ret = nullptr;
        if (ResourceType == SDK::EFortResourceType::Wood)
            ret = static_cast<SDK::UFortAssetManager*>(GetEngine()->AssetManager)->GameData->WoodItemDefinition;
        else if (ResourceType == SDK::EFortResourceType::Stone)
            ret = static_cast<SDK::UFortAssetManager*>(GetEngine()->AssetManager)->GameData->StoneItemDefinition;
        else if (ResourceType == SDK::EFortResourceType::Metal)
            ret = static_cast<SDK::UFortAssetManager*>(GetEngine()->AssetManager)->GameData->MetalItemDefinition;
        else if (ResourceType == SDK::EFortResourceType::Permanite)
            ret = static_cast<SDK::UFortAssetManager*>(GetEngine()->AssetManager)->GameData->PermaniteItemDefinition;
        return ret;
    }
}
