#pragma once
#include "../UE4.h"
#include "Abilities.h"

namespace Spawners
{
    template <typename T = SDK::UObject>
    static T* LoadObject(SDK::UClass* Class, const wchar_t* Name)
    {
        return (T*)Native::Static::StaticLoadObjectInternal(Class, nullptr, Name, nullptr, 0, nullptr, false);
    }

    static SDK::AActor* SpawnActor(SDK::UClass* StaticClass, SDK::FTransform SpawnTransform, SDK::AActor* Owner = nullptr, SDK::ESpawnActorCollisionHandlingMethod Flags = SDK::ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn)
    {
        SDK::AActor* FirstActor = GetGameplayStatics()->STATIC_BeginDeferredActorSpawnFromClass(GetWorld(), StaticClass, SpawnTransform, Flags, Owner);

        if (FirstActor)
        {
            SDK::AActor* FinalActor = GetGameplayStatics()->STATIC_FinishSpawningActor(FirstActor, SpawnTransform);

            if (FinalActor)
            {
                return FinalActor;
            }
        }

        return nullptr;
    }

    static SDK::AActor* SpawnActor(SDK::UClass* ActorClass, SDK::FVector Location = { 0.0f, 0.0f, 0.0f }, SDK::FRotator Rotation = { 0, 0, 0 }, SDK::AActor* Owner = nullptr, SDK::ESpawnActorCollisionHandlingMethod Flags = SDK::ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn)
    {
        SDK::FTransform SpawnTransform;

        SpawnTransform.Translation = Location;
        SpawnTransform.Scale3D = SDK::FVector { 1, 1, 1 };
        SpawnTransform.Rotation = Utils::RotToQuat(Rotation);

        return SpawnActor(ActorClass, SpawnTransform, Owner, Flags);
    }

    template <typename RetActorType = SDK::AActor>
    static RetActorType* SpawnActor(SDK::FVector Location = { 0.0f, 0.0f, 0.0f }, SDK::AActor* Owner = nullptr, SDK::FQuat Rotation = { 0, 0, 0 }, SDK::ESpawnActorCollisionHandlingMethod Flags = SDK::ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn, bool attempts = 10)
    {
        SDK::FTransform SpawnTransform;

        SpawnTransform.Translation = Location;
        SpawnTransform.Scale3D = SDK::FVector { 1, 1, 1 };
        SpawnTransform.Rotation = Rotation;
        
        for (int i = 0; i < attempts; i++)
        {
            auto ret = static_cast<RetActorType*>(SpawnActor(RetActorType::StaticClass(), SpawnTransform, Owner, Flags));
            if (ret)
                return ret;
        }
        return nullptr;
    }

    static SDK::ABuildingSMActor* SpawnBuilding(SDK::UClass* BGAClass, SDK::FVector& Location, SDK::FRotator& Rotation, SDK::APlayerPawn_Athena_C* Pawn)
    {
        SDK::FTransform Transform;
        Transform.Translation = Location;
        Transform.Scale3D = SDK::FVector { 1, 1, 1 };
        Transform.Rotation = Utils::RotToQuat(Rotation);

        return (SDK::ABuildingSMActor*)GetFortKismet()->STATIC_SpawnBuildingGameplayActor(BGAClass, Transform, Pawn);
    }

    static SDK::AFortPickup* SummonPickup(SDK::AFortPlayerPawn* Pawn, auto ItemDef, int Count, SDK::FVector Location)
    {
        auto FortPickup = SpawnActor<SDK::AFortPickup>(Location, Pawn);

        FortPickup->bReplicates = true; // should be autmoatic but eh

        FortPickup->PrimaryPickupItemEntry.Count = Count;
        FortPickup->PrimaryPickupItemEntry.ItemDefinition = ItemDef;

        FortPickup->OnRep_PrimaryPickupItemEntry();
        FortPickup->TossPickup(Location, Pawn, 6, true);

        return FortPickup;
    }

    static void SummonPickupFromChest(auto ItemDef, int Count, SDK::FVector Location)
    {
        int attempts = 0;
        for (int i = 0; i < 5; i++)
        {
            auto FortPickup = SpawnActor<SDK::AFortPickup>(Location);
            if (FortPickup)
            {
                FortPickup->bReplicates = true; // should be autmoatic but eh
                FortPickup->bRandomRotation = true;

                FortPickup->PrimaryPickupItemEntry.Count = Count;
                FortPickup->PrimaryPickupItemEntry.ItemDefinition = ItemDef;
                if (ItemDef->IsA(SDK::UFortWeaponRangedItemDefinition::StaticClass()))
                {
                    auto weapdef = (SDK::UFortWeaponItemDefinition*)ItemDef;
                    FortPickup->PrimaryPickupItemEntry.LoadedAmmo = ((SDK::FFortBaseWeaponStats*)weapdef->WeaponStatHandle.DataTable->RowMap.GetByKey(weapdef->WeaponStatHandle.RowName))->ClipSize;
                }

                FortPickup->OnRep_PrimaryPickupItemEntry();
                FortPickup->OnRep_TossedFromContainer();
                FortPickup->TossPickup(Location, nullptr, 6, true);
                break;
            }
        }
    }

