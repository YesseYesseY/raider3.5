#pragma once

#include "Patterns.h"
#include "Util.h"

inline SDK::UFortEngine* GetEngine()
{
    static auto engine = SDK::UObject::FindObject<SDK::UFortEngine>("FortEngine_");
    return engine;
}

namespace Native
{
    namespace Inventory
    {
        void(__fastcall* ReloadThing)(SDK::AFortWeapon* Weapon, uint32_t AmountToRemove);
    }
    namespace Actor
    {
        inline __int64 (*GetNetMode)(__int64* a1);
    }

    namespace PlayerController
    {
        void(__fastcall* GetPlayerViewPoint)(SDK::APlayerController* pc, SDK::FVector* a2, SDK::FRotator* a3);
    }

    namespace LocalPlayer
    {
        bool (*SpawnPlayActor)(SDK::ULocalPlayer* Player, const SDK::FString& URL, SDK::FString& OutError, SDK::UWorld* World);
    }

    namespace GC
    {
        __int64 (*CollectGarbage)(__int64 a1);
    }

    namespace AbilitySystemComponent
    {
        inline SDK::FGameplayAbilitySpecHandle* (*GiveAbility)(SDK::UAbilitySystemComponent* _this, SDK::FGameplayAbilitySpecHandle* outHandle, SDK::FGameplayAbilitySpec inSpec);
        inline bool (*InternalTryActivateAbility)(SDK::UAbilitySystemComponent* _this, SDK::FGameplayAbilitySpecHandle Handle, SDK::FPredictionKey InPredictionKey, SDK::UGameplayAbility** OutInstancedAbility, void* /* FOnGameplayAbilityEnded::FDelegate* */ OnGameplayAbilityEndedDelegate, SDK::FGameplayEventData* TriggerEventData);
        inline void (*MarkAbilitySpecDirty)(SDK::UAbilitySystemComponent* _this, SDK::FGameplayAbilitySpec& Spec);
    }

    namespace NetDriver
    {
        inline void (*TickFlush)(SDK::UNetDriver* NetDriver, float DeltaSeconds);
        inline bool (*IsLevelInitializedForActor)(SDK::UNetDriver* NetDriver, SDK::AActor* Actor, SDK::UNetConnection* Connection);

        inline bool (*InitListen)(SDK::UObject* Driver, void* InNotify, SDK::FURL& LocalURL, bool bReuseAddressAndPort, SDK::FString& Error);
    }

    namespace ReplicationDriver
    {
        inline void (*ServerReplicateActors)(SDK::UReplicationDriver* ReplicationDriver);
    }

    namespace NetConnection
    {
        inline void (*ReceiveFString)(void* Bunch, SDK::FString& Str);
        inline void (*ReceiveUniqueIdRepl)(void* Bunch, SDK::FUniqueNetIdRepl& Str);
        inline SDK::FString (*LowLevelGetRemoteAddress)(SDK::UNetConnection* Connection, bool bAppendPort);
    }

    namespace OnlineSession
    {
        inline char (*KickPlayer)(SDK::AGameSession* a1, SDK::APlayerController*, SDK::FText a3);
    }

    namespace OnlineBeacon
    {
        inline void (*PauseBeaconRequests)(SDK::AOnlineBeacon* Beacon, bool bPause);
        inline uint8 (*NotifyAcceptingConnection)(SDK::AOnlineBeacon* Beacon);
    }

    namespace OnlineBeaconHost
    {
        inline bool (*InitHost)(SDK::AOnlineBeaconHost* Beacon);
        inline void (*NotifyControlMessage)(SDK::AOnlineBeaconHost* World, SDK::UNetConnection* Connection, uint8 MessageType, void* Bunch);
    }

    namespace World
    {
        inline void (*RemoveNetworkActor)(SDK::UWorld* World, SDK::AActor* Actor);
        inline void (*WelcomePlayer)(SDK::UWorld* World, SDK::UNetConnection* Connection);
        inline void (*NotifyControlMessage)(SDK::UWorld* World, SDK::UNetConnection* Connection, uint8 MessageType, void* Bunch);
        inline SDK::APlayerController* (*SpawnPlayActor)(SDK::UWorld* World, SDK::UPlayer* NewPlayer, SDK::ENetRole RemoteRole, SDK::FURL& URL, void* UniqueId, SDK::FString& Error, uint8 NetPlayerIndex);
        inline uint8 (*NotifyAcceptingConnection)(SDK::UWorld* World);
    }

    namespace Engine
    {
        inline void* (*SeamlessTravelHandlerForWorld)(SDK::UEngine* Engine, SDK::UWorld* World);
    }

    namespace GameViewportClient
    {
        inline void (*PostRender)(SDK::UGameViewportClient* _this, SDK::UCanvas* Canvas);
    }

    namespace Static
    {
        inline SDK::UObject* (*StaticLoadObjectInternal)(SDK::UClass* a1, SDK::UObject* a2, const wchar_t* a3, const wchar_t* a4, unsigned int a5, SDK::UPackageMap* a6, bool a7);
    }

