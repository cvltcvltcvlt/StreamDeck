#include <windows.h>
#include <mmsystem.h>
#include <tchar.h>
#include "imgui.h"
#define IMGUI_DEFINE_MATH_OPERATORS
#include "imgui_impl_dx9.h"
#include "imgui_impl_win32.h"
#include <d3d9.h>
#include "Driver.hpp"
#include <thread>
#include <atomic>
#include <string>
#include <iostream>
#include <fstream>
#include <map>
#include "style.hpp"

#pragma comment(lib, "winmm.lib")

DriverHandler::Buttons _driver;

// Data
static LPDIRECT3D9              g_pD3D = nullptr;
static LPDIRECT3DDEVICE9        g_pd3dDevice = nullptr;
static bool                     g_DeviceLost = false;
static UINT                     g_ResizeWidth = 0, g_ResizeHeight = 0;
static D3DPRESENT_PARAMETERS    g_d3dpp = {};
static std::atomic<bool>        driver_exit(false);
static std::thread              driver_thread; // Thread handle

// Forward declarations of helper functions
bool CreateDeviceD3D(HWND hWnd);
void CleanupDeviceD3D();
void ResetDevice();
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
void DriverThreadFunc();
static char path[256] = "";
static char key[256] = "";
std::string text = "";
static char function[256] = "";
static std::map<int, std::pair<std::string, std::string>> settings;

// INI file handling
void LoadSettings(const std::string& filename);
void SaveSettings(const std::string& filename);

