#pragma once

namespace Hooks
{
    void InitNetworkHooks();
}

#include "imgui/imgui.h"
#include "imgui/imgui_impl_dx9.h"
#include "imgui/imgui_impl_win32.h"
#include <d3d9.h>
#include <tchar.h>

#pragma comment(lib, "d3d9.lib")

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

namespace GUI
{
    void Tick()
    {
        // 1. Show the big demo window (Most of the sample code is in ImGui::ShowDemoWindow()! You can browse its code to learn more about Dear ImGui!).
        ImGui::ShowDemoWindow();

        if (!bTraveled)
        {
            ImGui::Begin("Setup");
            
            if (ImGui::Button("Start"))
            {
                Game::Mode = std::make_unique<GameModeSolos>();
                LOG_INFO("Initializing the game.");
                Game::Start();
                LOG_INFO("Initializing Network Hooks.");
                Hooks::InitNetworkHooks();
            }

            ImGui::End();
            return;
        }

        if (ImGui::Begin("Raider"))
        {
            if (ImGui::BeginTabBar("Raider Tabs"))
            {
                if (ImGui::BeginTabItem("Game"))
                {
                    if (ImGui::Button("Start Bus"))
                    {
                        if (static_cast<SDK::AAthena_GameState_C*>(GetWorld()->GameState)->GamePhase >= SDK::EAthenaGamePhase::Aircraft)
                        {
                            LOG_INFO("The bus has already started!")
                            bStartedBus = true;
                        }

                        GetKismetSystem()->STATIC_ExecuteConsoleCommand(GetWorld(), L"startaircraft", nullptr);

                        LOG_INFO("The bus has been started!")
                        bStartedBus = true;
                    }
                    ImGui::EndTabItem();
                }
                if (ImGui::BeginTabItem("Dev"))
                {
                    if (ImGui::Button("Dump Objects"))
                    {
                        DumpObjects();
                    }
                    if (ImGui::Button("Init loot"))
                    {
                        Game::Mode->InitLoot();
                    }
                    ImGui::EndTabItem();
                }
                if (ImGui::BeginTabItem("Event"))
                {
                    static SDK::ABP_Athena_Event_Components_C* EventComponent = nullptr;

                    if (ImGui::Button("Spawn Meteor"))
                    {
                        if (EventComponent == nullptr)
                        {
                            SDK::UObject::FindObject<SDK::ABP_Athena_Event_Components_C>("BP_Athena_Event_Components_C Athena_GameplayActors.Athena_GameplayActors.PersistentLevel.BP_Athena_Event_Components_54_55");
                            SDK::TArray<SDK::AActor*> EventComponents;
                            GetGameplayStatics()->STATIC_GetAllActorsOfClass(GetWorld(), SDK::ABP_Athena_Event_Components_C::StaticClass(), &EventComponents);
                            if (EventComponents.Count > 0)
                            {
                                EventComponent = (SDK::ABP_Athena_Event_Components_C*)EventComponents[0];
                            }
                        }

                        EventComponent->RandomizeSpawnLocation();
                        EventComponent->SpawnMeteor();
                    }

                    if (ImGui::Button("Spawn Shooting Stars"))
                    {
                        if (EventComponent == nullptr)
                        {
                            SDK::UObject::FindObject<SDK::ABP_Athena_Event_Components_C>("BP_Athena_Event_Components_C Athena_GameplayActors.Athena_GameplayActors.PersistentLevel.BP_Athena_Event_Components_54_55");
                            SDK::TArray<SDK::AActor*> EventComponents;
                            GetGameplayStatics()->STATIC_GetAllActorsOfClass(GetWorld(), SDK::ABP_Athena_Event_Components_C::StaticClass(), &EventComponents);
                            if (EventComponents.Count > 0)
                            {
                                EventComponent = (SDK::ABP_Athena_Event_Components_C*)EventComponents[0];
                            }
                        }

                        EventComponent->RandomizeSpawnLocation();
                        EventComponent->SpawnShootingStars();
                    }

                    static float cometpos = 0.0f;
                    ImGui::SliderFloat("Comet pos", &cometpos, 0, 1);

                    if (ImGui::Button("Update Comet"))
                    {
                        if (EventComponent == nullptr)
                        {
                            SDK::UObject::FindObject<SDK::ABP_Athena_Event_Components_C>("BP_Athena_Event_Components_C Athena_GameplayActors.Athena_GameplayActors.PersistentLevel.BP_Athena_Event_Components_54_55");
                            SDK::TArray<SDK::AActor*> EventComponents;
                            GetGameplayStatics()->STATIC_GetAllActorsOfClass(GetWorld(), SDK::ABP_Athena_Event_Components_C::StaticClass(), &EventComponents);
                            if (EventComponents.Count > 0)
                            {
                                EventComponent = (SDK::ABP_Athena_Event_Components_C*)EventComponents[0];
                            }
                        }

                        EventComponent->CometPosition = cometpos;
                        EventComponent->OnRep_CometPosition();
                    }

                    ImGui::EndTabItem();
                }
                ImGui::EndTabBar();
            }
        }
        ImGui::End();

    }