    void InitializeAll()
    {
        Imagebase = (uintptr_t)GetModuleHandleA(nullptr);

        uintptr_t Address = Utils::FindPattern(Patterns::GObjects, true, 3);
        CheckNullFatal(Address, "Failed to find GObjects");
        AddressToFunction(Address, SDK::UObject::GObjects);

        Address = Utils::FindPattern(Patterns::Free);
        CheckNullFatal(Address, "Failed to find Free");
        AddressToFunction(Address, SDK::FMemory_Free);

        Address = Utils::FindPattern(Patterns::Realloc);
        CheckNullFatal(Address, "Failed to find Realloc");
        AddressToFunction(Address, SDK::FMemory_Realloc);

        Address = Utils::FindPattern(Patterns::Malloc);
        CheckNullFatal(Address, "Failed to find Malloc");
        AddressToFunction(Address, SDK::FMemory_Malloc);

        Address = Utils::FindPattern(Patterns::FNameToString);
        CheckNullFatal(Address, "Failed to find FNameToString");
        AddressToFunction(Address, SDK::FNameToString);

        Address = Utils::FindPattern(Patterns::TickFlush);
        CheckNullFatal(Address, "Failed to find TickFlush");
        AddressToFunction(Address, NetDriver::TickFlush);

        Address = Utils::FindPattern(Patterns::PauseBeaconRequests);
        CheckNullFatal(Address, "Failed to find PauseBeaconRequests");
        AddressToFunction(Address, OnlineBeacon::PauseBeaconRequests);

        Address = Utils::FindPattern(Patterns::InitHost);
        CheckNullFatal(Address, "Failed to find InitHost");
        AddressToFunction(Address, OnlineBeaconHost::InitHost);

        Address = Utils::FindPattern(Patterns::Beacon_NotifyControlMessage);
        CheckNullFatal(Address, "Failed to find NotifyControlMessage");
        AddressToFunction(Address, OnlineBeaconHost::NotifyControlMessage);

        Address = Utils::FindPattern(Patterns::WelcomePlayer);
        CheckNullFatal(Address, "Failed to find WelcomePlayer");
        AddressToFunction(Address, World::WelcomePlayer);

        Address = Utils::FindPattern(Patterns::World_NotifyControlMessage);
        CheckNullFatal(Address, "Failed to find NotifyControlMessage");
        AddressToFunction(Address, World::NotifyControlMessage);

        Address = Utils::FindPattern(Patterns::SpawnPlayActor);
        CheckNullFatal(Address, "Failed to find SpawnPlayActor");
        AddressToFunction(Address, World::SpawnPlayActor);

        Address = Utils::FindPattern(Patterns::ReceiveUniqueIdRepl);
        CheckNullFatal(Address, "Failed to find ReceiveUniqueIdRepl");
        AddressToFunction(Address, NetConnection::ReceiveUniqueIdRepl);

        Address = Utils::FindPattern(Patterns::ReceiveFString);
        CheckNullFatal(Address, "Failed to find ReceiveFString");
        AddressToFunction(Address, NetConnection::ReceiveFString);

        Address = Utils::FindPattern(Patterns::KickPlayer);
        CheckNullFatal(Address, "Failed to find KickPlayer");
        AddressToFunction(Address, OnlineSession::KickPlayer);

        Address = Utils::FindPattern(Patterns::GetNetMode);
        CheckNullFatal(Address, "Failed to find InternalGetNetMode");
        AddressToFunction(Address, Actor::GetNetMode);

        Address = Utils::FindPattern(Patterns::GiveAbility);
        CheckNullFatal(Address, "Failed to find GiveAbility");
        AddressToFunction(Address, AbilitySystemComponent::GiveAbility);

        Address = Utils::FindPattern(Patterns::InternalTryActivateAbility);
        CheckNullFatal(Address, "Failed to find InternalTryActivateAbility");
        AddressToFunction(Address, AbilitySystemComponent::InternalTryActivateAbility);

        Address = Utils::FindPattern(Patterns::MarkAbilitySpecDirty);
        CheckNullFatal(Address, "Failed to find MarkAbilitySpecDirty");
        AddressToFunction(Address, AbilitySystemComponent::MarkAbilitySpecDirty);

        Address = Utils::FindPattern(Patterns::LocalPlayerSpawnPlayActor);
        CheckNullFatal(Address, "Failed to find LocalPlayerSpawnPlayActor");
        AddressToFunction(Address, LocalPlayer::SpawnPlayActor);

        Address = Utils::FindPattern(Patterns::PostRender);
        CheckNullFatal(Address, "Failed to find PostRender");
        AddressToFunction(Address, GameViewportClient::PostRender);

        Address = Utils::FindPattern(Patterns::CollectGarbage, true, 1);
        CheckNullFatal(Address, "Failed to find CollectGarbage");
        AddressToFunction(Address, GC::CollectGarbage);

        Address = Utils::FindPattern(Patterns::InitListen);
        CheckNullFatal(Address, "Failed to find NetDriver::InitListen");
        AddressToFunction(Address, NetDriver::InitListen);

        Address = Utils::FindPattern(Patterns::GetPlayerViewPoint);
        CheckNullFatal(Address, "Failed to find PlayerController::GetPlayerViewPoint");
        AddressToFunction(Address, PlayerController::GetPlayerViewPoint);

        Address = Utils::FindPattern(Patterns::ReloadThing);
        CheckNullFatal(Address, "Failed to find ReloadThing");
        AddressToFunction(Address, Inventory::ReloadThing);

        Address = Utils::FindPattern(Patterns::StaticLoadObject);
        CheckNullFatal(Address, "Failed to find StaticLoadObject");
        AddressToFunction(Address, Static::StaticLoadObjectInternal);

        ProcessEvent = reinterpret_cast<decltype(ProcessEvent)>(GetEngine()->Vtable[0x40]);
    }
}