// Main code
int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    (void)hInstance;
    (void)hPrevInstance;
    (void)lpCmdLine;
    (void)nCmdShow;

    // Load settings from ini file
    LoadSettings("settings.ini");

    // Create application window
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
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.Fonts->AddFontFromFileTTF("Fonts/Roboto-Regular.ttf", 16.0f);
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();

    // Setup Platform/Renderer backends
    ImGui_ImplWin32_Init(hwnd);
    ImGui_ImplDX9_Init(g_pd3dDevice);

    // Start driver thread
    driver_exit = false;
    driver_thread = std::thread(DriverThreadFunc);

    // Our state
    bool show_demo_window = false;
    bool show_another_window = false;
    bool show_main_window = false;
    bool show_videos_window = false;
    bool show_sound_window = false;
    bool show_sound_setup1 = false;
    bool show_sound_setup2 = false;
    bool show_sound_setup3 = false;
    bool show_main_button_window = true;
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

        // Start the Dear ImGui frame
        ImGui_ImplDX9_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();
        ButtonStyle::SetStyle();

        // 1. Show the big demo window (Most of the sample code is in ImGui::ShowDemoWindow()! You can browse its code to learn more about Dear ImGui!).
        if (show_demo_window)
            ImGui::ShowDemoWindow(&show_demo_window);

        // 2. Show a simple window that we create ourselves. We use a Begin/End pair to create a named window.
        {
            if (show_main_button_window)
            {
                ImGui::Begin("Main window");
                ImGui::Text("StreamDeck");
                if (ImGui::Button("Button 1 settings"))
                {
                    strncpy(key, "1", sizeof(key));
                    strncpy(path, settings[1].second.c_str(), sizeof(path));
                    strncpy(function, settings[1].first.c_str(), sizeof(function));
                    show_main_window = true;
                    show_main_button_window = false;
                }
                if (ImGui::Button("Button 2 settings"))
                {
                    strncpy(key, "2", sizeof(key));
                    strncpy(path, settings[2].second.c_str(), sizeof(path));
                    strncpy(function, settings[2].first.c_str(), sizeof(function));
                    show_main_window = true;
                    show_main_button_window = false;
                }
                if (ImGui::Button("Button 3 settings"))
                {
                    strncpy(key, "3", sizeof(key));
                    strncpy(path, settings[3].second.c_str(), sizeof(path));
                    strncpy(function, settings[3].first.c_str(), sizeof(function));
                    show_main_window = true;
                    show_main_button_window = false;
                }
                ImGui::End();
            }

            if (show_main_window)
            {
                ImGui::Begin("Settings", &show_main_window);
                ImGui::InputText("Key", key, IM_ARRAYSIZE(key));  // Create a text input field labeled "Key"
                ImGui::InputText("URL or Sound Path", path, IM_ARRAYSIZE(path));  // Create a text input field labeled "URL or Sound Path"
                ImGui::InputText("Function", function, IM_ARRAYSIZE(function));  // Create a text input field labeled "Function"
                if (ImGui::Button("Save"))
                {
                    if (key == NULL || path == NULL || function == NULL || key[0] == '\0' || path[0] == '\0' || function[0] == '\0')
                    {
                        ImGui::OpenPopup("Error");
                    }
                    else
                    {
                        try
                        {
                            int checkKey = std::stoi(key);
                            settings[checkKey] = std::make_pair(function, path);  // Save the settings
                            SaveSettings("settings.ini");  // Save settings to ini file
                            key[0] = '\0';  // Clear the key input field
                            path[0] = '\0';  // Clear the URL or sound path input field
                            function[0] = '\0';  // Clear the function input field
                        }
                        catch (const std::invalid_argument& e)
                        {
                            ImGui::OpenPopup("Error");
                        }
                    }
                }
                if (ImGui::BeginPopupModal("Error", NULL, ImGuiWindowFlags_AlwaysAutoResize))
                {
                    ImGui::Text("Invalid input. Please try again.");
                    if (ImGui::Button("OK"))
                    {
                        ImGui::CloseCurrentPopup();
                    }
                    ImGui::EndPopup();
                }
                ImGui::Text(text.c_str());
                if (ImGui::Button("Close Me"))
                {
                    show_main_window = false;
                    show_main_button_window = true;
                }
                ImGui::End();
            }
            if (show_sound_window)
            {
                ImGui::Begin("Sounds");
                if (ImGui::Button("Sound 1"))
                {
                    show_sound_setup1 = true;
                }
                if (ImGui::Button("Sound 2"))
                {
                    show_sound_setup2 = true;
                }
                if (ImGui::Button("Sound 3"))
                {
                    show_sound_setup3 = true;
                }
                if (ImGui::Button("Go Back"))
                {
                    show_sound_window = false;
                    show_main_window = true;
                }
                if (show_sound_setup1)
                {
                    ImGui::Begin("Sound set up 1", &show_sound_setup1);
                    if (ImGui::Button("Play"))
                    {
                        std::string lmao = "1";
                        std::thread soundThread([lmao]() {
                            _driver.playSound(lmao);
                            });
                        soundThread.detach();
                    }
                    if (ImGui::Button("Save"))
                    {
                        show_sound_setup1 = false;
                    }
                    ImGui::End();
                }

                // Sound setup 2 window
                if (show_sound_setup2)
                {
                    ImGui::Begin("Sound set up 2", &show_sound_setup2);
                    if (ImGui::Button("Save"))
                    {
                        show_sound_setup2 = false;
                    }
                    ImGui::End();
                }

                // Sound setup 3 window
                if (show_sound_setup3)
                {
                    ImGui::Begin("Sound set up 3", &show_sound_setup3);
                    if (ImGui::Button("Save"))
                    {
                        show_sound_setup3 = false;
                    }
                    ImGui::End();
                }
                ImGui::End();
            }
        }
        ButtonStyle::ResetStyle();

        // 3. Show another simple window.
        if (show_another_window)
        {
            ImGui::Begin("Another Window", &show_another_window);   // Pass a pointer to our bool variable (the window will have a closing button that will clear the bool when clicked)
            ImGui::Text("Hello from another window!");
            if (ImGui::Button("Close Me"))
                show_another_window = false;
            show_main_window = true;
            ImGui::End();
        }

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

        // Handle loss of D3D9 device
        if (result == D3DERR_DEVICELOST && g_pd3dDevice->TestCooperativeLevel() == D3DERR_DEVICENOTRESET)
            g_DeviceLost = true;
    }

    // Cleanup
    driver_exit = true;
    driver_thread.join();
    ImGui_ImplDX9_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();

    CleanupDeviceD3D();
    ::DestroyWindow(hwnd);
    ::UnregisterClassW(wc.lpszClassName, wc.hInstance);

    return 0;
}

