#pragma once

#include <unordered_set>
#include <random>

#include "Util.h"
#include "json.hpp"
#include "Native.h"

typedef std::array<SDK::UFortWeaponRangedItemDefinition*, 6> PlayerLoadout;

inline bool bTraveled = false;
inline bool bListening = false;
static bool bSpawnedFloorLoot = false;

static std::vector<SDK::ABuildingActor*> ExistingBuildings;
static SDK::AFortOnlineBeaconHost* HostBeacon = nullptr;

inline SDK::UWorld* GetWorld()
{
    return GetEngine()->GameViewport->World;
}

inline SDK::AAthena_PlayerController_C* GetPlayerController(int32 Index = 0)
{
    if (Index > GetWorld()->OwningGameInstance->LocalPlayers.Num())
    {
        return static_cast<SDK::AAthena_PlayerController_C*>(GetWorld()->OwningGameInstance->LocalPlayers[0]->PlayerController);
    }

    return static_cast<SDK::AAthena_PlayerController_C*>(GetWorld()->OwningGameInstance->LocalPlayers[Index]->PlayerController);
}

struct FObjectKey
{
public:
    SDK::UObject* ResolveObjectPtr() const
    {
        SDK::FWeakObjectPtr WeakPtr;
        WeakPtr.ObjectIndex = ObjectIndex;
        WeakPtr.ObjectSerialNumber = ObjectSerialNumber;

        return WeakPtr.Get();
    }

    int32 ObjectIndex;
    int32 ObjectSerialNumber;
};

FORCEINLINE auto& GetClassRepNodePolicies(SDK::UObject* ReplicationDriver)
{
    return *reinterpret_cast<SDK::TMap<FObjectKey, SDK::EClassRepNodeMapping>*>(__int64(ReplicationDriver) + 0x3B8);
}

FORCEINLINE SDK::UGameplayStatics* GetGameplayStatics()
{
    return reinterpret_cast<SDK::UGameplayStatics*>(SDK::UGameplayStatics::StaticClass());
}

FORCEINLINE SDK::UKismetSystemLibrary* GetKismetSystem()
{
    return reinterpret_cast<SDK::UKismetSystemLibrary*>(SDK::UKismetSystemLibrary::StaticClass());
}

FORCEINLINE SDK::UDataTableFunctionLibrary* GetDataTableFunctionLibrary()
{
    return reinterpret_cast<SDK::UDataTableFunctionLibrary*>(SDK::UDataTableFunctionLibrary::StaticClass());
}

FORCEINLINE SDK::UFortKismetLibrary* GetFortKismet()
{
    return ((SDK::UFortKismetLibrary*)SDK::UFortKismetLibrary::StaticClass());
}

FORCEINLINE SDK::UKismetStringLibrary* GetKismetString()
{
    return (SDK::UKismetStringLibrary*)SDK::UKismetStringLibrary::StaticClass();
}
FORCEINLINE SDK::UKismetTextLibrary* GetKismetText()
{
    return (SDK::UKismetTextLibrary*)SDK::UKismetTextLibrary::StaticClass();
}
FORCEINLINE SDK::UKismetMathLibrary* GetKismetMath()
{
    return (SDK::UKismetMathLibrary*)SDK::UKismetMathLibrary::StaticClass();
}
FORCEINLINE SDK::UKismetGuidLibrary* GetKismetGuid()
{
    return (SDK::UKismetGuidLibrary*)SDK::UKismetGuidLibrary::StaticClass();
}

inline void CreateConsole()
{
    GetEngine()->GameViewport->ViewportConsole = static_cast<SDK::UConsole*>(GetGameplayStatics()->STATIC_SpawnObject(SDK::UConsole::StaticClass(), GetEngine()->GameViewport));
}

