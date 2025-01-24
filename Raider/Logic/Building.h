#pragma once
#include "../UE4.h"

namespace Building
{
    static void FarmBuild(SDK::ABuildingActor* Build, SDK::ABuildingActor_OnDamageServer_Params* Params)
    {
        auto Controller = (SDK::AFortPlayerControllerAthena*)Params->InstigatedBy;
        auto Pawn = (SDK::AFortPlayerPawnAthena*)Controller->Pawn;

        if (!Params->DamageCauser->IsA(SDK::AB_Melee_Impact_Generic_C::StaticClass()))
            return;

        if (Build->IsA(SDK::ABuildingSMActor::StaticClass()))
        {
            auto BuildSM = (SDK::ABuildingSMActor*)Build;
            auto MatDef = Game::EFortResourceTypeToItemDef(BuildSM->ResourceType.GetValue());
            if (!MatDef)
                return;

            static SDK::UCurveTable* resrates = nullptr;
            if (resrates == nullptr)
            {
                auto fromplaylist = Spawners::LoadObject<SDK::UCurveTable>(SDK::UCurveTable::StaticClass(), Game::Mode->BasePlaylist->ResourceRates.ObjectID.AssetPathName.ToWString(true).c_str());
                if (fromplaylist)
                    resrates = fromplaylist;
                else
                    resrates = BuildSM->BuildingResourceAmountOverride.CurveTable;
            }

            SDK::TEnumAsByte<SDK::EEvaluateCurveTableResult> EvalResult;
            float EvalOut;
            GetDataTableFunctionLibrary()->STATIC_EvaluateCurveTableRow(resrates, BuildSM->BuildingResourceAmountOverride.RowName, 0, L"Get resource rate", &EvalResult, &EvalOut);

            auto RawDamage = Params->Damage;
            // this isn't exactly like how it is supposed to be but it isn't bad either
            auto MatCount = floor(EvalOut / (BuildSM->GetMaxHealth() / RawDamage));
            if (MatCount <= 0)
                return;
            auto Pickup = Spawners::SummonPickup(Pawn, MatDef, MatCount, Pawn->K2_GetActorLocation());
            if (Pickup)
            {
                Pawn->ServerHandlePickup(Pickup, 1.0f, SDK::FVector(), true);
                Controller->ClientReportDamagedResourceBuilding(BuildSM, BuildSM->ResourceType, MatCount, BuildSM->GetHealth() <= 0, RawDamage > 50);
            }
        }
    }
}