    #pragma region Backend
    static LPDIRECT3D9 g_pD3D = nullptr;
    static LPDIRECT3DDEVICE9 g_pd3dDevice = nullptr;
    static bool g_DeviceLost = false;
    static UINT g_ResizeWidth = 0, g_ResizeHeight = 0;
    static D3DPRESENT_PARAMETERS g_d3dpp = {};

    bool CreateDeviceD3D(HWND hWnd);
    void CleanupDeviceD3D();
    void ResetDevice();
    LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

    DWORD WINAPI GuiMain(LPVOID lpParam)
    {
        // Create application window
        // ImGui_ImplWin32_EnableDpiAwareness();
        WNDCLASSEXW wc = { sizeof(wc), CS_CLASSDC, WndProc, 0L, 0L, GetModuleHandle(nullptr), nullptr, nullptr, nullptr, nullptr, L"ImGui Example", nullptr };
        ::RegisterClassExW(&wc);
        HWND hwnd = ::CreateWindowW(wc.lpszClassName, L"Dear ImGui DirectX9 Example", WS_OVERLAPPEDWINDOW, 100, 100, 1280, 800, nullptr, nullptr, wc.hInstance, nullptr);

        // Initialize Direct3D
        if (!CreateDeviceD3D(hwnd))
        {
            CleanupDeviceD3D();
            ::UnregisterClassW(wc.lpszClassName, wc.hInstance);
            return 1;
        }

        // Show the window
        ::ShowWindow(hwnd, SW_SHOWDEFAULT);
        ::UpdateWindow(hwnd);

        // Setup Dear ImGui context
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO();
        (void)io;
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard; // Enable Keyboard Controls
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad; // Enable Gamepad Controls

        // Setup Dear ImGui style
        ImGui::StyleColorsDark();
        // ImGui::StyleColorsLight();

        // Setup Platform/Renderer backends
        ImGui_ImplWin32_Init(hwnd);
        ImGui_ImplDX9_Init(g_pd3dDevice);

        // Load Fonts
        // - If no fonts are loaded, dear imgui will use the default font. You can also load multiple fonts and use ImGui::PushFont()/PopFont() to select them.
        // - AddFontFromFileTTF() will return the ImFont* so you can store it if you need to select the font among multiple.
        // - If the file cannot be loaded, the function will return a nullptr. Please handle those errors in your application (e.g. use an assertion, or display an error and quit).
        // - The fonts will be rasterized at a given size (w/ oversampling) and stored into a texture when calling ImFontAtlas::Build()/GetTexDataAsXXXX(), which ImGui_ImplXXXX_NewFrame below will call.
        // - Use '#define IMGUI_ENABLE_FREETYPE' in your imconfig file to use Freetype for higher quality font rendering.
        // - Read 'docs/FONTS.md' for more instructions and details.
        // - Remember that in C/C++ if you want to include a backslash \ in a string literal you need to write a double backslash \\ !
        // io.Fonts->AddFontDefault();
        // io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\segoeui.ttf", 18.0f);
        // io.Fonts->AddFontFromFileTTF("../../misc/fonts/DroidSans.ttf", 16.0f);
        // io.Fonts->AddFontFromFileTTF("../../misc/fonts/Roboto-Medium.ttf", 16.0f);
        // io.Fonts->AddFontFromFileTTF("../../misc/fonts/Cousine-Regular.ttf", 15.0f);
        // ImFont* font = io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\ArialUni.ttf", 18.0f, nullptr, io.Fonts->GetGlyphRangesJapanese());
        // IM_ASSERT(font != nullptr);

        // Our state
        ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

        // Main loop
        bool done = false;
        while (!done)
        {
            // Poll and handle messages (inputs, window resize, etc.)
            // See the WndProc() function below for our to dispatch events to the Win32 backend.
            MSG msg;
            while (::PeekMessage(&msg, nullptr, 0U, 0U, PM_REMOVE))
            {
                ::TranslateMessage(&msg);
                ::DispatchMessage(&msg);
                if (msg.message == WM_QUIT)
                    done = true;
            }
            if (done)
                break;

            // Handle lost D3D9 device
            if (g_DeviceLost)
            {
                HRESULT hr = g_pd3dDevice->TestCooperativeLevel();
                if (hr == D3DERR_DEVICELOST)
                {
                    ::Sleep(10);
                    continue;
                }
                if (hr == D3DERR_DEVICENOTRESET)
                    ResetDevice();
                g_DeviceLost = false;
            }

            // Handle window resize (we don't resize directly in the WM_SIZE handler)
            if (g_ResizeWidth != 0 && g_ResizeHeight != 0)
            {
                g_d3dpp.BackBufferWidth = g_ResizeWidth;
                g_d3dpp.BackBufferHeight = g_ResizeHeight;
                g_ResizeWidth = g_ResizeHeight = 0;
                ResetDevice();
            }

            // Start the Dear ImGui frame
            ImGui_ImplDX9_NewFrame();
            ImGui_ImplWin32_NewFrame();
            ImGui::NewFrame();

            Tick();

            // Rendering
            ImGui::EndFrame();
            g_pd3dDevice->SetRenderState(D3DRS_ZENABLE, FALSE);
            g_pd3dDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
            g_pd3dDevice->SetRenderState(D3DRS_SCISSORTESTENABLE, FALSE);
            D3DCOLOR clear_col_dx = D3DCOLOR_RGBA((int)(clear_color.x * clear_color.w * 255.0f), (int)(clear_color.y * clear_color.w * 255.0f), (int)(clear_color.z * clear_color.w * 255.0f), (int)(clear_color.w * 255.0f));
            g_pd3dDevice->Clear(0, nullptr, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, clear_col_dx, 1.0f, 0);
            if (g_pd3dDevice->BeginScene() >= 0)
            {
                ImGui::Render();
                ImGui_ImplDX9_RenderDrawData(ImGui::GetDrawData());
                g_pd3dDevice->EndScene();
            }
            HRESULT result = g_pd3dDevice->Present(nullptr, nullptr, nullptr, nullptr);
            if (result == D3DERR_DEVICELOST)
                g_DeviceLost = true;
        }

        // Cleanup
        ImGui_ImplDX9_Shutdown();
        ImGui_ImplWin32_Shutdown();
        ImGui::DestroyContext();

        CleanupDeviceD3D();
        ::DestroyWindow(hwnd);
        ::UnregisterClassW(wc.lpszClassName, wc.hInstance);

        return 0;
    }

