#pragma once

#include "../ue4.h"
#include "../SDK.hpp"
#include "../Logic/Teams.h"
#include "../Logic/Inventory.h"
#include "../Logic/Abilities.h"

class IGameModeBase
{
public:
    virtual void OnPlayerJoined(AFortPlayerControllerAthena* Controller) = 0;
    virtual void OnPlayerKilled(AFortPlayerControllerAthena* Controller) = 0;
};

struct LootData
{
    UFortItemDefinition* ItemDef;
    int Count;
};

class AbstractGameModeBase : protected IGameModeBase
{
public:
    AbstractGameModeBase(const std::string BasePlaylist, bool bRespawnEnabled = false, int maxTeamSize = 1, bool bRegenEnabled = false, bool bRejoinEnabled = false)
    {
        this->bRespawnEnabled = bRespawnEnabled;
        this->bRegenEnabled = bRegenEnabled;
        this->bRejoinEnabled = bRejoinEnabled;
        this->maxTeamSize = maxTeamSize;
        this->BasePlaylistName = BasePlaylist;
        this->Teams = std::make_unique<PlayerTeams>(maxTeamSize);
    }

    ~AbstractGameModeBase()
    {
        GetWorld()->GameState->AuthorityGameMode->ResetLevel();
    }

    void BaseInitialize()
    {
        this->BasePlaylist = UObject::FindObject<UFortPlaylistAthena>(BasePlaylistName);

        this->BasePlaylist->bNoDBNO = this->maxTeamSize > 1;
        /* Rejoin can be disabled for certain gamemodes only. To do this, leave the rejoin bool as false, go to the file of the gamemode you want to play (e.g. Playground), and find the line that says "AbstractGameModeBase(PlaylistName, true, 1)"
        Change it to "AbstractGameModeBase(PlaylistName, true, 1, false, true)". Now you have rejoins enabled for Playground */

        if (bRespawnEnabled)
        {
            this->BasePlaylist->FriendlyFireType = EFriendlyFireType::On;
            this->BasePlaylist->RespawnLocation = EAthenaRespawnLocation::Air;
            this->BasePlaylist->RespawnType = EAthenaRespawnType::InfiniteRespawn;
        }

        auto GameState = static_cast<AAthena_GameState_C*>(GetWorld()->GameState);

        GameState->CurrentPlaylistId = this->BasePlaylist->PlaylistId;
        GameState->CurrentPlaylistData = this->BasePlaylist;

        GameState->OnRep_CurrentPlaylistId();
        GameState->OnRep_CurrentPlaylistData();

        Initialize();
    }

    virtual void Initialize()
    {

    }

    // TODO: Make this virtual for gamemode to have custom loot ?
    std::vector<LootData> GetLoot(std::string TierGroup)
    {
        std::vector<LootData> ret;
        if (!ltd.contains(TierGroup))
        {
            LOG_ERROR("Invalid TierGroup name {}", TierGroup);
            return ret;
        }
        // Just ignore how messy this is, i've been staring at FModel for hours trying to figure this out and i still don't fully understand it
        auto ok = Utils::WeightedRand(ltd[TierGroup]);

        auto drops = std::vector<int>();
        for (int j = 0; j < ok.LootPackageCategoryWeightArray.Count; j++)
        {
            if (ok.LootPackageCategoryMinArray[j] > 0)
            {
                //drops += ((UKismetMathLibrary*)UKismetMathLibrary::StaticClass())->STATIC_RandomIntegerInRange(ok.LootPackageCategoryWeightArray[j], ok.LootPackageCategoryMinArray[j]);
                drops.push_back(((UKismetMathLibrary*)UKismetMathLibrary::StaticClass())->STATIC_RandomIntegerInRange(ok.LootPackageCategoryWeightArray[j], ok.LootPackageCategoryMinArray[j]));
            }
        }

        auto ok2 = lpd[ok.LootPackage.ToString()];
        for (int j = 0; j < drops.size(); j++)
        {
            if (j >= ok2.size())
                continue;
            
            for (int k = 0; k < drops[j]; k++)
            {
                FFortLootPackageData ok3;
                if (!ok2[j].LootPackageCall.IsValid() || ok2[j].LootPackageCall.Count <= 0)
                    ok3 = Utils::WeightedRand(lpd[ok2[j].LootPackageID.ToString()]);
                else
                    ok3 = Utils::WeightedRand(lpd[ok2[j].LootPackageCall.ToString()]);

                auto unk = *(TSoftObjectPtr<UObject*>*)&ok3.UnknownData01;

                bool addedit = false;
                auto obj = (UFortItemDefinition*)UObject::GObjects->GetByIndex(unk.WeakPtr.ObjectIndex);
                if (obj)
                {
                    for (int l = 0; l < ret.size(); l++)
                    {
                        if (ret[l].ItemDef == obj)
                        {
                            ret[l].Count += ok3.Count;
                            addedit = true;
                        }
                    }

                    if (!addedit)
                        ret.push_back({ obj, ok3.Count });
                }
                else
                {
                    LOG_INFO("Object name \"{}\" was not found", unk.ObjectID.AssetPathName.ToString())
                }
            }
        }

        return ret;
    }

