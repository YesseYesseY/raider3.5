#pragma once

#include "GameModeBase.hpp"

class GameModePlayground : public AbstractGameModeBase
{
public:
    GameModePlayground()
        : GameModePlayground("FortPlaylistAthena Playlist_Playground.Playlist_Playground")
    {
    }

    GameModePlayground(std::string PlaylistName)
        : AbstractGameModeBase(PlaylistName, true, 1, false, true)
    {
    }

    void Initialize()
    {
        LOG_INFO("Initializing GameMode Playground!");
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
