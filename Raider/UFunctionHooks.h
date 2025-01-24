#pragma once
#pragma warning(disable : 4099)

#include <functional>

#include "Logic/Game.h"
#include "Logic/Spawners.h"
#include "Logic/Abilities.h"
#include "Logic/Building.h"
#include "UE4.h"

// #define LOGGING
#define CHEATS
#define MAXPLAYERS 100

// Define the hook with ufunction full name
// Return true in the lambda to prevent the original function call

namespace UFunctionHooks
{
    inline std::vector<SDK::UFunction*> toHook;
    inline std::vector<std::function<bool(SDK::UObject*, void*)>> toCall;

#define DEFINE_PEHOOK(ufunctionName, func)                           \
    toHook.push_back(SDK::UObject::FindObject<SDK::UFunction>(ufunctionName)); \
    toCall.push_back([](SDK::UObject * Object, void* Parameters) -> bool func);

    auto Initialize()
    {
        DEFINE_PEHOOK("Function GameplayAbilities.AbilitySystemComponent.ServerTryActivateAbility", {
            auto AbilitySystemComponent = (SDK::UAbilitySystemComponent*)Object;
            auto Params = (SDK::UAbilitySystemComponent_ServerTryActivateAbility_Params*)Parameters;

            Abilities::TryActivateAbility(AbilitySystemComponent, Params->AbilityToActivate, Params->InputPressed, &Params->PredictionKey, nullptr);

            return false;
        })

        DEFINE_PEHOOK("Function GameplayAbilities.AbilitySystemComponent.ServerTryActivateAbilityWithEventData", {
            auto AbilitySystemComponent = (SDK::UAbilitySystemComponent*)Object;
            auto Params = (SDK::UAbilitySystemComponent_ServerTryActivateAbilityWithEventData_Params*)Parameters;

            Abilities::TryActivateAbility(AbilitySystemComponent, Params->AbilityToActivate, Params->InputPressed, &Params->PredictionKey, &Params->TriggerEventData);

            return false;
        })

        DEFINE_PEHOOK("Function GameplayAbilities.AbilitySystemComponent.ServerAbilityRPCBatch", {
            auto AbilitySystemComponent = (SDK::UAbilitySystemComponent*)Object;
            auto Params = (SDK::UAbilitySystemComponent_ServerAbilityRPCBatch_Params*)Parameters;

            Abilities::TryActivateAbility(AbilitySystemComponent, Params->BatchInfo.AbilitySpecHandle, Params->BatchInfo.InputPressed, &Params->BatchInfo.PredictionKey, nullptr);

            return false;
        })

        DEFINE_PEHOOK("Function FortniteGame.FortPlayerPawn.ServerHandlePickup", {
            Inventory::OnPickup((SDK::AFortPlayerControllerAthena*)((SDK::APawn*)Object)->Controller, Parameters);
            return false;
        })

        DEFINE_PEHOOK("Function FortniteGame.FortWeapon.GameplayCue_Weapons_Activation", {
            if (!Object->IsA(SDK::AB_RCRocket_Launcher_Athena_C::StaticClass()))
                return false;

            auto Weapon = (SDK::AFortWeapon*)Object;
            auto PlayerPawn = (SDK::AFortPlayerPawnAthena*)Weapon->Owner;
            auto PlayerController = (SDK::AFortPlayerControllerAthena*)PlayerPawn->Controller;
            auto Trans = PlayerPawn->GetTransform();
            Trans.Translation.Z += 1000;
            auto RocketPawn = static_cast<SDK::AB_PrjPawn_Athena_RCRocket_C*>(Spawners::SpawnActor(SDK::AB_PrjPawn_Athena_RCRocket_C::StaticClass(), Trans, nullptr));
            PlayerController->Possess(RocketPawn);

            // This crashes:
            // RocketPawn->SetupRemoteControlPawn(PlayerController, PlayerPawn, EFortCustomMovement::RemoteControl_Flying, RocketPawn->EffectContainerSpecToApplyOnKill);
            
            
            return false;
        })

#ifdef CHEATS
        DEFINE_PEHOOK("Function FortniteGame.FortPlayerController.ServerCheat", {
            if (Object->IsA(SDK::AFortPlayerControllerAthena::StaticClass()))
            {
                auto PC = (SDK::AFortPlayerControllerAthena*)Object;
                auto Params = (SDK::AFortPlayerController_ServerCheat_Params*)Parameters;

                if (Params && PC && !PC->IsInAircraft())
                {
                    auto Pawn = (SDK::APlayerPawn_Athena_C*)PC->Pawn;
                    auto Message = Params->Msg.ToString();

                    std::vector<std::string> Arguments;

                    int i = 0;
                    bool CurrentlyHandlingString = false;
                    std::string CurrentArg = "";
                    while (i < Message.size())
                    {
                        auto CurrentIndex = Message[i];
                        if (!CurrentlyHandlingString && isspace(CurrentIndex) && CurrentArg != "")
                        {
                            Arguments.push_back(CurrentArg);
                            CurrentArg = "";
                        }
                        else if (CurrentIndex == '"')
                        {
                            if (!CurrentlyHandlingString)
                            {
                                CurrentlyHandlingString = true;
                            }
                            else
                            {
                                CurrentlyHandlingString = false;
                                Arguments.push_back(CurrentArg);
                                CurrentArg = "";
                            }
                        }
                        else
                        {
                            CurrentArg += CurrentIndex;
                        }

                        i++;
                    }

                    if (CurrentArg != "")
                    {
                        Arguments.push_back(CurrentArg);
                        CurrentArg = "";
                    }
                    
                    auto NumArgs = Arguments.size();

                    if (NumArgs >= 0)
                    {
                        auto& Command = Arguments[0];
                        std::transform(Command.begin(), Command.end(), Command.begin(), ::tolower);

                        if (Command == "infammo")
                        {
                            PC->bInfiniteAmmo = !PC->bInfiniteAmmo;
                        }
                        else if (Command == "spawnpickup")
                        {
                            LOG_INFO("Spawning pickup for {}", Arguments[1]);
                            auto ItemDef = SDK::UObject::FindObject<SDK::UFortItemDefinition>(Arguments[1]);
                            int Count = 1;
                            if (NumArgs >= 3)
                                Count = stoi(Arguments[2]);
                            
                            Spawners::SummonPickup(Pawn, ItemDef, Count, Pawn->K2_GetActorLocation());
                        }

                        else
                            PC->ClientMessage(L"Unable to handle command!", SDK::FName(), 0);
                    }
                }
            }

            return false;
        })
#endif

        DEFINE_PEHOOK("Function Engine.CheatManager.CheatScript", {
            return false;
        })

        DEFINE_PEHOOK("Function FortniteGame.BuildingActor.OnDeathServer", {
            auto Actor = (SDK::ABuildingActor*)Object;

            if (Actor)
            {
                if (Actor->IsA(SDK::ABuildingSMActor::StaticClass()))
                {
                    for (int i = 0; i < ExistingBuildings.size(); i++)
                    {
                        auto Building = ExistingBuildings[i];

                        if (!Building)
                            continue;

                        if (Building == Actor)
                        {
                            ExistingBuildings.erase(ExistingBuildings.begin() + i);

                            break;
                        }
                    }
                }
            }

            return false;
        })

        DEFINE_PEHOOK("Function FortniteGame.FortPlayerController.ServerCreateBuildingActor", {
            auto PC = (SDK::AFortPlayerControllerAthena*)Object;

            auto Params = (SDK::AFortPlayerController_ServerCreateBuildingActor_Params*)Parameters;
            auto CurrentBuildClass = Params->BuildingClassData.BuildingClass;

            if (PC && Params && CurrentBuildClass)
            {
                //auto SupportSystem = reinterpret_cast<AAthena_GameState_C*>(GetWorld()->GameState)->StructuralSupportSystem;
                //if (SupportSystem->IsWorldLocValid(Params->BuildLoc))
                //{
                //    auto BuildingActor = (ABuildingSMActor*)Spawners::SpawnActor(CurrentBuildClass, Params->BuildLoc, Params->BuildRot, PC, ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn);
                //    TArray<ABuildingActor*> Existing;
                //    if (SupportSystem->K2_CanAddBuildingActorToGrid(GetWorld(), BuildingActor, Params->BuildLoc, Params->BuildRot, Params->bMirrored, false, &Existing) == EFortStructuralGridQueryResults::CanAdd && Existing.Count <= 0) 
                //    {

                //        BuildingActor->DynamicBuildingPlacementType = EDynamicBuildingPlacementType::DestroyAnythingThatCollides;
                //        BuildingActor->SetMirrored(Params->bMirrored);
                //    //    // BuildingActor->PlacedByPlacementTool();
                //        BuildingActor->InitializeKismetSpawnedBuildingActor(BuildingActor, PC);
                //        auto PlayerState = (AFortPlayerStateAthena*)PC->PlayerState;
                //        BuildingActor->Team = PlayerState->TeamIndex;
                //    }
                //    else
                //    {
                //        BuildingActor->SetActorScale3D({});
                //        BuildingActor->SilentDie();
                //    }
                //}
                
                auto BuildingActor = (SDK::ABuildingSMActor*)Spawners::SpawnActor(CurrentBuildClass, Params->BuildLoc, Params->BuildRot, PC, SDK::ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn);
                if (BuildingActor && CanBuild(BuildingActor))
                {
                    BuildingActor->DynamicBuildingPlacementType = SDK::EDynamicBuildingPlacementType::DestroyAnythingThatCollides;
                    BuildingActor->SetMirrored(Params->bMirrored);
                    // BuildingActor->PlacedByPlacementTool();
                    BuildingActor->InitializeKismetSpawnedBuildingActor(BuildingActor, PC);
                    auto PlayerState = (SDK::AFortPlayerStateAthena*)PC->PlayerState;
                    BuildingActor->Team = PlayerState->TeamIndex;

                    SDK::UFortResourceItemDefinition* matdef = Game::EFortResourceTypeToItemDef(BuildingActor->ResourceType);
                    
                    if (!PC->bInfiniteAmmo)
                    {
                        if (!Inventory::TryRemoveItem(PC, matdef, 10))
                        {
                            LOG_ERROR("Failed to remove resource from building")
                        }
                    }
                }
                else
                {
                    BuildingActor->SetActorScale3D({});
                    BuildingActor->SilentDie();
                }
            }

            return false;
        })

        DEFINE_PEHOOK("Function FortniteGame.FortPlayerController.ServerBeginEditingBuildingActor", {
            auto Params = (SDK::AFortPlayerController_ServerBeginEditingBuildingActor_Params*)Parameters;
            auto Controller = (SDK::AFortPlayerControllerAthena*)Object;
            auto Pawn = (SDK::APlayerPawn_Athena_C*)Controller->Pawn;
            bool bFound = false;
            auto EditToolEntry = Inventory::FindItemInInventory<SDK::UFortEditToolItemDefinition>(Controller, bFound);

            if (Controller && Pawn && Params->BuildingActorToEdit && bFound)
            {
                auto EditTool = (SDK::AFortWeap_EditingTool*)Inventory::EquipWeaponDefinition(Pawn, (SDK::UFortWeaponItemDefinition*)EditToolEntry.ItemDefinition, EditToolEntry.ItemGuid);

                if (EditTool)
                {
                    EditTool->EditActor = Params->BuildingActorToEdit;
                    EditTool->OnRep_EditActor();
                    Params->BuildingActorToEdit->EditingPlayer = (SDK::AFortPlayerStateZone*)Pawn->PlayerState;
                    Params->BuildingActorToEdit->OnRep_EditingPlayer();
                }
            }

            return false;
        })

        DEFINE_PEHOOK("Function FortniteGame.FortDecoTool.ServerSpawnDeco", {
            auto Tool = (SDK::AFortDecoTool*)Object;

            Spawners::SpawnDeco(Tool, Parameters);

            // TODO: Remove trap after use

            return false;
        })

        DEFINE_PEHOOK("Function FortniteGame.FortPlayerController.ServerEditBuildingActor", {
            auto Params = (SDK::AFortPlayerController_ServerEditBuildingActor_Params*)Parameters;
            auto PC = (SDK::AFortPlayerControllerAthena*)Object;

            if (PC && Params)
            {
                auto BuildingActor = Params->BuildingActorToEdit;
                auto NewBuildingClass = Params->NewBuildingClass;
                auto RotationIterations = Params->RotationIterations;

                if (BuildingActor && NewBuildingClass)
                {
                    auto location = BuildingActor->K2_GetActorLocation();
                    auto rotation = BuildingActor->K2_GetActorRotation();

                    auto ForwardVector = BuildingActor->GetActorForwardVector();
                    auto RightVector = BuildingActor->GetActorRightVector();

                    int yaw = round(float((int(rotation.Yaw) + 360) % 360) / 10) * 10; // Gets the rotation ranging from 0 to 360 degrees

                    if (BuildingActor->BuildingType != SDK::EFortBuildingType::Wall) // Centers building pieces if necessary
                    {
                        switch (RotationIterations)
                        {
                        case 1:
                            location = location + ForwardVector * 256.0f + RightVector * 256.0f;
                            break;
                        case 2:
                            location = location + RightVector * 512.0f;
                            break;
                        case 3:
                            location = location + ForwardVector * -256.0f + RightVector * 256.0f;
                        }
                    }

                    rotation.Yaw = yaw + 90 * RotationIterations;

                    auto HealthPercent = BuildingActor->GetHealthPercent();

                    BuildingActor->SilentDie();

                    if (auto NewBuildingActor = (SDK::ABuildingSMActor*)Spawners::SpawnActor(NewBuildingClass, location, rotation, PC))
                    {
                        if (!BuildingActor->bIsInitiallyBuilding)
                            NewBuildingActor->ForceBuildingHealth(NewBuildingActor->GetMaxHealth() * HealthPercent);
                        NewBuildingActor->SetMirrored(Params->bMirrored);
                        NewBuildingActor->InitializeKismetSpawnedBuildingActor(NewBuildingActor, PC);
                        auto PlayerState = (SDK::AFortPlayerStateAthena*)PC->PlayerState;
                        NewBuildingActor->Team = PlayerState->TeamIndex;

                        if (!NewBuildingActor->IsStructurallySupported())
                            NewBuildingActor->K2_DestroyActor();
                    }
                }
            }

            return false;
        })

        DEFINE_PEHOOK("Function FortniteGame.FortPlayerControllerZone.ClientOnPawnDied", { // Spectating hasn't been majorly testing
            auto Params = (SDK::AFortPlayerControllerZone_ClientOnPawnDied_Params*)Parameters;

            auto DeadPC = static_cast<SDK::AFortPlayerControllerAthena*>(Object);
            auto DeadPlayerState = static_cast<SDK::AFortPlayerStateAthena*>(DeadPC->PlayerState);

            auto GameState = reinterpret_cast<SDK::AAthena_GameState_C*>(GetWorld()->GameState);
            auto playerLeftBeforeKill = GameState->PlayersLeft;
            if (Params && DeadPC)
            {
                auto GameMode = static_cast<SDK::AFortGameModeAthena*>(GameState->AuthorityGameMode);
                auto KillerPlayerState = static_cast<SDK::AFortPlayerStateAthena*>(Params->DeathReport.KillerPlayerState);

                Spawners::SpawnActor<SDK::ABP_VictoryDrone_C>(DeadPC->Pawn->K2_GetActorLocation())->PlaySpawnOutAnim();

                SDK::FDeathInfo DeathData;
                DeathData.bDBNO = false;
                DeathData.DeathLocation = DeadPC->Pawn->K2_GetActorLocation();
                DeathData.Distance = Params->DeathReport.KillerPawn ? Params->DeathReport.KillerPawn->GetDistanceTo(DeadPC->Pawn) : 0;

                DeathData.DeathCause = Game::GetDeathCause(Params->DeathReport);
                DeathData.FinisherOrDowner = KillerPlayerState ? KillerPlayerState : DeadPlayerState;

                DeadPlayerState->DeathInfo = DeathData;
                DeadPlayerState->OnRep_DeathInfo();

                if (KillerPlayerState)
                {
                    if (auto Controller = static_cast<SDK::AFortPlayerControllerPvP*>(Params->DeathReport.KillerPawn->Controller))
                    {
                        Controller->ClientReceiveKillNotification(KillerPlayerState, DeadPlayerState);
                    }

                    KillerPlayerState->KillScore++;
                    KillerPlayerState->TeamKillScore++;

                    KillerPlayerState->ClientReportKill(DeadPlayerState);
                    KillerPlayerState->OnRep_Kills();

                    //   Spectate(DeadPC->NetConnection, KillerPlayerState);
                }

                DeadPC->ForceNetUpdate();
                if (IsCurrentlyDisconnecting(DeadPC->NetConnection))
                {
                    LOG_INFO("{} is currently disconnecting", DeadPlayerState->GetPlayerName().ToString());
                }
                
                Game::Mode->OnPlayerKilled(DeadPC);
                
                if (!Game::Mode->isRespawnEnabled() || IsCurrentlyDisconnecting(DeadPC->NetConnection))
                {
                    GameState->PlayersLeft--;
                    GameState->OnRep_PlayersLeft();
                    GameState->PlayerArray.RemoveSingle(DeadPC->NetPlayerIndex);
                }
                
                if (!Game::Mode->isRespawnEnabled() && KillerPlayerState)
                {
                    Spectate(DeadPC->NetConnection, KillerPlayerState);
                }

                if (playerLeftBeforeKill != 1)
                {
                    if (GameState->PlayersLeft == 1 && bStartedBus)
                    {
                        SDK::TArray<SDK::AFortPlayerPawn*> OutActors;
                        GetFortKismet()->STATIC_GetAllFortPlayerPawns(GetWorld(), &OutActors);

                        auto Winner = OutActors[0];
                        auto Controller = static_cast<SDK::AFortPlayerControllerAthena*>(Winner->Controller);

                        if (!Controller->bClientNotifiedOfWin)
                        {
                            GameState->WinningPlayerName = Controller->PlayerState->GetPlayerName();
                            GameState->OnRep_WinningPlayerName();

                            Controller->PlayWinEffects();
                            Controller->ClientNotifyWon();

                            Controller->ClientGameEnded(Winner, true);
                            GameMode->ReadyToEndMatch();
                            GameMode->EndMatch();
                        }
                        OutActors.FreeArray();
                    }

                    if (GameState->PlayersLeft > 1)
                    {
                        SDK::TArray<SDK::AFortPlayerPawn*> OutActors;
                        GetFortKismet()->STATIC_GetAllFortPlayerPawns(GetWorld(), &OutActors);
                        auto RandomTarget = OutActors[rand() % OutActors.Num()];

                        if (!RandomTarget)
                        {
                            LOG_ERROR("Couldn't assign to a spectator a target! Pawn picked was NULL!");
                            return false;
                        }

                        Spectate(DeadPC->NetConnection, static_cast<SDK::AFortPlayerStateAthena*>(RandomTarget->Controller->PlayerState));
                        OutActors.FreeArray();
                    }
                }
            }
            else
            {
                LOG_ERROR("Parameters of ClientOnPawnDied were invalid!");
            }

            return false;
        })

        // Known Problems:
        // Randomly duplicates items, probably picking up twice
        // 
        DEFINE_PEHOOK("Function FortniteGame.FortPlayerPawnAthena.OnCapsuleBeginOverlap", {
            auto Pawn = (SDK::AFortPlayerPawnAthena*)Object;
            auto Params = (SDK::AFortPlayerPawnAthena_OnCapsuleBeginOverlap_Params*)Parameters;
            if (Params->OtherActor->IsA(SDK::AFortPickup::StaticClass()))
            {
                auto Pickup = (SDK::AFortPickup*)Params->OtherActor;

                if (Pickup->bWeaponsCanBeAutoPickups && Inventory::CanPickup((SDK::AFortPlayerControllerAthena*)Pawn->Controller, Pickup))
                {
                    Pawn->ServerHandlePickup(Pickup, 0.40f, SDK::FVector(), true);
                }
            }

            return false;
        })

        DEFINE_PEHOOK("Function FortniteGame.BuildingActor.OnDamageServer", {
            auto Build = (SDK::ABuildingActor*)Object;
            auto Params = (SDK::ABuildingActor_OnDamageServer_Params*)Parameters;
            
            Building::FarmBuild(Build, Params);
            
            return false;
        })

        DEFINE_PEHOOK("Function Car_Copper.Car_Copper_C.OnDamageServer", {
            auto Build = (SDK::ABuildingActor*)Object;
            auto Params = (SDK::ABuildingActor_OnDamageServer_Params*)Parameters;
            
            Building::FarmBuild(Build, Params);
            
            return false;
        })

        DEFINE_PEHOOK("Function FortniteGame.FortPlayerController.ServerEndEditingBuildingActor", {
            auto Params = (SDK::AFortPlayerController_ServerEndEditingBuildingActor_Params*)Parameters;
            auto PC = (SDK::AFortPlayerControllerAthena*)Object;

            if (!PC->IsInAircraft() && Params->BuildingActorToStopEditing)
            {
                Params->BuildingActorToStopEditing->EditingPlayer = nullptr;
                Params->BuildingActorToStopEditing->OnRep_EditingPlayer();

                auto EditTool = (SDK::AFortWeap_EditingTool*)((SDK::APlayerPawn_Athena_C*)PC->Pawn)->CurrentWeapon;

                if (EditTool)
                {
                    EditTool->bEditConfirmed = true;
                    EditTool->EditActor = nullptr;
                    EditTool->OnRep_EditActor();
                }
            }

            return false;
        })

        DEFINE_PEHOOK("Function FortniteGame.FortPlayerController.ServerRepairBuildingActor", {
            auto Params = (SDK::AFortPlayerController_ServerRepairBuildingActor_Params*)Parameters;
            auto Controller = (SDK::AFortPlayerControllerAthena*)Object;
            auto Pawn = (SDK::APlayerPawn_Athena_C*)Controller->Pawn;

            if (Controller && Pawn && Params->BuildingActorToRepair)
            {
                auto HealthPercent = Params->BuildingActorToRepair->GetHealthPercent();
                // 0.75 is from DefaultGameData. This could be gotten at runtime but... it's the same for all materials so it doesn't matter
                auto Cost = floor(((1 - HealthPercent) * 0.75) * 10);
                Params->BuildingActorToRepair->RepairBuilding(Controller, Cost);
                if (!Controller->bInfiniteAmmo)
                {
                    if (!Inventory::TryRemoveItem(Controller, Game::EFortResourceTypeToItemDef(Params->BuildingActorToRepair->ResourceType), Cost))
                    {
                        LOG_ERROR("Failed to remove resource after repair")
                    }
                }
            }

            return false;
        })

        DEFINE_PEHOOK("Function FortniteGame.FortPlayerControllerAthena.ServerAttemptAircraftJump", {
            auto Params = (SDK::AFortPlayerControllerAthena_ServerAttemptAircraftJump_Params*)Parameters;
            auto PC = (SDK::AFortPlayerControllerAthena*)Object;
            auto GameState = (SDK::AAthena_GameState_C*)GetWorld()->AuthorityGameMode->GameState;

            if (PC && Params && !PC->Pawn && PC->IsInAircraft())
            {
                auto Aircraft = (SDK::AFortAthenaAircraft*)GameState->Aircrafts[0];

                if (Aircraft)
                {
                    auto ExitLocation = Aircraft->K2_GetActorLocation();

                    Game::Mode->InitPawn(PC, ExitLocation);

                    PC->ClientSetRotation(Params->ClientRotation, false);

                    ((SDK::AAthena_GameState_C*)GetWorld()->AuthorityGameMode->GameState)->Aircrafts[0]->PlayEffectsForPlayerJumped(); // TODO: fix for gammodes with 2 aircrafts (50v50 v2)
                    PC->ActivateSlot(SDK::EFortQuickBars::Primary, 0, 0, true); // Select the pickaxe

                    Inventory::EmptyInventory(PC);

                    bool bFound = false;
                    auto PickaxeEntry = Inventory::FindItemInInventory<SDK::UFortWeaponMeleeItemDefinition>(PC, bFound);

                    if (bFound)
                        Inventory::EquipInventoryItem(PC, PickaxeEntry.ItemGuid);
                }
            }

            return false;
        })

        DEFINE_PEHOOK("Function FortniteGame.FortPlayerPawn.ServerReviveFromDBNO", {
            auto Params = (SDK::AFortPlayerPawn_ServerReviveFromDBNO_Params*)Parameters;
            auto DBNOPawn = (SDK::APlayerPawn_Athena_C*)Object;
            auto DBNOPC = (SDK::AFortPlayerControllerAthena*)DBNOPawn->Controller;
            auto InstigatorPC = (SDK::AFortPlayerControllerAthena*)Params->EventInstigator;

            if (InstigatorPC && DBNOPawn && DBNOPC)
            {
                DBNOPawn->bIsDBNO = false;
                DBNOPawn->OnRep_IsDBNO();

                DBNOPC->ClientOnPawnRevived(InstigatorPC);
                DBNOPawn->SetHealth(30);
            }

            return false;
        })

        DEFINE_PEHOOK("Function FortniteGame.FortPlayerController.ServerAttemptInteract", {
            auto Params = (SDK::AFortPlayerController_ServerAttemptInteract_Params*)Parameters;
            auto PC = (SDK::AFortPlayerControllerAthena*)Object;

            if (Params->ReceivingActor)
            {
                if (Params->ReceivingActor->IsA(SDK::APlayerPawn_Athena_C::StaticClass()))
                {
                    auto DBNOPawn = (SDK::APlayerPawn_Athena_C*)Params->ReceivingActor;
                    auto DBNOPC = (SDK::AFortPlayerControllerAthena*)DBNOPawn->Controller;

                    if (DBNOPawn && DBNOPC)
                    {
                        DBNOPawn->ReviveFromDBNO(PC);
                    }
                }

                if (Params->ReceivingActor->IsA(SDK::AFortAthenaSupplyDrop::StaticClass()))
                {
                    auto loot = Game::Mode->GetLoot("Loot_AthenaSupplyDrop");
                    auto spawnloc = Params->ReceivingActor->K2_GetActorLocation();
                    for (int i = 0; i < loot.size(); i++)
                    {
                        Spawners::SummonPickupFromChest(loot[i].ItemDef, loot[i].Count, spawnloc);
                    }
                }
                
                if (Params->ReceivingActor->IsA(SDK::ABuildingContainer::StaticClass()))
                {
                    auto Container = (SDK::ABuildingContainer*)Params->ReceivingActor;

                    Container->bAlreadySearched = true;
                    Container->OnRep_bAlreadySearched();

                    LOG_INFO("{}", Container->Class->GetFullName());

                    auto TierGroup = Container->SearchLootTierGroup.ToString();

                    // This can't be the way to do it
                    if (TierGroup == "Loot_Treasure")
                    {
                        TierGroup = "Loot_AthenaTreasure";
                    }
                    else if (TierGroup == "Loot_Ammo")
                    {
                        if(GetKismetMath()->STATIC_RandomBool()) // Maybe there's a better way, but i can't fin it rn
                            TierGroup = "Loot_AthenaAmmoSmall";
                        else
                            TierGroup = "Loot_AthenaAmmoLarge";
                    }
                    auto loot = Game::Mode->GetLoot(TierGroup);
                    for (int i = 0; i < loot.size(); i++)
                    {
                        Spawners::SpawnPickupFromContainer(Container, loot[i].ItemDef, loot[i].Count);
                    }
                }
            }

            return false;
        })

        DEFINE_PEHOOK("Function FortniteGame.FortPlayerController.ServerPlayEmoteItem", {
            if (!Object->IsA(SDK::AFortPlayerControllerAthena::StaticClass()))
                return false;

            auto CurrentPC = (SDK::AFortPlayerControllerAthena*)Object;
            auto CurrentPawn = (SDK::APlayerPawn_Athena_C*)CurrentPC->Pawn;

            auto EmoteParams = (SDK::AFortPlayerController_ServerPlayEmoteItem_Params*)Parameters;
            auto AnimInstance = (SDK::UFortAnimInstance*)CurrentPawn->Mesh->GetAnimInstance();

            if (CurrentPC && !CurrentPC->IsInAircraft() && CurrentPawn && EmoteParams->EmoteAsset && AnimInstance && !AnimInstance->bIsJumping && !AnimInstance->bIsFalling)
            {
                // ((UFortCheatManager*)CurrentPC->CheatManager)->AthenaEmote(EmoteParams->EmoteAsset->Name.ToWString().c_str());
                // CurrentPC->ServerEmote(EmoteParams->EmoteAsset->Name);
                if (EmoteParams->EmoteAsset->IsA(SDK::UAthenaDanceItemDefinition::StaticClass()))
                {
                    if (auto Montage = EmoteParams->EmoteAsset->GetAnimationHardReference(CurrentPawn->CharacterBodyType, CurrentPawn->CharacterGender))
                    {
                        auto& RepAnimMontageInfo = CurrentPawn->RepAnimMontageInfo;
                        auto& ReplayRepAnimMontageInfo = CurrentPawn->ReplayRepAnimMontageInfo;
                        auto& RepCharPartAnimMontageInfo = CurrentPawn->RepCharPartAnimMontageInfo;
                        auto& LocalAnimMontageInfo = CurrentPawn->AbilitySystemComponent->LocalAnimMontageInfo;
                        auto Ability = CurrentPawn->AbilitySystemComponent->AllReplicatedInstancedAbilities[0];

                        const auto Duration = AnimInstance->Montage_Play(Montage, 1.0f, SDK::EMontagePlayReturnType::Duration, 0.0f, true);

                        if (Duration > 0.f)
                        {
                            ReplayRepAnimMontageInfo.AnimMontage = Montage;
                            LocalAnimMontageInfo.AnimMontage = Montage;
                            if (Ability)
                            {
                                LocalAnimMontageInfo.AnimatingAbility = Ability;
                            }
                            LocalAnimMontageInfo.PlayBit = 1;

                            RepAnimMontageInfo.AnimMontage = Montage;
                            RepAnimMontageInfo.ForcePlayBit = 1;

                            RepCharPartAnimMontageInfo.PawnMontage = Montage;

                            if (Ability)
                            {
                                Ability->CurrentMontage = Montage;
                            }

                            bool bIsStopped = AnimInstance->Montage_GetIsStopped(Montage);

                            if (!bIsStopped)
                            {
                                RepAnimMontageInfo.PlayRate = AnimInstance->Montage_GetPlayRate(Montage);
                                RepAnimMontageInfo.Position = AnimInstance->Montage_GetPosition(Montage);
                                RepAnimMontageInfo.BlendTime = AnimInstance->Montage_GetBlendTime(Montage);
                            }

                            RepAnimMontageInfo.IsStopped = bIsStopped;
                            RepAnimMontageInfo.NextSectionID = 0;

                            // CurrentPawn->Mesh->SetAnimation(Montage);
                            CurrentPawn->OnRep_ReplicatedMovement();
                            CurrentPawn->OnRep_CharPartAnimMontageInfo();
                            CurrentPawn->OnRep_ReplicatedAnimMontage();
                            CurrentPawn->OnRep_RepAnimMontageStartSection();
                            CurrentPawn->OnRep_ReplayRepAnimMontageInfo();
                            CurrentPawn->ForceNetUpdate();

                            // Look into ACharacter::FRepRootMotionMontage
                        }
                    }
                }
            }

            return false;
        })

        DEFINE_PEHOOK("Function FortniteGame.FortPlayerController.ServerAttemptInventoryDrop", {
            auto PC = (SDK::AFortPlayerControllerAthena*)Object;

            if (PC && !PC->IsInAircraft())
            {
                auto Params = static_cast<SDK::AFortPlayerController_ServerAttemptInventoryDrop_Params*>(Parameters);
                Inventory::OnDrop(PC, Params->ItemGuid, Params->Count);
            }

            return false;
        })

        DEFINE_PEHOOK("Function BP_VictoryDrone.BP_VictoryDrone_C.OnSpawnOutAnimEnded", {
            if (Object->IsA(SDK::ABP_VictoryDrone_C::StaticClass()))
            {
                auto Drone = (SDK::ABP_VictoryDrone_C*)Object;

                if (Drone)
                {
                    Drone->K2_DestroyActor();
                }
            }

            return false;
        })

        DEFINE_PEHOOK("Function FortniteGame.FortPlayerController.ServerExecuteInventoryItem", {
            Inventory::EquipInventoryItem((SDK::AFortPlayerControllerAthena*)Object, *(SDK::FGuid*)Parameters);

            return false;
        })

        DEFINE_PEHOOK("Function FortniteGame.FortPlayerController.ServerReturnToMainMenu", {
            auto PC = (SDK::AFortPlayerController*)Object;
            PC->bIsDisconnecting = true;
            PC->ClientTravel(L"Frontend", SDK::ETravelType::TRAVEL_Absolute, false, SDK::FGuid());

            return false;
        })

        DEFINE_PEHOOK("Function FortniteGame.FortPlayerController.ServerLoadingScreenDropped", {
            auto Pawn = (SDK::APlayerPawn_Athena_C*)((SDK::AFortPlayerController*)Object)->Pawn;
            if (Pawn && Pawn->AbilitySystemComponent)
            {
                Abilities::ApplyAbilities(Pawn);
            }

            return false;
        })

        DEFINE_PEHOOK("Function FortniteGame.FortPlayerPawn.ServerChoosePart", {
            return true;
        })

        DEFINE_PEHOOK("Function Engine.GameMode.ReadyToStartMatch", {
            if (!bListening)
            {
                Game::OnReadyToStartMatch();

                HostBeacon = Spawners::SpawnActor<SDK::AFortOnlineBeaconHost>();
                HostBeacon->ListenPort = 7776;
                auto bInitBeacon = Native::OnlineBeaconHost::InitHost(HostBeacon);
                CheckNullFatal(bInitBeacon, "Failed to initialize the Beacon!");

                HostBeacon->NetDriverName = SDK::FName(282); // REGISTER_NAME(282,GameNetDriver)
                HostBeacon->NetDriver->NetDriverName = SDK::FName(282); // REGISTER_NAME(282,GameNetDriver)
                HostBeacon->NetDriver->World = GetWorld();
                SDK::FString Error;
                auto InURL = SDK::FURL();
                InURL.Port = 7777;

                Native::NetDriver::InitListen(HostBeacon->NetDriver, GetWorld(), InURL, true, Error);

                Native::ReplicationDriver::ServerReplicateActors = decltype(Native::ReplicationDriver::ServerReplicateActors)(HostBeacon->NetDriver->ReplicationDriver->Vtable[0x53]);

                auto ClassRepNodePolicies = GetClassRepNodePolicies(HostBeacon->NetDriver->ReplicationDriver);

                for (auto&& Pair : ClassRepNodePolicies)
                {
                    auto key = Pair.Key().ResolveObjectPtr();
                    auto& value = Pair.Value();

                    LOG_INFO("ClassRepNodePolicies: {} - {}", key->GetName(), ClassRepNodeMappingToString(value));

                    if (key == SDK::AFortInventory::StaticClass())
                    {
                        value = SDK::EClassRepNodeMapping::RelevantAllConnections;
                        LOG_INFO("Found ClassRepNodePolicy for AFortInventory! {}", (int)value);
                    }

                    if (key == SDK::AFortQuickBars::StaticClass())
                    {
                        value = SDK::EClassRepNodeMapping::RelevantAllConnections;
                        LOG_INFO("Found ClassRepNodePolicy for AFortQuickBars! {}", (int)value);
                    }

                    //if (key == AFortPickup::StaticClass())
                    //{
                    //    value = EClassRepNodeMapping::Spatialize_Dormancy;
                    //    LOG_INFO("Found ClassRepNodePolicy for AFortPickup! {}", (int)value);
                    //}
                }

                GetWorld()->NetDriver = HostBeacon->NetDriver;
                GetWorld()->LevelCollections[0].NetDriver = HostBeacon->NetDriver;
                GetWorld()->LevelCollections[1].NetDriver = HostBeacon->NetDriver;

                GetWorld()->AuthorityGameMode->GameSession->MaxPlayers = MAXPLAYERS;

                Native::OnlineBeacon::PauseBeaconRequests(HostBeacon, false);
                bListening = true;
                LOG_INFO("Listening for connections on port {}!", HostBeacon->ListenPort);
            }

            return false;
        })

        DEFINE_PEHOOK("Function FortniteGame.FortGameModeAthena.OnAircraftExitedDropZone", {
            if (GetWorld() && GetWorld()->NetDriver && GetWorld()->NetDriver->ClientConnections.Data)
            {
                auto Connections = HostBeacon->NetDriver->ClientConnections;

                for (int i = 0; i < Connections.Num(); i++)
                {
                    auto Controller = (SDK::AFortPlayerControllerAthena*)Connections[i]->PlayerController;

                    if (!Controller || !Controller->IsA(SDK::AFortPlayerControllerAthena::StaticClass()) || Controller->PlayerState->bIsSpectator)
                        continue;

                    if (Controller && Controller->IsInAircraft())
                        Controller->ServerAttemptAircraftJump(SDK::FRotator());
                }
            }

            return false;
        })

        DEFINE_PEHOOK("Function FortniteGame.FortPlayerController.ServerCheatAll", {
            // auto PlayerController = (AFortPlayerControllerAthena*)Object;

            // if (PlayerController)
            //     KickController((AFortPlayerControllerAthena*)Object, L"Please do not do that!");

            return true;
        })
        
        DEFINE_PEHOOK("Function Engine.GameMode.Logout", {
            auto PC = (SDK::AFortPlayerController*)Object;
            if (PC) PC->bIsDisconnecting = true;
            
            return false;
        })

        LOG_INFO("Hooked {} UFunction(s)!", toHook.size());
    }
}