    static void SpawnPickupFromFloorLoot(auto ItemDef, int Count, SDK::FVector Location)
    {
        int attempts = 0;
        for (int i = 0; i < 5; i++)
        {
            auto FortPickup = SpawnActor<SDK::AFortPickup>(Location);

            if (FortPickup)
            {
                FortPickup->bReplicates = true; // should be autmoatic but eh

                FortPickup->PrimaryPickupItemEntry.Count = Count;
                FortPickup->PrimaryPickupItemEntry.ItemDefinition = ItemDef;
                if (ItemDef->IsA(SDK::UFortWeaponRangedItemDefinition::StaticClass()))
                {
                    auto weapdef = (SDK::UFortWeaponItemDefinition*)ItemDef;
                    FortPickup->PrimaryPickupItemEntry.LoadedAmmo = ((SDK::FFortBaseWeaponStats*)weapdef->WeaponStatHandle.DataTable->RowMap.GetByKey(weapdef->WeaponStatHandle.RowName))->ClipSize;
                }

                FortPickup->OnRep_PrimaryPickupItemEntry();
                FortPickup->TossPickup(Location, nullptr, 6, false);
                break;
            }
        }
    }

    static void SpawnPickupFromContainer(SDK::ABuildingContainer* Container, SDK::UFortItemDefinition* ItemDef, int Count)
    {
        auto LootSpawnLocation_Athena = Container->LootSpawnLocation_Athena;
        auto Location = Container->K2_GetActorLocation() + (Container->GetActorRightVector() * LootSpawnLocation_Athena.Y) + (Container->GetActorUpVector() * LootSpawnLocation_Athena.Z);

        auto FortPickup = SpawnActor<SDK::AFortPickup>(Location);
        if (FortPickup)
        {
            FortPickup->bReplicates = true; // should be autmoatic but eh

            FortPickup->PrimaryPickupItemEntry.Count = Count;
            FortPickup->PrimaryPickupItemEntry.ItemDefinition = ItemDef;
            if (ItemDef->IsA(SDK::UFortWeaponRangedItemDefinition::StaticClass()))
            {
                auto weapdef = (SDK::UFortWeaponItemDefinition*)ItemDef;
                FortPickup->PrimaryPickupItemEntry.LoadedAmmo = ((SDK::FFortBaseWeaponStats*)weapdef->WeaponStatHandle.DataTable->RowMap.GetByKey(weapdef->WeaponStatHandle.RowName))->ClipSize;
            }
            
            FortPickup->OnRep_PrimaryPickupItemEntry();
            
            FortPickup->bTossedFromContainer = true;
            FortPickup->OnRep_TossedFromContainer();

            // TODO: Fix floorloot getting tossed too far

            FortPickup->TossPickup(Location, nullptr, 6, true);
        }
    }

    static void SpawnDeco(SDK::AFortDecoTool* Tool, void* _Params)
    {
        if (!_Params)
            return;

        auto Params = static_cast<SDK::AFortDecoTool_ServerSpawnDeco_Params*>(_Params);

        SDK::FTransform Transform {};
        Transform.Scale3D = SDK::FVector(1, 1, 1);
        Transform.Rotation = Utils::RotToQuat(Params->Rotation);
        Transform.Translation = Params->Location;

        SDK::UFortTrapItemDefinition* TrapDef;
        // This is not correct but it works i guess
        if (Tool->IsA(SDK::AFortDecoTool_ContextTrap::StaticClass()))
        {
            auto buildpos = Params->AttachedActor->K2_GetActorLocation();
            if (Params->AttachedActor->BuildingAttachmentSlot == SDK::EBuildingAttachmentSlot::SLOT_Wall)
                TrapDef = static_cast<SDK::AFortDecoTool_ContextTrap*>(Tool)->ContextTrapItemDefinition->WallTrap;
            else if (buildpos.Z > Params->Location.Z)
                TrapDef = static_cast<SDK::AFortDecoTool_ContextTrap*>(Tool)->ContextTrapItemDefinition->CeilingTrap;
            else
                TrapDef = static_cast<SDK::AFortDecoTool_ContextTrap*>(Tool)->ContextTrapItemDefinition->FloorTrap;
        }
        else
        {
            TrapDef = static_cast<SDK::UFortTrapItemDefinition*>(Tool->ItemDefinition);
        }

        if (TrapDef)
        {
            auto Trap = static_cast<SDK::ABuildingTrap*>(SpawnActor(TrapDef->GetBlueprintClass(), Transform));

            if (Trap)
            {
                Trap->TrapData = static_cast<SDK::UFortTrapItemDefinition*>(Tool->ItemDefinition);

                auto Pawn = static_cast<SDK::APlayerPawn_Athena_C*>(Tool->Owner);

                Trap->InitializeKismetSpawnedBuildingActor(Trap, static_cast<SDK::AFortPlayerController*>(Pawn->Controller));

                Trap->AttachedTo = Params->AttachedActor;
                Trap->OnRep_AttachedTo();

                auto PlayerState = (SDK::AFortPlayerStateAthena*)Pawn->Controller->PlayerState;
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