void DriverThreadFunc()
{
    while (!driver_exit)
    {
        _driver.setMusic(key, path);
        _driver.handle();
        text = _driver.getOutput();
        std::this_thread::sleep_for(std::chrono::milliseconds(10)); // Sleep to prevent tight loop
    }
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
    g_d3dpp.PresentationInterval = D3DPRESENT_INTERVAL_ONE;           // Present with vsync
    //g_d3dpp.PresentationInterval = D3DPRESENT_INTERVAL_IMMEDIATE;   // Present without vsync, maximum unthrottled framerate
    if (g_pD3D->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, hWnd, D3DCREATE_HARDWARE_VERTEXPROCESSING, &g_d3dpp, &g_pd3dDevice) < 0)
        return false;

    return true;
}

void CleanupDeviceD3D()
{
    if (g_pd3dDevice) { g_pd3dDevice->Release(); g_pd3dDevice = nullptr; }
    if (g_pD3D) { g_pD3D->Release(); g_pD3D = nullptr; }
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
extern LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

// Win32 message handler
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
        return true;

    switch (msg)
    {
    case WM_SIZE:
        if (g_pd3dDevice != nullptr && wParam != SIZE_MINIMIZED)
        {
            g_ResizeWidth = (UINT)LOWORD(lParam);
            g_ResizeHeight = (UINT)HIWORD(lParam);
            g_d3dpp.BackBufferWidth = (UINT)LOWORD(lParam);
            g_d3dpp.BackBufferHeight = (UINT)HIWORD(lParam);
            ResetDevice();
            g_DeviceLost = false;
        }
        return 0;
    case WM_SYSCOMMAND:
        if ((wParam & 0xfff0) == SC_KEYMENU) // Disable ALT application menu
            return 0;
        break;
    case WM_DESTROY:
        ::PostQuitMessage(0);
        return 0;
    case WM_DEVICECHANGE:
        if (g_pd3dDevice != nullptr && g_ResizeWidth != 0 && g_ResizeHeight != 0)
        {
            g_d3dpp.BackBufferWidth = g_ResizeWidth;
            g_d3dpp.BackBufferHeight = g_ResizeHeight;
            ResetDevice();
            g_DeviceLost = false;
        }
        break;
    }
    return ::DefWindowProc(hWnd, msg, wParam, lParam);
}

// INI file handling
void LoadSettings(const std::string& filename)
{
    std::ifstream file(filename);
    if (file.is_open())
    {
        std::string line;
        while (std::getline(file, line))
        {
            if (line.empty() || line[0] == '[' || line[0] == ';')
                continue;

            size_t pos = line.find('=');
            if (pos == std::string::npos)
                continue;

            int key = std::stoi(line.substr(0, pos));
            size_t funcPos = line.find(':', pos + 1);
            if (funcPos == std::string::npos)
                continue;

            std::string function = line.substr(pos + 1, funcPos - pos - 1);
            std::string path = line.substr(funcPos + 1);
            settings[key] = std::make_pair(function, path);
        }
        file.close();
    }
}

void SaveSettings(const std::string& filename)
{
    std::ofstream file(filename);
    if (file.is_open())
    {
        for (const auto& setting : settings)
        {
            file << setting.first << '=' << setting.second.first << ':' << setting.second.second << '\n';
        }
        file.close();
    }
}
