#pragma once

#include "GameModeBase.hpp"

class GameModeSolos : public AbstractGameModeBase
{
public:
    GameModeSolos()
        : GameModeSolos("FortPlaylistAthena Playlist_DefaultSolo.Playlist_DefaultSolo")
    {
    }

    GameModeSolos(std::string SoloPlaylistName)
        : AbstractGameModeBase(SoloPlaylistName, false, 1)
    {
    }

    void Initialize()
    {
        LOG_INFO("Initializing GameMode Solo!");
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