    void InitLoot()
    {
        UDataTable* loottierdata = nullptr;
        UDataTable* lootpackage = nullptr;
        // TODO: Get datatables from playlist?
        bool gotdata = false;
        bool gotpack = false;
        if (this->BasePlaylist) // Idk if this works
        {

            auto PlaylistLtd = Spawners::LoadObject<UDataTable>(UDataTable::StaticClass(), this->BasePlaylist->LootTierData.ObjectID.AssetPathName.ToWString(true).c_str());
            auto PlaylistLpd = Spawners::LoadObject<UDataTable>(UDataTable::StaticClass(), this->BasePlaylist->LootPackages.ObjectID.AssetPathName.ToWString(true).c_str());

            if (PlaylistLtd)
            {
                loottierdata = PlaylistLtd;
                gotdata = true;
            }
            if (PlaylistLpd)
            {
                lootpackage = PlaylistLpd;
                gotpack = true;
            }
        }
        if (!gotdata || !gotpack)
        {
            LOG_WARN("Failed to get LootTierData and LootPackages from playlist\ngotdata: {}\ngotpack: {}", gotdata, gotpack);
            loottierdata = UObject::FindObject<UDataTable>("DataTable AthenaLootTierData_Client.AthenaLootTierData_Client");
            lootpackage = UObject::FindObject<UDataTable>("DataTable AthenaLootPackages_Client.AthenaLootPackages_Client");
        }
        
        if (loottierdata)
        {
            for (auto Pair : loottierdata->RowMap)
            {
                auto rowptr = Pair.Value();
                auto row = *(FFortLootTierData*)rowptr;
                this->ltd[row.TierGroup.ToString()].push_back(row);
            }
        }

        if (lootpackage)
        {

            for (auto Pair : lootpackage->RowMap)
            {
                auto rowptr = Pair.Value();
                auto row = *(FFortLootPackageData*)rowptr;
                this->lpd[row.LootPackageID.ToString()].push_back(row);
            }
        }
    }

    bool isRespawnEnabled()
    {
        return this->bRespawnEnabled;
    }

    void LoadKilledPlayer(AFortPlayerControllerAthena* Controller, FVector Spawn = { 500, 500, 500 })
    {
        if (this->bRespawnEnabled)
        {
            if (Controller->Pawn)
            {
                Controller->Pawn->K2_DestroyActor();
            }

            InitPawn(Controller, Spawn);
            Controller->ActivateSlot(EFortQuickBars::Primary, 0, 0, true);

            bool bFound = false;
            auto PickaxeEntry = Inventory::FindItemInInventory<UFortWeaponMeleeItemDefinition>(Controller, bFound);
            if (bFound)
            {
                Inventory::EquipInventoryItem(Controller, PickaxeEntry.ItemGuid);
            }

            LOG_INFO("({}) Re-initializing {} that has been killed (bRespawnEnabled == true)!", "GameModeBase", Controller->PlayerState->GetPlayerName().ToString());
        }
    }
    