    // Helper functions

    bool CreateDeviceD3D(HWND hWnd)
    {
        if ((g_pD3D = Direct3DCreate9(D3D_SDK_VERSION)) == nullptr)
            return false;

        // Create the D3DDevice
        ZeroMemory(&g_d3dpp, sizeof(g_d3dpp));
        g_d3dpp.Windowed = TRUE;
        g_d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
        g_d3dpp.BackBufferFormat = D3DFMT_UNKNOWN; // Need to use an explicit format with alpha if needing per-pixel alpha composition.
        g_d3dpp.EnableAutoDepthStencil = TRUE;
        g_d3dpp.AutoDepthStencilFormat = D3DFMT_D16;
        g_d3dpp.PresentationInterval = D3DPRESENT_INTERVAL_ONE; // Present with vsync
        // g_d3dpp.PresentationInterval = D3DPRESENT_INTERVAL_IMMEDIATE;   // Present without vsync, maximum unthrottled framerate
        if (g_pD3D->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, hWnd, D3DCREATE_HARDWARE_VERTEXPROCESSING, &g_d3dpp, &g_pd3dDevice) < 0)
            return false;

        return true;
    }

    void CleanupDeviceD3D()
    {
        if (g_pd3dDevice)
        {
            g_pd3dDevice->Release();
            g_pd3dDevice = nullptr;
        }
        if (g_pD3D)
        {
            g_pD3D->Release();
            g_pD3D = nullptr;
        }
    }

    void ResetDevice()
    {
        ImGui_ImplDX9_InvalidateDeviceObjects();
        HRESULT hr = g_pd3dDevice->Reset(&g_d3dpp);
        if (hr == D3DERR_INVALIDCALL)
            IM_ASSERT(0);
        ImGui_ImplDX9_CreateDeviceObjects();
    }

    // Forward declare message handler from imgui_impl_win32.cpp

    // Win32 message handler
    // You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
    // - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application, or clear/overwrite your copy of the mouse data.
    // - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application, or clear/overwrite your copy of the keyboard data.
    // Generally you may always pass all inputs to dear imgui, and hide them from your application based on those two flags.
    LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
    {
        if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
            return true;

        switch (msg)
        {
        case WM_SIZE:
            if (wParam == SIZE_MINIMIZED)
                return 0;
            g_ResizeWidth = (UINT)LOWORD(lParam); // Queue resize
            g_ResizeHeight = (UINT)HIWORD(lParam);
            return 0;
        case WM_SYSCOMMAND:
            if ((wParam & 0xfff0) == SC_KEYMENU) // Disable ALT application menu
                return 0;
            break;
        case WM_DESTROY:
            ::PostQuitMessage(0);
            return 0;
        }
        return ::DefWindowProcW(hWnd, msg, wParam, lParam);
    }
    #pragma endregion

}

