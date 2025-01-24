#pragma once

#include "GUI.h"
#include "UFunctionHooks.h"

#define LOGGING

namespace Hooks
{
    bool LocalPlayerSpawnPlayActor(SDK::ULocalPlayer* Player, const SDK::FString& URL, SDK::FString& OutError, SDK::UWorld* World)
    {
        if (!bTraveled)
        {
            return Native::LocalPlayer::SpawnPlayActor(Player, URL, OutError, World);
        }
        return true;
    }

    void TickFlush(SDK::UNetDriver* NetDriver, float DeltaSeconds)
    {
        if (!NetDriver)
            return;

        if (NetDriver->IsA(SDK::UIpNetDriver::StaticClass()) && NetDriver->ClientConnections.Num() > 0 && NetDriver->ClientConnections[0]->InternalAck == false)
        {
            // Replication::ServerReplicateActors(NetDriver);
            if (NetDriver->ReplicationDriver)
            {
                Native::ReplicationDriver::ServerReplicateActors(NetDriver->ReplicationDriver);
            }
        }

        Native::NetDriver::TickFlush(NetDriver, DeltaSeconds);
    }

    uint8 Beacon_NotifyAcceptingConnection(SDK::AOnlineBeacon*) { return Native::World::NotifyAcceptingConnection(GetWorld()); }
    void* SeamlessTravelHandlerForWorld(SDK::UEngine* Engine, SDK::UWorld*) { return Native::Engine::SeamlessTravelHandlerForWorld(Engine, GetWorld()); }
    void* NetDebug(SDK::UObject*) { return nullptr; }
    __int64 CollectGarbage(__int64) { return 0; }
    void WelcomePlayer(SDK::UWorld*, SDK::UNetConnection* IncomingConnection) { Native::World::WelcomePlayer(GetWorld(), IncomingConnection); }
    char KickPlayer(__int64, __int64, __int64) { return 0; }
    uint64 GetNetMode(SDK::UWorld*) { return NM_ListenServer; }
    void World_NotifyControlMessage(SDK::UWorld*, SDK::UNetConnection* Connection, uint8 MessageType, void* Bunch) { Native::World::NotifyControlMessage(GetWorld(), Connection, MessageType, Bunch); }

    void __fastcall ReloadThing(SDK::AFortWeapon* Weapon, uint32_t AmountToRemove)
    {
        auto ammodef = (SDK::UFortAmmoItemDefinition*)SDK::UObject::GObjects->GetByIndex(((*(SDK::TSoftObjectPtr<SDK::UObject*>*)&Weapon->WeaponData->UnknownData10).WeakPtr.ObjectIndex));
        auto pawn = (SDK::APawn*)Weapon->Owner;
        auto controller = (SDK::AFortPlayerController*)pawn->Controller;
        if (!controller->bInfiniteAmmo)
        {
            if (!Inventory::TryRemoveItem((SDK::AFortPlayerControllerAthena*)pawn->Controller, ammodef, AmountToRemove))
            {
                LOG_ERROR("Failed to remove ammo from user");
            }
        }
        //return Native::Inventory::ReloadThing(Weapon, AmountToRemove);
    }

    void __fastcall GetPlayerViewPoint(SDK::APlayerController* pc, SDK::FVector* a2, SDK::FRotator* a3)
    {
        if (pc && HostBeacon && HostBeacon->NetDriver && HostBeacon->NetDriver->ClientConnections.Num() > 0)
        {
            SDK::AActor* TheViewTarget = pc->GetViewTarget();

            if (TheViewTarget)
            {
                if (a2)
                    *a2 = TheViewTarget->K2_GetActorLocation();
                if (a3)
                    *a3 = TheViewTarget->K2_GetActorRotation();
                // LOG_INFO("Did the ViewTarget!");

                return;
            }
            else
                LOG_INFO("Unable to get ViewTarget!");
        }
        else
            return Native::PlayerController::GetPlayerViewPoint(pc, a2, a3);
    }

