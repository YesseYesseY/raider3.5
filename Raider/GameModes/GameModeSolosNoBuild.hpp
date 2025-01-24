#pragma once

#include "GameModeBase.hpp"

class GameModeSolosNoBuild : public AbstractGameModeBase
{
public:
    GameModeSolosNoBuild()
        : GameModeSolosNoBuild("FortPlaylistAthena Playlist_DefaultSolo.Playlist_DefaultSolo")
    {
    }

    GameModeSolosNoBuild(std::string SoloPlaylistName)
        : AbstractGameModeBase(SoloPlaylistName, false, 1)
    {
    }

    void Initialize()
    {
        LOG_INFO("Initializing GameMode Solo No Builds!");
    }

    void OnPlayerJoined(SDK::AFortPlayerControllerAthena* Controller) override
    {
        this->Teams->AddPlayerToRandomTeam(Controller);

        static auto BuildingItemData_Wall = SDK::UObject::FindObject<SDK::UFortBuildingItemDefinition>("FortBuildingItemDefinition BuildingItemData_Wall.BuildingItemData_Wall");
        static auto BuildingItemData_Floor = SDK::UObject::FindObject<SDK::UFortBuildingItemDefinition>("FortBuildingItemDefinition BuildingItemData_Floor.BuildingItemData_Floor");
        static auto BuildingItemData_Stair_W = SDK::UObject::FindObject<SDK::UFortBuildingItemDefinition>("FortBuildingItemDefinition BuildingItemData_Stair_W.BuildingItemData_Stair_W");
        static auto BuildingItemData_RoofS = SDK::UObject::FindObject<SDK::UFortBuildingItemDefinition>("FortBuildingItemDefinition BuildingItemData_RoofS.BuildingItemData_RoofS");
        static auto TID_Floor_Player_Launch_Pad_Athena = SDK::UObject::FindObject<SDK::UFortTrapItemDefinition>("FortTrapItemDefinition TID_Floor_Player_Launch_Pad_Athena.TID_Floor_Player_Launch_Pad_Athena");
        static auto TID_ContextTrap_Athena = SDK::UObject::FindObject<SDK::UFortTrapItemDefinition>("FortContextTrapItemDefinition TID_ContextTrap_Athena.TID_ContextTrap_Athena");
        static auto TID_Floor_Player_Campfire_Athena = SDK::UObject::FindObject<SDK::UFortTrapItemDefinition>("FortTrapItemDefinition TID_Floor_Player_Campfire_Athena.TID_Floor_Player_Campfire_Athena");


        Inventory::TryRemoveItem(Controller, BuildingItemData_Wall, 1, false);
        Inventory::TryRemoveItem(Controller, BuildingItemData_Floor, 1, false);
        Inventory::TryRemoveItem(Controller, BuildingItemData_Stair_W, 1, false);
        Inventory::TryRemoveItem(Controller, BuildingItemData_RoofS, 1, false);
        Inventory::TryRemoveItem(Controller, TID_Floor_Player_Launch_Pad_Athena, 1, false);
        Inventory::TryRemoveItem(Controller, TID_ContextTrap_Athena, 1, false);
        Inventory::TryRemoveItem(Controller, TID_Floor_Player_Campfire_Athena, 1, false);

        for (int i = 0; i < 5; i++)
        {
            Controller->QuickBars->ServerDisableSlot(SDK::EFortQuickBars::Secondary, i);
        }
        
    }

    void InitializeGameplay()
    {
    }

    /*void OnPlayerKilled(AFortPlayerControllerAthena*& Controller) override
    {
    }*/
};