    inline void InitCharParts(AFortPlayerControllerAthena* Controller)
    {
        auto PlayerState = (AFortPlayerState*)Controller->PlayerState;
        auto CharDef = Controller->CustomizationLoadout.Character;
        // This is actually unreadable lol
        auto HeroType = *(UFortHeroType**)((uintptr_t)CharDef + 0x0358);
        auto Specialization = (UFortHeroSpecialization*)(*(TArray<TSoftObjectPtr<UFortHeroSpecialization*>>*)(&HeroType->UnknownData01))[0].WeakPtr.Get();
        auto CharacterParts = *(TArray<TSoftObjectPtr<UObject*>>*)(&Specialization->UnknownData01);

        if (HeroType)
        {
            PlayerState->HeroType = HeroType;
            PlayerState->OnRep_HeroType();

            for (int i = 0; i < CharacterParts.Count; i++)
            {
                auto part = Spawners::LoadObject<UCustomCharacterPart>(UCustomCharacterPart::StaticClass(), CharacterParts[i].ObjectID.AssetPathName.ToWString(true).c_str());
                PlayerState->CharacterParts[(uint8_t)part->CharacterPartType.GetValue()] = part;
            }

            PlayerState->OnRep_CharacterParts();
        }
    }

    void LoadJoiningPlayer(AFortPlayerControllerAthena* Controller)
    {
        LOG_INFO("({}) Initializing {} that has just joined!", "GameModeBase", Controller->PlayerState->GetPlayerName().ToString());

        auto Pawn = Spawners::SpawnActor<APlayerPawn_Athena_C>(GetPlayerStart(Controller).Translation, Controller, {});
        Pawn->Owner = Controller;
        Pawn->OnRep_Owner();

        Pawn->NetCullDistanceSquared = 0.f;

        Controller->Pawn = Pawn;
        Controller->AcknowledgedPawn = Pawn;
        Controller->OnRep_Pawn();
        Controller->Possess(Pawn);

        Pawn->HealthSet->Health.Minimum = 0;
        Pawn->HealthSet->CurrentShield.Minimum = 0;

        Pawn->SetMaxHealth(this->maxHealth);
        Pawn->SetMaxShield(this->maxShield);

        Pawn->bCanBeDamaged = bStartedBus;

        Controller->bIsDisconnecting = false;
        Controller->bHasClientFinishedLoading = true;
        Controller->bHasServerFinishedLoading = true;
        Controller->bHasInitiallySpawned = true;
        Controller->OnRep_bHasServerFinishedLoading();

        auto PlayerState = (AFortPlayerStateAthena*)Controller->PlayerState;
        PlayerState->bHasFinishedLoading = true;
        PlayerState->bHasStartedPlaying = true;
        PlayerState->OnRep_bHasStartedPlaying();

        static auto FortRegisteredPlayerInfo = ((UFortGameInstance*)GetWorld()->OwningGameInstance)->RegisteredPlayers[0]; // UObject::FindObject<UFortRegisteredPlayerInfo>("FortRegisteredPlayerInfo Transient.FortEngine_0_1.FortGameInstance_0_1.FortRegisteredPlayerInfo_0_1");
        static auto RandomCharacters = static_cast<UFortAssetManager*>(GetEngine()->AssetManager)->AthenaGameData->RandomCharacters;

        Controller->CustomizationLoadout.Character = RandomCharacters[GetKismetMath()->STATIC_RandomIntegerInRange(0, RandomCharacters.Count - 1)];
        
        InitCharParts(Controller);

        Inventory::Init(Controller);
        Inventory::Update(Controller);
        //Inventory::EquipLoadout(Controller, this->GetPlaylistLoadout()); // TODO: Make this better
        Abilities::ApplyAbilities(Pawn);

        auto Drone = Spawners::SpawnActor<ABP_VictoryDrone_C>(Controller->K2_GetActorLocation());
        Drone->InitDrone();
        Drone->TriggerPlayerSpawnEffects();

        if (!bFloortLootSpawned)
        {
            InitLoot();

            TArray<AActor*> floots;
            GetGameplayStatics()->STATIC_GetAllActorsOfClass(GetWorld(), UObject::FindClass("BlueprintGeneratedClass Tiered_Athena_FloorLoot_01.Tiered_Athena_FloorLoot_01_C"), &floots);
            LOG_INFO("Found {} floor loot spots", floots.Count);
            for (int i = 0; i < floots.Count; i++)
            {
                auto floot = (ABuildingContainer*)floots[i];

                auto loot = GetLoot("Loot_AthenaFloorLoot");
                for (int j = 0; j < loot.size(); j++)
                {
                    auto Location = floot->K2_GetActorLocation();
                    //Location.Z += 42; // scuffed but it worked for chests

                    Spawners::SpawnPickupFromFloorLoot(loot[j].ItemDef, loot[j].Count, Location);
                }
            }
            floots.FreeArray();
            GetGameplayStatics()->STATIC_GetAllActorsOfClass(GetWorld(), UObject::FindClass("BlueprintGeneratedClass Tiered_Athena_FloorLoot_Warmup.Tiered_Athena_FloorLoot_Warmup_C"), &floots);
            LOG_INFO("Found {} warmup floor loot spots", floots.Count);
            for (int i = 0; i < floots.Count; i++)
            {
                auto floot = (ABuildingContainer*)floots[i];

                auto loot = GetLoot("Loot_AthenaFloorLoot_Warmup");
                for (int j = 0; j < loot.size(); j++)
                {
                    auto Location = floot->K2_GetActorLocation();
                    Location.Z += 42; // scuffed but it worked for chests

                    Spawners::SpawnPickupFromFloorLoot(loot[j].ItemDef, loot[j].Count, Location);
                }
            }
            floots.FreeArray();

            //GetGameplayStatics()->STATIC_GetAllActorsOfClass(GetWorld(), UObject::FindClass("BlueprintGeneratedClass B_Athena_VendingMachine.B_Athena_VendingMachine_C"), &floots);
            //LOG_INFO("Found {} vending machines", floots.Count);
            //for (int i = 0; i < floots.Count; i++)
            //{
            //    auto floot = (ABuildingItemCollectorActor*)floots[i];

            //    LOG_INFO("Flooter: {}", floot->ItemCollections.Count);
            //    
            //    static auto inpitem = UObject::FindObject<UFortResourceItemDefinition>("FortResourceItemDefinition WoodItemData.WoodItemData");
            //    static auto outitem = UObject::FindObject<UFortResourceItemDefinition>("FortResourceItemDefinition StoneItemData.StoneItemData");
            //    static auto outinstance = (UFortWorldItem*)outitem->CreateTemporaryItemInstanceBP(1, 1);

            //    FColletorUnitInfo unitinfo;
            //    unitinfo.InputCount.Value = 200;
            //    unitinfo.InputItem = inpitem;
            //    unitinfo.OutputItem = outitem;
            //    unitinfo.OutputItemEntry.Add(outinstance->ItemEntry);
            //    floot->ItemCollections[0] = unitinfo;
            //    floot->ItemCollections[1] = unitinfo;
            //    floot->ItemCollections[2] = unitinfo;
            //}
            //floots.FreeArray();
        }

        OnPlayerJoined(Controller);
    }

