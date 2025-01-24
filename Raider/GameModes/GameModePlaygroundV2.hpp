#pragma once

#include "GameModeBase.hpp"

class GameModePlaygroundV2 : public AbstractGameModeBase
{
public:
    GameModePlaygroundV2()
        : GameModePlaygroundV2("FortPlaylistAthena Playlist_Playground.Playlist_Playground")
    {
    }

    GameModePlaygroundV2(std::string PlaylistName)
        : AbstractGameModeBase(PlaylistName, true, 1, false, true)
    {
    }

    void Initialize()
    {
        LOG_INFO("Initializing GameMode PlaygroundV2!");
        SDK::AAthena_GameState_C* GameState = static_cast<SDK::AAthena_GameState_C*>(GetWorld()->GameState);
        SDK::UCurveTable* GameData = (SDK::UCurveTable*)GameState->CurrentPlaylistData->GameData.WeakPtr.Get();
        if (!GameData)
        {
            GameData = Spawners::LoadObject<SDK::UCurveTable>(SDK::UCurveTable::StaticClass(), GameState->CurrentPlaylistData->GameData.ObjectID.AssetPathName.ToWString(true).c_str());
        }

        if (!GameData)
        {
            LOG_ERROR("GameData not found");
            return;
        }

        for (auto& pair : GameData->RowMap)
        {
            auto KeyName = pair.Key().ToString();

            #define CHANGE_VAL(Key, Val, Index) if (KeyName == Key) \
            { \
                pair.Value()->Keys[Index].Value = Val; \
            }

            // Gotten from 4.5
            CHANGE_VAL("Default.Aircraft.Height", 8000, 0);
            CHANGE_VAL("Default.SafeZone.StartingRadius", 225000, 0);
            CHANGE_VAL("Default.SafeZone.StartingRadius", 60, 1);
            CHANGE_VAL("Default.SafeZone.00.Radius", 175000, 0);
            CHANGE_VAL("Default.SafeZone.01.Radius", 225000, 0);
            CHANGE_VAL("Default.SafeZone.02.Radius", 10, 0);
            CHANGE_VAL("Default.SafeZone.02.WaitTime", 3300, 0);
            CHANGE_VAL("Default.SafeZone.02.ShrinkTime", 300, 0);

            LOG_INFO("{}: {}", KeyName, pair.Value()->Keys[0].Value);
        }
    }

    void OnPlayerJoined(SDK::AFortPlayerControllerAthena* Controller) override
    {
        this->Teams->AddPlayerToRandomTeam(Controller);
    }

    void InitializeGameplay()
    {
    }

    /*void OnPlayerKilled(AFortPlayerControllerAthena*& Controller) override
    {
    }*/
};