inline auto CreateCheatManager(SDK::APlayerController* Controller)
{
    if (!Controller->CheatManager)
    {
        Controller->CheatManager = static_cast<SDK::UCheatManager*>(GetGameplayStatics()->STATIC_SpawnObject(SDK::UFortCheatManager::StaticClass(), Controller)); // lets just assume its gamemode athena
    }

    return static_cast<SDK::UFortCheatManager*>(Controller->CheatManager);
}

bool CanBuild(SDK::ABuildingSMActor* BuildingActor)
{
    bool bCanBuild = true;

    for (auto Building : ExistingBuildings)
    {
        if (!Building)
            continue;

        if (Building->K2_GetActorLocation() == BuildingActor->K2_GetActorLocation() && Building->BuildingType == BuildingActor->BuildingType)
        {
            bCanBuild = false;
        }
    }
    
    if (bCanBuild || ExistingBuildings.size() == 0)
    {
        ExistingBuildings.push_back(BuildingActor);

        return true;
    }

    return false;
}

bool IsCurrentlyDisconnecting(SDK::UNetConnection* Connection)
{
    if (!Connection || IsBadReadPtr(Connection, sizeof(Connection))) return true;
    
    auto PC = (SDK::AFortPlayerController*)Connection->PlayerController;
    if (!PC || IsBadReadPtr(PC, sizeof(PC))) return true;
    
    return PC->bIsDisconnecting;
}

void SwapPlayerControllers(SDK::APlayerController* OldPC, SDK::APlayerController* NewPC)
{
    if (OldPC && NewPC && OldPC->Player)
    {
        NewPC->NetPlayerIndex = OldPC->NetPlayerIndex;
        NewPC->NetConnection = OldPC->NetConnection;

        GetWorld()->AuthorityGameMode->K2_OnSwapPlayerControllers(OldPC, NewPC);

        OldPC->PendingSwapConnection = reinterpret_cast<SDK::UNetConnection*>(OldPC->Player);
    }
}

void Spectate(SDK::UNetConnection* SpectatingConnection, SDK::AFortPlayerStateAthena* StateToSpectate)
{
    if (!SpectatingConnection || !StateToSpectate)
        return;

    auto PawnToSpectate = StateToSpectate->GetCurrentPawn();
    auto DeadPC = static_cast<SDK::AFortPlayerControllerAthena*>(SpectatingConnection->PlayerController);

    if (!DeadPC)
        return;

    auto DeadPlayerState = static_cast<SDK::AFortPlayerStateAthena*>(DeadPC->PlayerState);

    if (!IsCurrentlyDisconnecting(SpectatingConnection) && DeadPlayerState && PawnToSpectate)
    {
        //ABP_SpectatorPC_C and ABP_SpectatorPawn_C stuff are for live spectating not for normal spectating.
    }
}

inline void DumpObjects()
{
    std::ofstream objects("ObjectsDump.txt");

    if (objects)
    {
        for (int i = 0; i < SDK::UObject::GObjects->Num(); i++)
        {
            auto Object = SDK::UObject::GObjects->GetByIndex(i);

            if (!Object)
                continue;

            objects << '[' + std::to_string(Object->InternalIndex) + "] " + Object->GetFullName() << '\n';
        }
    }

    objects.close();

    std::cout << "Finished dumping objects!\n";
}

static bool KickController(SDK::APlayerController* PC, SDK::FString Message)
{
    if (PC && Message.Data)
    {
        SDK::FText text = reinterpret_cast<SDK::UKismetTextLibrary*>(SDK::UKismetTextLibrary::StaticClass())->STATIC_Conv_StringToText(Message);
        return Native::OnlineSession::KickPlayer(GetWorld()->AuthorityGameMode->GameSession, PC, text);
    }

    return false;
}

// template <typename SoftType>
SDK::UObject* SoftObjectToObject(SDK::TSoftObjectPtr<SDK::UObject*> SoftPtr)
{
    static auto KismetSystem = GetKismetSystem();
    static auto fn = SDK::UObject::FindObject<SDK::UFunction>("Function Engine.KismetSystemLibrary.Conv_SoftClassReferenceToClass");

    struct
    {
        SDK::TSoftObjectPtr<SDK::UObject*> SoftClassReference;
        SDK::UObject* Class;
    } params;

    params.SoftClassReference = SoftPtr;

    auto flags = fn->FunctionFlags;
    fn->FunctionFlags |= 0x400;

    ProcessEvent(KismetSystem, fn, &params);

    fn->FunctionFlags = flags;

    return params.Class;
}