    SDK::APlayerController* SpawnPlayActor(SDK::UWorld*, SDK::UPlayer* NewPlayer, SDK::ENetRole RemoteRole, SDK::FURL& URL, void* UniqueId, SDK::FString& Error, uint8 NetPlayerIndex)
    {
        auto PlayerController = static_cast<SDK::AFortPlayerControllerAthena*>(Native::World::SpawnPlayActor(GetWorld(), NewPlayer, RemoteRole, URL, UniqueId, Error, NetPlayerIndex));
        NewPlayer->PlayerController = PlayerController;

        Game::Mode->LoadJoiningPlayer(PlayerController);

        PlayerController->OverriddenBackpackSize = 100;
        return PlayerController;
    }

    void Beacon_NotifyControlMessage(SDK::AOnlineBeaconHost*, SDK::UNetConnection* Connection, uint8 MessageType, int64* Bunch)
    {
        switch (MessageType)
        {
        case 4: // NMT_Netspeed
            Connection->CurrentNetSpeed = 30000;
            return;
        case 5: // NMT_Login
        {
            if (GetWorld()->GameState->HasMatchStarted())
                return;

            Bunch[7] += (16 * 1024 * 1024);

            auto OnlinePlatformName = SDK::FString(L"");

            Native::NetConnection::ReceiveFString(Bunch, Connection->ClientResponse);
            Native::NetConnection::ReceiveFString(Bunch, Connection->RequestURL);
            Native::NetConnection::ReceiveUniqueIdRepl(Bunch, Connection->PlayerID);
            Native::NetConnection::ReceiveFString(Bunch, OnlinePlatformName);

            Bunch[7] -= (16 * 1024 * 1024);

            Native::World::WelcomePlayer(GetWorld(), Connection);

            return;
        }
        case 15: // NMT_PCSwap
            break;
        }

        Native::World::NotifyControlMessage(GetWorld(), Connection, MessageType, Bunch);
    }

    void PostRender(SDK::UGameViewportClient* _this, SDK::UCanvas* Canvas)
    {
        ZeroGUI::SetupCanvas(Canvas);
        GUI::Tick();

        return Native::GameViewportClient::PostRender(_this, Canvas);
    }

    void InitNetworkHooks()
    {
        DETOUR_START
        DetourAttachE(Native::World::WelcomePlayer, WelcomePlayer);
        DetourAttachE(Native::Actor::GetNetMode, GetNetMode);
        DetourAttachE(Native::World::NotifyControlMessage, World_NotifyControlMessage);
        DetourAttachE(Native::World::SpawnPlayActor, SpawnPlayActor);
        DetourAttachE(Native::OnlineBeaconHost::NotifyControlMessage, Beacon_NotifyControlMessage);
        DetourAttachE(Native::OnlineSession::KickPlayer, KickPlayer);
        DetourAttachE(Native::GC::CollectGarbage, CollectGarbage);
        DETOUR_END
    }

    void ProcessEventHook(SDK::UObject* Object, SDK::UFunction* Function, void* Parameters)
    {
        if (bTraveled)
        {
#ifdef LOGGING
            auto FunctionName = Function->GetName();
            if (Function->FunctionFlags & 0x00200000 || (Function->FunctionFlags & 0x01000000 && FunctionName.find("Ack") == -1 && FunctionName.find("AdjustPos") == -1))
            {
                if (FunctionName.find("ServerUpdateCamera") == -1 && FunctionName.find("ServerMove") == -1 && FunctionName.find("ServerFireAIDirectorEventBatch") == -1 &&
                    FunctionName.find("ServerTriggerCombatEventBatch") == -1 && FunctionName.find("ServerTriggerCombatEvent") == -1 && FunctionName.find("ServerFireAIDirectorEvent") == -1 &&
                    FunctionName.find("ServerEndAbility") == -1 && FunctionName.find("ClientEndAbility") == -1 && FunctionName.find("ServerCurrentMontageSetNextSectionName") == -1)
                {
                    LOG_INFO("RPC Called: {}", FunctionName);
                }
            }
#endif

            for (int i = 0; i < UFunctionHooks::toHook.size(); i++)
            {
                if (Function == UFunctionHooks::toHook[i])
                {
                    if (UFunctionHooks::toCall[i](Object, Parameters))
                    {
                        return;
                    }
                    break;
                }
            }
        }

        return ProcessEvent(Object, Function, Parameters);
    }
}