    void OnPlayerJoined(AFortPlayerControllerAthena* Controller) override // derived classes should implement these
    {
    }

    virtual void OnPlayerKilled(AFortPlayerControllerAthena* Controller) override
    {
        if (Controller && !IsCurrentlyDisconnecting(Controller->NetConnection) && this->bRespawnEnabled)
        {
            LOG_INFO("Trying to respawn {}", Controller->PlayerState->GetPlayerName().ToString());
            // -Kyiro TO-DO: See if most of this code is even needed but it does work
            FVector RespawnPos = Controller->Pawn ? Controller->Pawn->K2_GetActorLocation() : FVector(10000, 10000, 10000);
            RespawnPos.Z += 3000;

            this->LoadKilledPlayer(Controller, RespawnPos);
            Controller->RespawnPlayerAfterDeath();

            if (Controller->Pawn->K2_TeleportTo(RespawnPos, FRotator { 0, 0, 0 }))
            {
                Controller->Character->CharacterMovement->SetMovementMode(EMovementMode::MOVE_Custom, 4);
            }
            else
            {
                LOG_ERROR("Failed to teleport {}", Controller->PlayerState->GetPlayerName().ToString())
            }

            // auto CheatManager = static_cast<UFortCheatManager*>(Controller->CheatManager);
            // CheatManager->RespawnPlayerServer();
            // CheatManager->RespawnPlayer();
        }
    }

