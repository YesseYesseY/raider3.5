#pragma once

#include "SDK.hpp"
#include "Logic/Game.h"
#include "hooks.h"
#include "Logger.hpp"
#include "ufunctionhooks.h"

DWORD WINAPI Main(LPVOID lpParam)
{
    AllocConsole();

    FILE* File;

    freopen_s(&File, "redirect.log", "w", stdout);

    raider::utils::Logger::Initialize();

    LOG_INFO("Welcome to Raider!");
    LOG_INFO("Initializing hooks!");

    Native::InitializeAll();
    UFunctionHooks::Initialize();

    DETOUR_START
    DetourAttachE(Native::NetDriver::TickFlush, Hooks::TickFlush);
    DetourAttachE(Native::LocalPlayer::SpawnPlayActor, Hooks::LocalPlayerSpawnPlayActor);

    auto Address = Utils::FindPattern(Patterns::NetDebug);
    CheckNullFatal(Address, "Failed to find NetDebug");
    void* (*NetDebug)(void* _this) = nullptr;
    AddressToFunction(Address, NetDebug);

    DetourAttachE(NetDebug, Hooks::NetDebug);
    DetourAttachE(ProcessEvent, Hooks::ProcessEventHook);
    DetourAttachE(Native::PlayerController::GetPlayerViewPoint, Hooks::GetPlayerViewPoint);
    DetourAttachE(Native::Inventory::ReloadThing, Hooks::ReloadThing);
    DETOUR_END

    LOG_INFO("Base Address: {:X}", Imagebase);

    //GetKismetSystem()->STATIC_ExecuteConsoleCommand(GetWorld(), L"log LogFortReplicationGraph VeryVerbose", GetPlayerController());
    //GetKismetSystem()->STATIC_ExecuteConsoleCommand(GetWorld(), L"log LogReplicationGraph VeryVerbose", GetPlayerController());

    CreateConsole();

    return 0;
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD dwReason, LPVOID lpReserved)
{
    if (dwReason == DLL_PROCESS_ATTACH)
        CreateThread(nullptr, 0, Main, hModule, 0, nullptr);

    return true;
}
