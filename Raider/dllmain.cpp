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
    freopen_s(&File, "CONOUT$", "w", stdout);

    
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
    DetourAttachE(Native::GameViewportClient::PostRender, Hooks::PostRender);
    DETOUR_END

    LOG_INFO("Base Address: {:X}", Imagebase);

    //GetKismetSystem()->STATIC_ExecuteConsoleCommand(GetWorld(), L"log LogFortReplicationGraph VeryVerbose", GetPlayerController());
    //GetKismetSystem()->STATIC_ExecuteConsoleCommand(GetWorld(), L"log LogReplicationGraph VeryVerbose", GetPlayerController());

    CreateConsole();

    CreateThread(nullptr, 0, GUI::GuiMain, 0, 0, nullptr);

    return 0;
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD dwReason, LPVOID lpReserved)
{
    if (dwReason == DLL_PROCESS_ATTACH)
        CreateThread(nullptr, 0, Main, hModule, 0, nullptr);

    return true;
}