    virtual PlayerLoadout& GetPlaylistLoadout()
    {
        static PlayerLoadout Ret = {
            FindWID("WID_Harvest_Pickaxe_Athena_C_T01"),
            FindWID("WID_Shotgun_Standard_Athena_UC_Ore_T03"), // Blue Pump
            FindWID("WID_Shotgun_Standard_Athena_UC_Ore_T03"), // Blue Pump
            FindWID("WID_Assault_AutoHigh_Athena_SR_Ore_T03"), // Gold AR
            FindWID("WID_Sniper_BoltAction_Scope_Athena_R_Ore_T03"), // Blue Bolt Action
            //FindWID("Athena_Shields") // Big Shield Potion
            // FindWID("Athena_Medkit"),
            // FindWID("Athena_Medkit"),
            // FindWID("Athena_Medkit"),
            // FindWID("Athena_Medkit"),
            FindWID("Athena_Medkit")

        };

        return Ret;
    }

    void InitPawn(AFortPlayerControllerAthena* PlayerController, FVector Loc = FVector { 1250, 1818, 3284 }, FQuat Rotation = FQuat(), bool bResetCharacterParts = false)
    {
        if (PlayerController->Pawn)
            PlayerController->Pawn->K2_DestroyActor();

        auto SpawnTransform = FTransform();
        SpawnTransform.Scale3D = FVector(1, 1, 1);
        SpawnTransform.Rotation = Rotation;
        SpawnTransform.Translation = Loc;

        // SpawnTransform = GetPlayerStart(PlayerController);

        auto Pawn = static_cast<APlayerPawn_Athena_C*>(Spawners::SpawnActor(APlayerPawn_Athena_C::StaticClass(), SpawnTransform, PlayerController));

        PlayerController->Pawn = Pawn;
        PlayerController->AcknowledgedPawn = Pawn;
        Pawn->Owner = PlayerController;
        Pawn->OnRep_Owner();
        PlayerController->OnRep_Pawn();
        PlayerController->Possess(Pawn);

        Pawn->SetMaxHealth(this->maxHealth);
        Pawn->SetMaxShield(this->maxShield);

        if (!this->bRegenEnabled)
        {
            Pawn->AbilitySystemComponent->RemoveActiveGameplayEffectBySourceEffect(Pawn->HealthRegenDelayGameplayEffect, Pawn->AbilitySystemComponent, 1);
            Pawn->AbilitySystemComponent->RemoveActiveGameplayEffectBySourceEffect(Pawn->HealthRegenGameplayEffect, Pawn->AbilitySystemComponent, 1);
            Pawn->AbilitySystemComponent->RemoveActiveGameplayEffectBySourceEffect(Pawn->ShieldRegenDelayGameplayEffect, Pawn->AbilitySystemComponent, 1);
            Pawn->AbilitySystemComponent->RemoveActiveGameplayEffectBySourceEffect(Pawn->ShieldRegenGameplayEffect, Pawn->AbilitySystemComponent, 1);
            // TODO: Check if this is useless.
            Pawn->HealthRegenDelayGameplayEffect = nullptr;
            Pawn->HealthRegenGameplayEffect = nullptr;
            Pawn->ShieldRegenDelayGameplayEffect = nullptr;
            Pawn->ShieldRegenGameplayEffect = nullptr;
        }

        Pawn->HealthSet->Health.Minimum = 0.0f; // Disables spawn island protection
        Pawn->HealthSet->CurrentShield.Minimum = 0.0f;

        Pawn->bReplicateMovement = true;
        Pawn->OnRep_ReplicateMovement();
        
        if (bResetCharacterParts)
            InitCharParts(PlayerController);

        Inventory::Update(PlayerController);

        Abilities::ApplyAbilities(Pawn);
    }

protected:
    std::unique_ptr<PlayerTeams> Teams;

private:
    int maxHealth = 100;
    int maxShield = 100;
    int maxTeamSize = 1;
    bool bRespawnEnabled = false;
    bool bRegenEnabled = false;
    bool bRejoinEnabled = false;
    bool bFloortLootSpawned = false;
    std::string BasePlaylistName = "";
    // Looting stuff
    std::map<std::string, std::vector<FFortLootTierData>> ltd;
    std::map<std::string, std::vector<FFortLootPackageData>> lpd;

public:
    UFortPlaylistAthena* BasePlaylist;
};
