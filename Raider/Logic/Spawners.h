#pragma once
#include "../UE4.h"
#include "Abilities.h"

namespace Spawners
{
    static AActor* SpawnActor(UClass* StaticClass, FTransform SpawnTransform, AActor* Owner = nullptr, ESpawnActorCollisionHandlingMethod Flags = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn)
    {
        AActor* FirstActor = GetGameplayStatics()->STATIC_BeginDeferredActorSpawnFromClass(GetWorld(), StaticClass, SpawnTransform, Flags, Owner);

        if (FirstActor)
        {
            AActor* FinalActor = GetGameplayStatics()->STATIC_FinishSpawningActor(FirstActor, SpawnTransform);

            if (FinalActor)
            {
                return FinalActor;
            }
        }

        return nullptr;
    }

    static AActor* SpawnActor(UClass* ActorClass, FVector Location = { 0.0f, 0.0f, 0.0f }, FRotator Rotation = { 0, 0, 0 }, AActor* Owner = nullptr, ESpawnActorCollisionHandlingMethod Flags = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn)
    {
        FTransform SpawnTransform;

        SpawnTransform.Translation = Location;
        SpawnTransform.Scale3D = FVector { 1, 1, 1 };
        SpawnTransform.Rotation = Utils::RotToQuat(Rotation);

        return SpawnActor(ActorClass, SpawnTransform, Owner, Flags);
    }

    template <typename RetActorType = AActor>
    static RetActorType* SpawnActor(FVector Location = { 0.0f, 0.0f, 0.0f }, AActor* Owner = nullptr, FQuat Rotation = { 0, 0, 0 }, ESpawnActorCollisionHandlingMethod Flags = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn, bool attempts = 10)
    {
        FTransform SpawnTransform;

        SpawnTransform.Translation = Location;
        SpawnTransform.Scale3D = FVector { 1, 1, 1 };
        SpawnTransform.Rotation = Rotation;
        
        for (int i = 0; i < attempts; i++)
        {
            auto ret = static_cast<RetActorType*>(SpawnActor(RetActorType::StaticClass(), SpawnTransform, Owner, Flags));
            if (ret)
                return ret;
        }
        return nullptr;
    }

    static ABuildingSMActor* SpawnBuilding(UClass* BGAClass, FVector& Location, FRotator& Rotation, APlayerPawn_Athena_C* Pawn)
    {
        FTransform Transform;
        Transform.Translation = Location;
        Transform.Scale3D = FVector { 1, 1, 1 };
        Transform.Rotation = Utils::RotToQuat(Rotation);

        return (ABuildingSMActor*)GetFortKismet()->STATIC_SpawnBuildingGameplayActor(BGAClass, Transform, Pawn);
    }

    static AFortPickup* SummonPickup(AFortPlayerPawn* Pawn, auto ItemDef, int Count, FVector Location)
    {
        auto FortPickup = SpawnActor<AFortPickup>(Location, Pawn);

        FortPickup->bReplicates = true; // should be autmoatic but eh

        FortPickup->PrimaryPickupItemEntry.Count = Count;
        FortPickup->PrimaryPickupItemEntry.ItemDefinition = ItemDef;

        FortPickup->OnRep_PrimaryPickupItemEntry();
        FortPickup->TossPickup(Location, Pawn, 6, true);

        return FortPickup;
    }

    static void SummonPickupFromChest(auto ItemDef, int Count, FVector Location)
    {
        int attempts = 0;
        for (int i = 0; i < 5; i++)
        {
            auto FortPickup = SpawnActor<AFortPickup>(Location);
            if (FortPickup)
            {
                FortPickup->bReplicates = true; // should be autmoatic but eh
                FortPickup->bRandomRotation = true;

                FortPickup->PrimaryPickupItemEntry.Count = Count;
                FortPickup->PrimaryPickupItemEntry.ItemDefinition = ItemDef;
                if (ItemDef->IsA(UFortWeaponRangedItemDefinition::StaticClass()))
                {
                    auto weapdef = (UFortWeaponItemDefinition*)ItemDef;
                    FortPickup->PrimaryPickupItemEntry.LoadedAmmo = ((FFortBaseWeaponStats*)weapdef->WeaponStatHandle.DataTable->RowMap.GetByKey(weapdef->WeaponStatHandle.RowName))->ClipSize;
                }

                FortPickup->OnRep_PrimaryPickupItemEntry();
                FortPickup->OnRep_TossedFromContainer();
                FortPickup->TossPickup(Location, nullptr, 6, true);
                break;
            }
        }
    }

