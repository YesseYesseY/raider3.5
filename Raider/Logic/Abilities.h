#pragma once

#include "Native.h"

namespace Abilities
{
    SDK::FGameplayAbilitySpec* UAbilitySystemComponent_FindAbilitySpecFromHandle(SDK::UAbilitySystemComponent* AbilitySystemComponent, SDK::FGameplayAbilitySpecHandle Handle)
    {
        auto Specs = AbilitySystemComponent->ActivatableAbilities.Items;

        for (int i = 0; i < Specs.Num(); i++)
        {
            auto& Spec = Specs[i];

            if (Spec.Handle.Handle == Handle.Handle)
            {
                return &Spec;
            }
        }

        return nullptr;
    }

    void UAbilitySystemComponent_ConsumeAllReplicatedData(SDK::UAbilitySystemComponent* AbilitySystemComponent, SDK::FGameplayAbilitySpecHandle AbilityHandle, SDK::FPredictionKey AbilityOriginalPredictionKey)
    {
        /*
        FGameplayAbilitySpecHandleAndPredictionKey toFind { AbilityHandle, AbilityOriginalPredictionKey.Current };

        auto MapPairsData = AbilitySystemComponent->AbilityTargetDataMap;
        */
    }

    auto TryActivateAbility(SDK::UAbilitySystemComponent* AbilitySystemComponent, SDK::FGameplayAbilitySpecHandle AbilityToActivate, bool InputPressed, SDK::FPredictionKey* PredictionKey, SDK::FGameplayEventData* TriggerEventData)
    {
        auto Spec = UAbilitySystemComponent_FindAbilitySpecFromHandle(AbilitySystemComponent, AbilityToActivate);

        if (!Spec)
        {
            LOG_WARN("InternalServerTryActiveAbility. Rejecting ClientActivation of ability with invalid SpecHandle!");
            AbilitySystemComponent->ClientActivateAbilityFailed(AbilityToActivate, PredictionKey->Current);
            return;
        }

        // UAbilitySystemComponent_ConsumeAllReplicatedData(AbilitySystemComponent, AbilityToActivate, *PredictionKey);

        SDK::UGameplayAbility* InstancedAbility = nullptr;
        Spec->InputPressed = true;

        if (Native::AbilitySystemComponent::InternalTryActivateAbility(AbilitySystemComponent, AbilityToActivate, *PredictionKey, &InstancedAbility, nullptr, TriggerEventData))
        {
            // TryActivateAbility handles notifying the client of success
        }
        else
        {
            LOG_WARN("InternalServerTryActiveAbility. Rejecting ClientActivation of {}. InternalTryActivateAbility failed!", Spec->Ability->GetName());
            AbilitySystemComponent->ClientActivateAbilityFailed(AbilityToActivate, PredictionKey->Current);
            Spec->InputPressed = false;

            return;
        }

        Native::AbilitySystemComponent::MarkAbilitySpecDirty(AbilitySystemComponent, *Spec);
    }

    void GrantGameplayAbility(SDK::APlayerPawn_Athena_C* TargetPawn, SDK::UClass* GameplayAbilityClass)
    {
        auto AbilitySystemComponent = TargetPawn->AbilitySystemComponent;

        if (!AbilitySystemComponent)
            return;

        auto GenerateNewSpec = [&]() -> SDK::FGameplayAbilitySpec
        {
            SDK::FGameplayAbilitySpecHandle Handle { rand() };

            SDK::FGameplayAbilitySpec Spec { -1, -1, -1, Handle, (SDK::UGameplayAbility*)GameplayAbilityClass->CreateDefaultObject(), 1, -1, nullptr, 0, false, false, false };

            return Spec;
        };

        auto Spec = GenerateNewSpec();

        for (int i = 0; i < AbilitySystemComponent->ActivatableAbilities.Items.Num(); i++)
        {
            auto& CurrentSpec = AbilitySystemComponent->ActivatableAbilities.Items[i];

            if (CurrentSpec.Ability == Spec.Ability)
                return;
        }

        Native::AbilitySystemComponent::GiveAbility(AbilitySystemComponent, &Spec.Handle, Spec);
        return;
    }

    inline auto ApplyAbilities(SDK::APawn* _Pawn) // TODO: Check if the player already has the ability.
    {
        auto Pawn = (SDK::APlayerPawn_Athena_C*)_Pawn;

        auto GAS_DefaultPlayer = SDK::UObject::FindObject<SDK::UFortAbilitySet>("FortAbilitySet GAS_DefaultPlayer.GAS_DefaultPlayer");
        for (int i = 0; i < GAS_DefaultPlayer->GameplayAbilities.Count; i++)
        {
            auto Ability = GAS_DefaultPlayer->GameplayAbilities[i];
            if (Ability)
                GrantGameplayAbility(Pawn, Ability);
        }

        //static auto SprintAbility = UObject::FindClass("Class FortniteGame.FortGameplayAbility_Sprint");
        //static auto ReloadAbility = UObject::FindClass("Class FortniteGame.FortGameplayAbility_Reload");
        //static auto RangedWeaponAbility = UObject::FindClass("Class FortniteGame.FortGameplayAbility_RangedWeapon");
        //static auto JumpAbility = UObject::FindClass("Class FortniteGame.FortGameplayAbility_Jump");
        //static auto DeathAbility = UObject::FindClass("BlueprintGeneratedClass GA_DefaultPlayer_Death.GA_DefaultPlayer_Death_C");
        //static auto InteractUseAbility = UObject::FindClass("BlueprintGeneratedClass GA_DefaultPlayer_InteractUse.GA_DefaultPlayer_InteractUse_C");
        //static auto InteractSearchAbility = UObject::FindClass("BlueprintGeneratedClass GA_DefaultPlayer_InteractSearch.GA_DefaultPlayer_InteractSearch_C");
        //static auto EmoteAbility = UObject::FindClass("BlueprintGeneratedClass GAB_Emote_Generic.GAB_Emote_Generic_C");
        //static auto TrapAbility = UObject::FindClass("BlueprintGeneratedClass GA_TrapBuildGeneric.GA_TrapBuildGeneric_C");
        //static auto DanceGrenadeAbility = UObject::FindClass("BlueprintGeneratedClass GA_DanceGrenade_Stun.GA_DanceGrenade_Stun_C");

        //GrantGameplayAbility(Pawn, SprintAbility);
        //GrantGameplayAbility(Pawn, ReloadAbility);
        //GrantGameplayAbility(Pawn, RangedWeaponAbility);
        //GrantGameplayAbility(Pawn, JumpAbility);
        //GrantGameplayAbility(Pawn, DeathAbility);
        //GrantGameplayAbility(Pawn, InteractUseAbility);
        //GrantGameplayAbility(Pawn, InteractSearchAbility);
        //GrantGameplayAbility(Pawn, EmoteAbility);
        //GrantGameplayAbility(Pawn, TrapAbility);
        //GrantGameplayAbility(Pawn, DanceGrenadeAbility);
    }
}