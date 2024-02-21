#pragma once

#include "ZeroGUI.h"
#include <format>
#include <mutex>

enum class CustomMode
{
    NONE,
    JUGGERNAUT,
    // Gives the players 500 health and makes you slower.
    LATEGAME,
    // TODO: You know what late game is.
    LIFESTEAL,
    // TODO: You know what life steal is, but this might be a stupid idea.
    SPACE,
    // Sets gravity like the moon // BUG: Unfortunately, the gravityscale variable doesn't update for the client, making them rubberband and making it look weird.
    SIPHON // Gives 50 shield/health whenever you finish someone. (Late game also has this)
};

// constexpr CustomMode Mode = CustomMode::NONE;

namespace GUI
{
    std::mutex mtx;
    void Tick()
    {
        ZeroGUI::Input::Handle();

        static bool menu_opened = true;

        if (GetAsyncKeyState(VK_F2) & 1)
            menu_opened = !menu_opened;

        static auto pos = FVector2D { 200.f, 250.0f };

        if (ZeroGUI::Window(L"Raider", &pos, FVector2D { 500.0f, 700.0f }, menu_opened))
        {
            if (bListening && HostBeacon)
            {
                static auto GameState = reinterpret_cast<AAthena_GameState_C*>(GetWorld()->GameState);
                static APlayerState* currentPlayer = nullptr;

                // This is bad, but works for now.
                if (currentPlayer)
                {
                    auto playerName = currentPlayer->GetPlayerName();
                    if (ZeroGUI::Button(L"<", { 25.0f, 25.0f }))
                    {
                        mtx.lock();
                        currentPlayer = nullptr;
                        mtx.unlock();
                    }

                    ZeroGUI::NextColumn(90.0f);

                    ZeroGUI::Text(std::format(L"Current Player: {}", playerName.c_str()).c_str());

                    if (ZeroGUI::Button(L"Kick", { 60.0f, 25.0f }))
                    {
                        KickController((APlayerController*)currentPlayer->Owner, L"You have been kicked by the server.");

                        mtx.lock();
                        currentPlayer = nullptr;
                        mtx.unlock();
                    }
                }
                else
                {
                    static int tab = 0;
                    if (ZeroGUI::ButtonTab(L"Game", FVector2D { 110, 25 }, tab == 0))
                        tab = 0;
                    if (ZeroGUI::ButtonTab(L"Players", FVector2D { 110, 25 }, tab == 1))
                        tab = 1;
                    if (ZeroGUI::ButtonTab(L"Dev", FVector2D { 110, 25 }, tab == 2))
                        tab = 2;
                    if (ZeroGUI::ButtonTab(L"Event", FVector2D { 110, 25 }, tab == 3))
                        tab = 3;

                    ZeroGUI::NextColumn(130.0f);

                    switch (tab)
                    {
                    case 0:
                    {
                        if (!bStartedBus)
                        {
                            if (ZeroGUI::Button(L"Start Bus", FVector2D { 100, 25 }))
                            {
                                if (static_cast<AAthena_GameState_C*>(GetWorld()->GameState)->GamePhase >= EAthenaGamePhase::Aircraft)
                                {
                                    LOG_INFO("The bus has already started!")
                                    bStartedBus = true;
                                }

                                //GameState->bGameModeWillSkipAircraft = false;
                                //GameState->AircraftStartTime = 0;
                                //GameState->WarmupCountdownEndTime = 0;

                                GetKismetSystem()->STATIC_ExecuteConsoleCommand(GetWorld(), L"startaircraft", nullptr);
                                
                                Game::Mode->InitLoot();

                                //auto wood = UObject::FindObject<UFortResourceItemDefinition>("FortResourceItemDefinition WoodItemData.WoodItemData");
                                //auto pump = UObject::FindObject<UFortWeaponRangedItemDefinition>("FortWeaponRangedItemDefinition WID_Shotgun_Standard_Athena_UC_Ore_T03.WID_Shotgun_Standard_Athena_UC_Ore_T03");
                                

                                //auto vendclass = UObject::FindObject<UBlueprintGeneratedClass>("BlueprintGeneratedClass Tiered_Athena_FloorLoot_Warmup.Tiered_Athena_FloorLoot_Warmup_C");
                                //
                                //for (auto clas = (UClass*)vendclass; clas; clas = static_cast<UClass*>(clas->SuperField))
                                //{
                                //    LOG_INFO("OK: {}", clas->GetFullName());
                                //}

                                //Game::Mode->InitializeGameplay();
                                //Native::OnlineBeacon::PauseBeaconRequests(HostBeacon, true);
                                LOG_INFO("The bus has been started!")
                                bStartedBus = true;
                            }
                        }
                        break;
                    }
                    case 1:
                    {
                        std::wstring ConnectedPlayers = std::format(L"Connected Players: {}\n", GameState->PlayerArray.Num());

                        ZeroGUI::Text(ConnectedPlayers.c_str());

                        for (auto i = 0; i < GameState->PlayerArray.Num(); i++)
                        {
                            auto PlayerState = GameState->PlayerArray[i];

                            if (ZeroGUI::Button(PlayerState->GetPlayerName().c_str(), { 100, 25 }))
                            {
                                currentPlayer = PlayerState;
                            }
                        }

                        break;
                    }

                    case 2:
                    {
                        if (ZeroGUI::Button(L"Dump Objects", FVector2D { 100, 25 }))
                        {
                            DumpObjects();
                        }
                        if (ZeroGUI::Button(L"Init loot", FVector2D { 100, 25 }))
                        {
                            Game::Mode->InitLoot();
                        }

                        if (ZeroGUI::Button(L"Test", FVector2D { 100, 25 }))
                        {
                            for (int i = 0; i < 4; i++)
                            {
                                if (static_cast<UFortAssetManager*>(GetEngine()->AssetManager)->GameData->BuildingRepairCostMultiplierHandles[i].CurveTable)
                                {
                                    LOG_INFO("{}: {}", i, static_cast<UFortAssetManager*>(GetEngine()->AssetManager)->GameData->BuildingRepairCostMultiplierHandles[i].CurveTable->GetFullName());
                                }
                                else
                                {
                                    LOG_INFO("{}, NULL", i);
                                }
                            }
                        }
                        break;
                    }
                    case 3:
                    {

                        static ABP_Athena_Event_Components_C* EventComponent = nullptr;

                        if (ZeroGUI::Button(L"Spawn Meteor", FVector2D { 100, 25 }))
                        {
                            if (EventComponent == nullptr)
                            {
                                UObject::FindObject<ABP_Athena_Event_Components_C>("BP_Athena_Event_Components_C Athena_GameplayActors.Athena_GameplayActors.PersistentLevel.BP_Athena_Event_Components_54_55");
                                TArray<AActor*> EventComponents;
                                GetGameplayStatics()->STATIC_GetAllActorsOfClass(GetWorld(), ABP_Athena_Event_Components_C::StaticClass(), &EventComponents);
                                if (EventComponents.Count > 0)
                                {
                                    EventComponent = (ABP_Athena_Event_Components_C*)EventComponents[0];
                                }
                            }

                            EventComponent->RandomizeSpawnLocation();
                            EventComponent->SpawnMeteor();
                        }

                        if (ZeroGUI::Button(L"Spawn Shooting Stars", FVector2D { 100, 25 }))
                        {
                            if (EventComponent == nullptr)
                            {
                                UObject::FindObject<ABP_Athena_Event_Components_C>("BP_Athena_Event_Components_C Athena_GameplayActors.Athena_GameplayActors.PersistentLevel.BP_Athena_Event_Components_54_55");
                                TArray<AActor*> EventComponents;
                                GetGameplayStatics()->STATIC_GetAllActorsOfClass(GetWorld(), ABP_Athena_Event_Components_C::StaticClass(), &EventComponents);
                                if (EventComponents.Count > 0)
                                {
                                    EventComponent = (ABP_Athena_Event_Components_C*)EventComponents[0];
                                }
                            }

                            EventComponent->RandomizeSpawnLocation();
                            EventComponent->SpawnShootingStars();
                        }

                        static float cometpos = 0.0f;
                        ZeroGUI::SliderFloat(L"Comet pos", &cometpos, 0, 1, L"%.02f");

                        if (ZeroGUI::Button(L"Update Comet", FVector2D { 100, 25 }))
                        {
                            if (EventComponent == nullptr)
                            {
                                UObject::FindObject<ABP_Athena_Event_Components_C>("BP_Athena_Event_Components_C Athena_GameplayActors.Athena_GameplayActors.PersistentLevel.BP_Athena_Event_Components_54_55");
                                TArray<AActor*> EventComponents;
                                GetGameplayStatics()->STATIC_GetAllActorsOfClass(GetWorld(), ABP_Athena_Event_Components_C::StaticClass(), &EventComponents);
                                if (EventComponents.Count > 0)
                                {
                                    EventComponent = (ABP_Athena_Event_Components_C*)EventComponents[0];
                                }
                            }

                            EventComponent->CometPosition = cometpos;
                            EventComponent->OnRep_CometPosition();
                        }

                        break;
                    }
                    }
                }
            }
            else
            {
                // ZeroGUI::Text(L"Waiting for map to load...");
            }
        }

        ZeroGUI::Render();
        // ZeroGUI::Draw_Cursor(menu_opened);
    }
}