    static void SpawnPickupFromFloorLoot(auto ItemDef, int Count, FVector Location)
    {
        int attempts = 0;
        for (int i = 0; i < 5; i++)
        {
            auto FortPickup = SpawnActor<AFortPickup>(Location);

            if (FortPickup)
            {
                FortPickup->bReplicates = true; // should be autmoatic but eh

                FortPickup->PrimaryPickupItemEntry.Count = Count;
                FortPickup->PrimaryPickupItemEntry.ItemDefinition = ItemDef;
                if (ItemDef->IsA(UFortWeaponRangedItemDefinition::StaticClass()))
                {
                    auto weapdef = (UFortWeaponItemDefinition*)ItemDef;
                    FortPickup->PrimaryPickupItemEntry.LoadedAmmo = ((FFortBaseWeaponStats*)weapdef->WeaponStatHandle.DataTable->RowMap.GetByKey(weapdef->WeaponStatHandle.RowName))->ClipSize;
                }

                FortPickup->OnRep_PrimaryPickupItemEntry();
                FortPickup->TossPickup(Location, nullptr, 6, false);
                break;
            }
        }
    }

    static void SpawnDeco(AFortDecoTool* Tool, void* _Params)
    {
        if (!_Params)
            return;

        auto Params = static_cast<AFortDecoTool_ServerSpawnDeco_Params*>(_Params);

        FTransform Transform {};
        Transform.Scale3D = FVector(1, 1, 1);
        Transform.Rotation = Utils::RotToQuat(Params->Rotation);
        Transform.Translation = Params->Location;

        UFortTrapItemDefinition* TrapDef;
        // This is not correct but it works i guess
        if (Tool->IsA(AFortDecoTool_ContextTrap::StaticClass()))
        {
            auto buildpos = Params->AttachedActor->K2_GetActorLocation();
            if (Params->AttachedActor->BuildingAttachmentSlot == EBuildingAttachmentSlot::SLOT_Wall)
                TrapDef = static_cast<AFortDecoTool_ContextTrap*>(Tool)->ContextTrapItemDefinition->WallTrap;
            else if (buildpos.Z > Params->Location.Z)
                TrapDef = static_cast<AFortDecoTool_ContextTrap*>(Tool)->ContextTrapItemDefinition->CeilingTrap;
            else
                TrapDef = static_cast<AFortDecoTool_ContextTrap*>(Tool)->ContextTrapItemDefinition->FloorTrap;
        }
        else
        {
            TrapDef = static_cast<UFortTrapItemDefinition*>(Tool->ItemDefinition);
        }

        if (TrapDef)
        {
            auto Trap = static_cast<ABuildingTrap*>(SpawnActor(TrapDef->GetBlueprintClass(), Transform));

            if (Trap)
            {
                Trap->TrapData = static_cast<UFortTrapItemDefinition*>(Tool->ItemDefinition);

                auto Pawn = static_cast<APlayerPawn_Athena_C*>(Tool->Owner);

                Trap->InitializeKismetSpawnedBuildingActor(Trap, static_cast<AFortPlayerController*>(Pawn->Controller));

                Trap->AttachedTo = Params->AttachedActor;
                Trap->OnRep_AttachedTo();

                auto PlayerState = (AFortPlayerStateAthena*)Pawn->Controller->PlayerState;
                Trap->Team = PlayerState->TeamIndex;

                auto TrapAbilitySet = Trap->AbilitySet;

                if (TrapAbilitySet)
                {
                    for (int i = 0; i < TrapAbilitySet->GameplayAbilities.Num(); i++) // this fixes traps crashing the game // don't ask how
                    {
                        auto Ability = TrapAbilitySet->GameplayAbilities[i];

                        if (!Ability)
                            continue;

                        Abilities::GrantGameplayAbility(Pawn, Ability);
                    }
                }
            }
        }
    }

}