auto GetAllActorsOfClass(SDK::UClass* Class)
{
    // You have to free this!!!
    SDK::TArray<SDK::AActor*> OutActors;

    static auto GameplayStatics = static_cast<SDK::UGameplayStatics*>(SDK::UGameplayStatics::StaticClass()->CreateDefaultObject());
    GameplayStatics->STATIC_GetAllActorsOfClass(GetWorld(), Class, &OutActors);

    return OutActors;
}

SDK::FTransform GetPlayerStart(SDK::AFortPlayerControllerAthena* PC)
{
    SDK::TArray<SDK::AActor*> OutActors = GetAllActorsOfClass(SDK::AFortPlayerStartWarmup::StaticClass());

    auto ActorsNum = OutActors.Num();

    auto SpawnTransform = SDK::FTransform();
    SpawnTransform.Scale3D = SDK::FVector(1, 1, 1);
    SpawnTransform.Rotation = SDK::FQuat();
    SpawnTransform.Translation = SDK::FVector { 1250, 1818, 3284 }; // Next to salty

    auto GamePhase = static_cast<SDK::AAthena_GameState_C*>(GetWorld()->GameState)->GamePhase;

    if (ActorsNum != 0 && (GamePhase == SDK::EAthenaGamePhase::Setup || GamePhase == SDK::EAthenaGamePhase::Warmup))
    {
        auto ActorToUseNum = Utils::RandomIntInRange(0, ActorsNum);
        auto ActorToUse = (OutActors)[ActorToUseNum];

        while (!ActorToUse)
        {
            ActorToUseNum = Utils::RandomIntInRange(0, ActorsNum);
            ActorToUse = (OutActors)[ActorToUseNum];
        }

        SpawnTransform.Translation = ActorToUse->K2_GetActorLocation();

        PC->WarmupPlayerStart = static_cast<SDK::AFortPlayerStartWarmup*>(ActorToUse);
    }

    OutActors.FreeArray();

    return SpawnTransform;
}

inline SDK::UKismetMathLibrary* GetMath()
{
    return (SDK::UKismetMathLibrary*)SDK::UKismetMathLibrary::StaticClass();
}

SDK::FVector RotToVec(const SDK::FRotator& Rotator)
{
    float CP, SP, CY, SY;
    Utils::sinCos(&SP, &CP, GetMath()->STATIC_DegreesToRadians(Rotator.Pitch));
    Utils::sinCos(&SY, &CY, GetMath()->STATIC_DegreesToRadians(Rotator.Yaw));
    auto V = SDK::FVector(CP * CY, CP * SY, SP);

    return V;
}

SDK::UFortWeaponRangedItemDefinition* FindWID(const std::string& WID)
{
    auto Def = SDK::UObject::FindObject<SDK::UFortWeaponRangedItemDefinition>("FortWeaponRangedItemDefinition " + WID + '.' + WID);

    if (!Def)
    {
        Def = SDK::UObject::FindObject<SDK::UFortWeaponRangedItemDefinition>("WID_Harvest_" + WID + "_Athena_C_T01" + ".WID_Harvest_" + WID + "_Athena_C_T01");

        if (!Def)
            Def = SDK::UObject::FindObject<SDK::UFortWeaponRangedItemDefinition>(WID + "." + WID);
    }

    return Def;
}

auto GetRandomWID(int skip = 0)
{
    if (skip == 0)
        skip = Utils::RandomIntInRange(4, 100);

    return SDK::UObject::FindObject<SDK::UFortWeaponRangedItemDefinition>("FortWeaponRangedItemDefinition WID_", skip);
}