#if 0

#include "ZeroGUI.h"
#include <format>
#include <mutex>

#define REGISTER_MODE(GM)                                     \
    if (ZeroGUI::Button(L## #GM, { 150.0f, 25.0f })) \
    {                                                         \
        Game::Mode = std::make_unique<GM>(); \
        LOG_INFO("Initializing the game.");          \
        Game::Start();                               \
                                                        \
        LOG_INFO("Initializing Network Hooks.");     \
        Hooks::InitNetworkHooks();                   \
    }


namespace GUI
{
    std::mutex mtx;
    void Tick()
    {
        ZeroGUI::Input::Handle();

        static bool menu_opened = true;

        if (GetAsyncKeyState(VK_F2) & 1)
            menu_opened = !menu_opened;

        static auto pos = SDK::FVector2D { 200.f, 250.0f };
        static auto setuppos = SDK::FVector2D { 200.f, 250.0f };

        if (ZeroGUI::Window(L"Setup", &setuppos, SDK::FVector2D { 500.0f, 700.0f }, menu_opened && !bTraveled))
        {
            REGISTER_MODE(GameModeSolos)
            REGISTER_MODE(GameModeDuos)
            REGISTER_MODE(GameModePlayground)
            REGISTER_MODE(GameModeLateGame)
            REGISTER_MODE(GameMode50v50)
            REGISTER_MODE(GameModePlaygroundV2)
            REGISTER_MODE(GameModeSolosNoBuild)
        }

        if (ZeroGUI::Window(L"Raider", &pos, SDK::FVector2D { 500.0f, 700.0f }, menu_opened && bTraveled))
        {
            if (bListening && HostBeacon)
            {
                static auto GameState = reinterpret_cast<SDK::AAthena_GameState_C*>(GetWorld()->GameState);
                static SDK::APlayerState* currentPlayer = nullptr;

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
                        KickController((SDK::APlayerController*)currentPlayer->Owner, L"You have been kicked by the server.");

                        mtx.lock();
                        currentPlayer = nullptr;
                        mtx.unlock();
                    }

                    if (ZeroGUI::Button(L"Dump inv", { 60.0f, 25.0f }))
                    {
                        Inventory::DumpInventory((SDK::AFortPlayerControllerAthena*)currentPlayer->Owner);
                    }

                    if (ZeroGUI::Button(L"Empty inv", { 60.0f, 25.0f }))
                    {
                        Inventory::EmptyInventory((SDK::AFortPlayerControllerAthena*)currentPlayer->Owner);
                    }
                }
                else
                {
                    static int tab = 0;
                    if (ZeroGUI::ButtonTab(L"Game", SDK::FVector2D { 110, 25 }, tab == 0))
                        tab = 0;
                    if (ZeroGUI::ButtonTab(L"Players", SDK::FVector2D { 110, 25 }, tab == 1))
                        tab = 1;
                    if (ZeroGUI::ButtonTab(L"Dev", SDK::FVector2D { 110, 25 }, tab == 2))
                        tab = 2;
                    if (ZeroGUI::ButtonTab(L"Event", SDK::FVector2D { 110, 25 }, tab == 3))
                        tab = 3;

                    ZeroGUI::NextColumn(130.0f);

                    switch (tab)
                    {
                    case 0:
                    {
                        if (!bStartedBus)
                        {
                            if (ZeroGUI::Button(L"Start Bus", SDK::FVector2D { 100, 25 }))
                            {
                                if (static_cast<SDK::AAthena_GameState_C*>(GetWorld()->GameState)->GamePhase >= SDK::EAthenaGamePhase::Aircraft)
                                {
                                    LOG_INFO("The bus has already started!")
                                    bStartedBus = true;
                                }

                                //GameState->bGameModeWillSkipAircraft = false;
                                //GameState->AircraftStartTime = 0;
                                //GameState->WarmupCountdownEndTime = 0;

                                GetKismetSystem()->STATIC_ExecuteConsoleCommand(GetWorld(), L"startaircraft", nullptr);

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
                        if (ZeroGUI::Button(L"Dump Objects", SDK::FVector2D { 100, 25 }))
                        {
                            DumpObjects();
                        }
                        if (ZeroGUI::Button(L"Init loot", SDK::FVector2D { 100, 25 }))
                        {
                            Game::Mode->InitLoot();
                        }
                        break;
                    }
                    case 3:
                    {

                        static SDK::ABP_Athena_Event_Components_C* EventComponent = nullptr;

                        if (ZeroGUI::Button(L"Spawn Meteor", SDK::FVector2D { 100, 25 }))
                        {
                            if (EventComponent == nullptr)
                            {
                                SDK::UObject::FindObject<SDK::ABP_Athena_Event_Components_C>("BP_Athena_Event_Components_C Athena_GameplayActors.Athena_GameplayActors.PersistentLevel.BP_Athena_Event_Components_54_55");
                                SDK::TArray<SDK::AActor*> EventComponents;
                                GetGameplayStatics()->STATIC_GetAllActorsOfClass(GetWorld(), SDK::ABP_Athena_Event_Components_C::StaticClass(), &EventComponents);
                                if (EventComponents.Count > 0)
                                {
                                    EventComponent = (SDK::ABP_Athena_Event_Components_C*)EventComponents[0];
                                }
                            }

                            EventComponent->RandomizeSpawnLocation();
                            EventComponent->SpawnMeteor();
                        }

                        if (ZeroGUI::Button(L"Spawn Shooting Stars", SDK::FVector2D { 100, 25 }))
                        {
                            if (EventComponent == nullptr)
                            {
                                SDK::UObject::FindObject<SDK::ABP_Athena_Event_Components_C>("BP_Athena_Event_Components_C Athena_GameplayActors.Athena_GameplayActors.PersistentLevel.BP_Athena_Event_Components_54_55");
                                SDK::TArray<SDK::AActor*> EventComponents;
                                GetGameplayStatics()->STATIC_GetAllActorsOfClass(GetWorld(), SDK::ABP_Athena_Event_Components_C::StaticClass(), &EventComponents);
                                if (EventComponents.Count > 0)
                                {
                                    EventComponent = (SDK::ABP_Athena_Event_Components_C*)EventComponents[0];
                                }
                            }

                            EventComponent->RandomizeSpawnLocation();
                            EventComponent->SpawnShootingStars();
                        }

                        static float cometpos = 0.0f;
                        ZeroGUI::SliderFloat(L"Comet pos", &cometpos, 0, 1, L"%.02f");

                        if (ZeroGUI::Button(L"Update Comet", SDK::FVector2D { 100, 25 }))
                        {
                            if (EventComponent == nullptr)
                            {
                                SDK::UObject::FindObject<SDK::ABP_Athena_Event_Components_C>("BP_Athena_Event_Components_C Athena_GameplayActors.Athena_GameplayActors.PersistentLevel.BP_Athena_Event_Components_54_55");
                                SDK::TArray<SDK::AActor*> EventComponents;
                                GetGameplayStatics()->STATIC_GetAllActorsOfClass(GetWorld(), SDK::ABP_Athena_Event_Components_C::StaticClass(), &EventComponents);
                                if (EventComponents.Count > 0)
                                {
                                    EventComponent = (SDK::ABP_Athena_Event_Components_C*)EventComponents[0];
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
#endif