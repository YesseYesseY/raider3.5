#pragma once
#pragma warning(disable : 4099)

#include <functional>

#include "Logic/Game.h"
#include "Logic/Spawners.h"
#include "Logic/Abilities.h"
#include "UE4.h"

// #define LOGGING
//#define CHEATS
#define MAXPLAYERS 100

// Define the hook with ufunction full name
// Return true in the lambda to prevent the original function call

namespace UFunctionHooks
{
    inline std::vector<UFunction*> toHook;
    inline std::vector<std::function<bool(UObject*, void*)>> toCall;

#define DEFINE_PEHOOK(ufunctionName, func)                           \
    toHook.push_back(UObject::FindObject<UFunction>(ufunctionName)); \
    toCall.push_back([](UObject * Object, void* Parameters) -> bool func);

    auto Initialize()
    {
        DEFINE_PEHOOK("Function GameplayAbilities.AbilitySystemComponent.ServerTryActivateAbility", {
            auto AbilitySystemComponent = (UAbilitySystemComponent*)Object;
            auto Params = (UAbilitySystemComponent_ServerTryActivateAbility_Params*)Parameters;

            Abilities::TryActivateAbility(AbilitySystemComponent, Params->AbilityToActivate, Params->InputPressed, &Params->PredictionKey, nullptr);

            return false;
        })

        DEFINE_PEHOOK("Function GameplayAbilities.AbilitySystemComponent.ServerTryActivateAbilityWithEventData", {
            auto AbilitySystemComponent = (UAbilitySystemComponent*)Object;
            auto Params = (UAbilitySystemComponent_ServerTryActivateAbilityWithEventData_Params*)Parameters;

            Abilities::TryActivateAbility(AbilitySystemComponent, Params->AbilityToActivate, Params->InputPressed, &Params->PredictionKey, &Params->TriggerEventData);

            return false;
        })

        DEFINE_PEHOOK("Function GameplayAbilities.AbilitySystemComponent.ServerAbilityRPCBatch", {
            auto AbilitySystemComponent = (UAbilitySystemComponent*)Object;
            auto Params = (UAbilitySystemComponent_ServerAbilityRPCBatch_Params*)Parameters;

            Abilities::TryActivateAbility(AbilitySystemComponent, Params->BatchInfo.AbilitySpecHandle, Params->BatchInfo.InputPressed, &Params->BatchInfo.PredictionKey, nullptr);

            return false;
        })

        DEFINE_PEHOOK("Function FortniteGame.FortPlayerPawn.ServerHandlePickup", {
            Inventory::OnPickup((AFortPlayerControllerAthena*)((APawn*)Object)->Controller, Parameters);
            return false;
        })

#ifdef CHEATS
        DEFINE_PEHOOK("Function FortniteGame.FortPlayerController.ServerCheat", {
            if (Object->IsA(AFortPlayerControllerAthena::StaticClass()))
            {
                auto PC = (AFortPlayerControllerAthena*)Object;
                auto Params = (AFortPlayerController_ServerCheat_Params*)Parameters;

                if (Params && PC && !PC->IsInAircraft())
                {
                    auto Pawn = (APlayerPawn_Athena_C*)PC->Pawn;
                    auto Message = Params->Msg.ToString() + ' ';

                    std::vector<std::string> Arguments;

                    while (Message.find(" ") != -1)
                    {
                        Arguments.push_back(Message.substr(0, Message.find(' ')));
                        Message.erase(0, Message.find(' ') + 1);
                    }

                    auto NumArgs = Arguments.size() - 1;

                    if (NumArgs >= 0)
                    {
                        auto& Command = Arguments[0];
                        std::transform(Command.begin(), Command.end(), Command.begin(), ::tolower);

                        if (Command == "start")
                        {
                            PC->ClientMessage(L"starting Aircraft!", FName(), 0);
                            GetKismetSystem()->STATIC_ExecuteConsoleCommand(GetWorld(), L"startaircraft", nullptr);
                        }

                        else
                            PC->ClientMessage(L"Unable to handle command!", FName(), 0);
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
            auto Actor = (ABuildingActor*)Object;

            if (Actor)
            {
                if (Actor->IsA(ABuildingSMActor::StaticClass()))
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
            auto PC = (AFortPlayerControllerAthena*)Object;

            auto Params = (AFortPlayerController_ServerCreateBuildingActor_Params*)Parameters;
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

                auto BuildingActor = (ABuildingSMActor*)Spawners::SpawnActor(CurrentBuildClass, Params->BuildLoc, Params->BuildRot, PC, ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn);
                if (BuildingActor && CanBuild(BuildingActor))
                {
                    BuildingActor->DynamicBuildingPlacementType = EDynamicBuildingPlacementType::DestroyAnythingThatCollides;
                    BuildingActor->SetMirrored(Params->bMirrored);
                    // BuildingActor->PlacedByPlacementTool();
                    BuildingActor->InitializeKismetSpawnedBuildingActor(BuildingActor, PC);
                    auto PlayerState = (AFortPlayerStateAthena*)PC->PlayerState;
                    BuildingActor->Team = PlayerState->TeamIndex;

                    UFortResourceItemDefinition* matdef = Game::EFortResourceTypeToItemDef(BuildingActor->ResourceType);
                    
                    if (!Inventory::TryRemoveItem(PC, matdef, 10))
                    {
                        LOG_ERROR("Failed to remove resource from building")
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
            auto Params = (AFortPlayerController_ServerBeginEditingBuildingActor_Params*)Parameters;
            auto Controller = (AFortPlayerControllerAthena*)Object;
            auto Pawn = (APlayerPawn_Athena_C*)Controller->Pawn;
            bool bFound = false;
            auto EditToolEntry = Inventory::FindItemInInventory<UFortEditToolItemDefinition>(Controller, bFound);

            if (Controller && Pawn && Params->BuildingActorToEdit && bFound)
            {
                auto EditTool = (AFortWeap_EditingTool*)Inventory::EquipWeaponDefinition(Pawn, (UFortWeaponItemDefinition*)EditToolEntry.ItemDefinition, EditToolEntry.ItemGuid);

                if (EditTool)
                {
                    EditTool->EditActor = Params->BuildingActorToEdit;
                    EditTool->OnRep_EditActor();
                    Params->BuildingActorToEdit->EditingPlayer = (AFortPlayerStateZone*)Pawn->PlayerState;
                    Params->BuildingActorToEdit->OnRep_EditingPlayer();
                }
            }

            return false;
        })

        DEFINE_PEHOOK("Function FortniteGame.FortDecoTool.ServerSpawnDeco", {
            auto Tool = (AFortDecoTool*)Object;

            Spawners::SpawnDeco(Tool, Parameters);

            // TODO: Remove trap after use

            return false;
        })

        DEFINE_PEHOOK("Function FortniteGame.FortPlayerController.ServerEditBuildingActor", {
            auto Params = (AFortPlayerController_ServerEditBuildingActor_Params*)Parameters;
            auto PC = (AFortPlayerControllerAthena*)Object;

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

                    if (BuildingActor->BuildingType != EFortBuildingType::Wall) // Centers building pieces if necessary
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

                    if (auto NewBuildingActor = (ABuildingSMActor*)Spawners::SpawnActor(NewBuildingClass, location, rotation, PC))
                    {
                        if (!BuildingActor->bIsInitiallyBuilding)
                            NewBuildingActor->ForceBuildingHealth(NewBuildingActor->GetMaxHealth() * HealthPercent);
                        NewBuildingActor->SetMirrored(Params->bMirrored);
                        NewBuildingActor->InitializeKismetSpawnedBuildingActor(NewBuildingActor, PC);
                        auto PlayerState = (AFortPlayerStateAthena*)PC->PlayerState;
                        NewBuildingActor->Team = PlayerState->TeamIndex;

                        if (!NewBuildingActor->IsStructurallySupported())
                            NewBuildingActor->K2_DestroyActor();
                    }
                }
            }

            return false;
        })

        DEFINE_PEHOOK("Function FortniteGame.FortPlayerControllerZone.ClientOnPawnDied", { // Spectating hasn't been majorly testing
            auto Params = (AFortPlayerControllerZone_ClientOnPawnDied_Params*)Parameters;

            auto DeadPC = static_cast<AFortPlayerControllerAthena*>(Object);
            auto DeadPlayerState = static_cast<AFortPlayerStateAthena*>(DeadPC->PlayerState);

            auto GameState = reinterpret_cast<AAthena_GameState_C*>(GetWorld()->GameState);
            auto playerLeftBeforeKill = GameState->PlayersLeft;
            if (Params && DeadPC)
            {
                auto GameMode = static_cast<AFortGameModeAthena*>(GameState->AuthorityGameMode);
                auto KillerPlayerState = static_cast<AFortPlayerStateAthena*>(Params->DeathReport.KillerPlayerState);

                Spawners::SpawnActor<ABP_VictoryDrone_C>(DeadPC->Pawn->K2_GetActorLocation())->PlaySpawnOutAnim();

                FDeathInfo DeathData;
                DeathData.bDBNO = false;
                DeathData.DeathLocation = DeadPC->Pawn->K2_GetActorLocation();
                DeathData.Distance = Params->DeathReport.KillerPawn ? Params->DeathReport.KillerPawn->GetDistanceTo(DeadPC->Pawn) : 0;

                DeathData.DeathCause = Game::GetDeathCause(Params->DeathReport);
                DeathData.FinisherOrDowner = KillerPlayerState ? KillerPlayerState : DeadPlayerState;

                DeadPlayerState->DeathInfo = DeathData;
                DeadPlayerState->OnRep_DeathInfo();

                if (KillerPlayerState)
                {
                    if (auto Controller = static_cast<AFortPlayerControllerPvP*>(Params->DeathReport.KillerPawn->Controller))
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
                        TArray<AFortPlayerPawn*> OutActors;
                        GetFortKismet()->STATIC_GetAllFortPlayerPawns(GetWorld(), &OutActors);

                        auto Winner = OutActors[0];
                        auto Controller = static_cast<AFortPlayerControllerAthena*>(Winner->Controller);

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
                        TArray<AFortPlayerPawn*> OutActors;
                        GetFortKismet()->STATIC_GetAllFortPlayerPawns(GetWorld(), &OutActors);
                        auto RandomTarget = OutActors[rand() % OutActors.Num()];

                        if (!RandomTarget)
                        {
                            LOG_ERROR("Couldn't assign to a spectator a target! Pawn picked was NULL!");
                            return false;
                        }

                        Spectate(DeadPC->NetConnection, static_cast<AFortPlayerStateAthena*>(RandomTarget->Controller->PlayerState));
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

        // TODO: Make this thing not suck
        DEFINE_PEHOOK("Function FortniteGame.FortPlayerPawnAthena.OnCapsuleBeginOverlap", {
            auto Pawn = (AFortPlayerPawnAthena*)Object;
            auto Params = (AFortPlayerPawnAthena_OnCapsuleBeginOverlap_Params*)Parameters;
            if (Params->OtherActor->IsA(AFortPickup::StaticClass()))
            {
                auto Pickup = (AFortPickup*)Params->OtherActor;

                if (Pickup->bWeaponsCanBeAutoPickups)
                {
                    Pawn->ServerHandlePickup(Pickup, 0.40f, FVector(), true);
                }
            }

            return false;
        })

        DEFINE_PEHOOK("Function FortniteGame.BuildingActor.OnDamageServer", {
            if (!Game::Mode->ResourceRates)
                return false;
            auto Build = (ABuildingActor*)Object;
            auto Params = (ABuildingActor_OnDamageServer_Params*)Parameters;
            auto Controller = (AFortPlayerControllerAthena*)Params->InstigatedBy;
            auto Pawn = (AFortPlayerPawnAthena*)Controller->Pawn;
            if (!Pawn->CurrentWeapon->WeaponData->IsA(UFortWeaponMeleeItemDefinition::StaticClass()))
                return false;
                
            if (Build->IsA(ABuildingSMActor::StaticClass()))
            {
                auto BuildSM = (ABuildingSMActor*)Object;
                auto MatDef = Game::EFortResourceTypeToItemDef(BuildSM->ResourceType.GetValue());
                if (!MatDef)
                    return false;

                TEnumAsByte<EEvaluateCurveTableResult> EvalResult;
                float EvalOut;
                GetDataTableFunctionLibrary()->STATIC_EvaluateCurveTableRow(BuildSM->BuildingResourceAmountOverride.CurveTable, BuildSM->BuildingResourceAmountOverride.RowName, 0, L"Get resource rate", &EvalResult, &EvalOut);
                
                // this isn't exactly like how it is supposed to be but it isn't bad either
                auto MatCount = floor(EvalOut / (BuildSM->GetMaxHealth() / Params->Damage));
                if (MatCount <= 0)
                    return false;
                auto Pickup = Spawners::SummonPickup(Pawn, MatDef, MatCount, Pawn->K2_GetActorLocation());
                if (Pickup)
                {
                    Pawn->ServerHandlePickup(Pickup, 1.0f, FVector(), true);
                    Controller->ClientReportDamagedResourceBuilding(BuildSM, BuildSM->ResourceType, MatCount, BuildSM->GetHealth() <= 0, Params->Damage > 50);
                }
            }
            
            return false;
        })

        DEFINE_PEHOOK("Function FortniteGame.FortPlayerController.ServerEndEditingBuildingActor", {
            auto Params = (AFortPlayerController_ServerEndEditingBuildingActor_Params*)Parameters;
            auto PC = (AFortPlayerControllerAthena*)Object;

            if (!PC->IsInAircraft() && Params->BuildingActorToStopEditing)
            {
                Params->BuildingActorToStopEditing->EditingPlayer = nullptr;
                Params->BuildingActorToStopEditing->OnRep_EditingPlayer();

                auto EditTool = (AFortWeap_EditingTool*)((APlayerPawn_Athena_C*)PC->Pawn)->CurrentWeapon;

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
            auto Params = (AFortPlayerController_ServerRepairBuildingActor_Params*)Parameters;
            auto Controller = (AFortPlayerControllerAthena*)Object;
            auto Pawn = (APlayerPawn_Athena_C*)Controller->Pawn;

            if (Controller && Pawn && Params->BuildingActorToRepair)
            {
                auto HealthPercent = Params->BuildingActorToRepair->GetHealthPercent();
                // 0.75 is from DefaultGameData. This could be gotten at runtime but... it's the same for all materials so it doesn't matter
                auto Cost = floor(((1 - HealthPercent) * 0.75) * 10);
                Params->BuildingActorToRepair->RepairBuilding(Controller, Cost);
                if (!Inventory::TryRemoveItem(Controller, Game::EFortResourceTypeToItemDef(Params->BuildingActorToRepair->ResourceType), Cost))
                {
                    LOG_ERROR("Failed to remove resource after repair")
                }
            }

            return false;
        })

        DEFINE_PEHOOK("Function FortniteGame.FortPlayerControllerAthena.ServerAttemptAircraftJump", {
            auto Params = (AFortPlayerControllerAthena_ServerAttemptAircraftJump_Params*)Parameters;
            auto PC = (AFortPlayerControllerAthena*)Object;
            auto GameState = (AAthena_GameState_C*)GetWorld()->AuthorityGameMode->GameState;

            if (PC && Params && !PC->Pawn && PC->IsInAircraft())
            {
                auto Aircraft = (AFortAthenaAircraft*)GameState->Aircrafts[0];

                if (Aircraft)
                {
                    auto ExitLocation = Aircraft->K2_GetActorLocation();

                    Game::Mode->InitPawn(PC, ExitLocation);

                    PC->ClientSetRotation(Params->ClientRotation, false);

                    ((AAthena_GameState_C*)GetWorld()->AuthorityGameMode->GameState)->Aircrafts[0]->PlayEffectsForPlayerJumped(); // TODO: fix for gammodes with 2 aircrafts (50v50 v2)
                    PC->ActivateSlot(EFortQuickBars::Primary, 0, 0, true); // Select the pickaxe

                    bool bFound = false;
                    auto PickaxeEntry = Inventory::FindItemInInventory<UFortWeaponMeleeItemDefinition>(PC, bFound);

                    if (bFound)
                        Inventory::EquipInventoryItem(PC, PickaxeEntry.ItemGuid);
                }
            }

            return false;
        })

        DEFINE_PEHOOK("Function FortniteGame.FortPlayerPawn.ServerReviveFromDBNO", {
            auto Params = (AFortPlayerPawn_ServerReviveFromDBNO_Params*)Parameters;
            auto DBNOPawn = (APlayerPawn_Athena_C*)Object;
            auto DBNOPC = (AFortPlayerControllerAthena*)DBNOPawn->Controller;
            auto InstigatorPC = (AFortPlayerControllerAthena*)Params->EventInstigator;

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
            auto Params = (AFortPlayerController_ServerAttemptInteract_Params*)Parameters;
            auto PC = (AFortPlayerControllerAthena*)Object;

            if (Params->ReceivingActor)
            {
                if (Params->ReceivingActor->IsA(APlayerPawn_Athena_C::StaticClass()))
                {
                    auto DBNOPawn = (APlayerPawn_Athena_C*)Params->ReceivingActor;
                    auto DBNOPC = (AFortPlayerControllerAthena*)DBNOPawn->Controller;

                    if (DBNOPawn && DBNOPC)
                    {
                        DBNOPawn->ReviveFromDBNO(PC);
                    }
                }

                if (Params->ReceivingActor->IsA(ABuildingContainer::StaticClass()))
                {
                    auto Container = (ABuildingContainer*)Params->ReceivingActor;

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
                    auto spawnloc = Container->K2_GetActorLocation() + (Container->GetActorRightVector() * 42.0f);
                    for (int i = 0; i < loot.size(); i++)
                    {
                        Spawners::SummonPickupFromChest(loot[i].ItemDef, loot[i].Count, spawnloc);
                    }
                }
            }

            return false;
        })

        DEFINE_PEHOOK("Function FortniteGame.FortPlayerController.ServerPlayEmoteItem", {
            if (!Object->IsA(AFortPlayerControllerAthena::StaticClass()))
                return false;

            auto CurrentPC = (AFortPlayerControllerAthena*)Object;
            auto CurrentPawn = (APlayerPawn_Athena_C*)CurrentPC->Pawn;

            auto EmoteParams = (AFortPlayerController_ServerPlayEmoteItem_Params*)Parameters;
            auto AnimInstance = (UFortAnimInstance*)CurrentPawn->Mesh->GetAnimInstance();

            if (CurrentPC && !CurrentPC->IsInAircraft() && CurrentPawn && EmoteParams->EmoteAsset && AnimInstance && !AnimInstance->bIsJumping && !AnimInstance->bIsFalling)
            {
                // ((UFortCheatManager*)CurrentPC->CheatManager)->AthenaEmote(EmoteParams->EmoteAsset->Name.ToWString().c_str());
                // CurrentPC->ServerEmote(EmoteParams->EmoteAsset->Name);
                if (EmoteParams->EmoteAsset->IsA(UAthenaDanceItemDefinition::StaticClass()))
                {
                    if (auto Montage = EmoteParams->EmoteAsset->GetAnimationHardReference(CurrentPawn->CharacterBodyType, CurrentPawn->CharacterGender))
                    {
                        auto& RepAnimMontageInfo = CurrentPawn->RepAnimMontageInfo;
                        auto& ReplayRepAnimMontageInfo = CurrentPawn->ReplayRepAnimMontageInfo;
                        auto& RepCharPartAnimMontageInfo = CurrentPawn->RepCharPartAnimMontageInfo;
                        auto& LocalAnimMontageInfo = CurrentPawn->AbilitySystemComponent->LocalAnimMontageInfo;
                        auto Ability = CurrentPawn->AbilitySystemComponent->AllReplicatedInstancedAbilities[0];

                        const auto Duration = AnimInstance->Montage_Play(Montage, 1.0f, EMontagePlayReturnType::Duration, 0.0f, true);

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
            auto PC = (AFortPlayerControllerAthena*)Object;

            if (PC && !PC->IsInAircraft())
                Inventory::OnDrop(PC, Parameters);

            return false;
        })

        DEFINE_PEHOOK("Function BP_VictoryDrone.BP_VictoryDrone_C.OnSpawnOutAnimEnded", {
            if (Object->IsA(ABP_VictoryDrone_C::StaticClass()))
            {
                auto Drone = (ABP_VictoryDrone_C*)Object;

                if (Drone)
                {
                    Drone->K2_DestroyActor();
                }
            }

            return false;
        })

        DEFINE_PEHOOK("Function FortniteGame.FortPlayerController.ServerExecuteInventoryItem", {
            Inventory::EquipInventoryItem((AFortPlayerControllerAthena*)Object, *(FGuid*)Parameters);

            return false;
        })

        DEFINE_PEHOOK("Function FortniteGame.FortPlayerController.ServerReturnToMainMenu", {
            auto PC = (AFortPlayerController*)Object;
            PC->bIsDisconnecting = true;
            PC->ClientTravel(L"Frontend", ETravelType::TRAVEL_Absolute, false, FGuid());

            return false;
        })

        DEFINE_PEHOOK("Function FortniteGame.FortPlayerController.ServerLoadingScreenDropped", {
            auto Pawn = (APlayerPawn_Athena_C*)((AFortPlayerController*)Object)->Pawn;
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

                HostBeacon = Spawners::SpawnActor<AFortOnlineBeaconHost>();
                HostBeacon->ListenPort = 7776;
                auto bInitBeacon = Native::OnlineBeaconHost::InitHost(HostBeacon);
                CheckNullFatal(bInitBeacon, "Failed to initialize the Beacon!");

                HostBeacon->NetDriverName = FName(282); // REGISTER_NAME(282,GameNetDriver)
                HostBeacon->NetDriver->NetDriverName = FName(282); // REGISTER_NAME(282,GameNetDriver)
                HostBeacon->NetDriver->World = GetWorld();
                FString Error;
                auto InURL = FURL();
                InURL.Port = 7777;

                Native::NetDriver::InitListen(HostBeacon->NetDriver, GetWorld(), InURL, true, Error);

                Native::ReplicationDriver::ServerReplicateActors = decltype(Native::ReplicationDriver::ServerReplicateActors)(HostBeacon->NetDriver->ReplicationDriver->Vtable[0x53]);

                auto ClassRepNodePolicies = GetClassRepNodePolicies(HostBeacon->NetDriver->ReplicationDriver);

                for (auto&& Pair : ClassRepNodePolicies)
                {
                    auto key = Pair.Key().ResolveObjectPtr();
                    auto& value = Pair.Value();

                    LOG_INFO("ClassRepNodePolicies: {} - {}", key->GetName(), ClassRepNodeMappingToString(value));

                    if (key == AFortInventory::StaticClass())
                    {
                        value = EClassRepNodeMapping::RelevantAllConnections;
                        LOG_INFO("Found ClassRepNodePolicy for AFortInventory! {}", (int)value);
                    }

                    if (key == AFortQuickBars::StaticClass())
                    {
                        value = EClassRepNodeMapping::RelevantAllConnections;
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
                    auto Controller = (AFortPlayerControllerAthena*)Connections[i]->PlayerController;

                    if (!Controller || !Controller->IsA(AFortPlayerControllerAthena::StaticClass()) || Controller->PlayerState->bIsSpectator)
                        continue;

                    if (Controller && Controller->IsInAircraft())
                        Controller->ServerAttemptAircraftJump(FRotator());
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
            auto PC = (AFortPlayerController*)Object;
            if (PC) PC->bIsDisconnecting = true;
            
            return false;
        })

        LOG_INFO("Hooked {} UFunction(s)!", toHook.size());
    }
}
