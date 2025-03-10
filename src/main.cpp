#include <cmath>
#define UNICODE
#define _UNICODE

#include "appfont.h"
#include "gui.h"
#include "imgui.h"
#include "imgui_impl_dx11.h"
#include "imgui_impl_win32.h"
#include <d3d11.h>
#include <iostream>
#include <stdint.h>
#include <tchar.h>
#include <windows.h>

extern "C" {
#include <hidsdi.h>
#include <hidusage.h>
}

// the goal FPS for both the GUI and the message handlers (will impact
// keypresses)
const DWORD GOAL_FPS = 60;

// Data
static ID3D11Device *g_pd3dDevice = nullptr;
static ID3D11DeviceContext *g_pd3dDeviceContext = nullptr;
static IDXGISwapChain *g_pSwapChain = nullptr;
static bool g_SwapChainOccluded = false;
static UINT g_ResizeWidth = 0, g_ResizeHeight = 0;
static ID3D11RenderTargetView *g_mainRenderTargetView = nullptr;

// Forward declarations of helper functions
bool CreateDeviceD3D(HWND hWnd);
void CleanupDeviceD3D();
void CreateRenderTarget();
void CleanupRenderTarget();
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

// Main code
int main(int, char **) {
    // Create application window
    // ImGui_ImplWin32_EnableDpiAwareness();
    WNDCLASSEXW wc = {sizeof(wc),
                      CS_CLASSDC,
                      WndProc,
                      0L,
                      0L,
                      GetModuleHandleW(nullptr),
                      nullptr,
                      nullptr,
                      nullptr,
                      nullptr,
                      L"keyboardAttachWindow",
                      nullptr};
    ::RegisterClassExW(&wc);
    HWND hwnd = ::CreateWindowW(
        wc.lpszClassName, L"Attach Keyboards to a Window", WS_OVERLAPPEDWINDOW,
        100, 100, 1280, 800, nullptr, nullptr, wc.hInstance, nullptr);

    if (!hwnd) {
        MessageBoxW(nullptr, L"Failed to create window", L"Error",
                    MB_OK | MB_ICONERROR);
        return 1;
    }

    // https://devblogs.microsoft.com/oldnewthing/20160627-00/?p=93755
    // https://learn.microsoft.com/en-us/windows/win32/inputdev/raw-input?redirectedfrom=MSDN
    RAWINPUTDEVICE rdev;
    rdev.usUsagePage = HID_USAGE_PAGE_GENERIC;
    rdev.usUsage = HID_USAGE_GENERIC_KEYBOARD;
    rdev.dwFlags = RIDEV_INPUTSINK | RIDEV_NOHOTKEYS;
    rdev.hwndTarget = hwnd;
    if (!RegisterRawInputDevices(&rdev, 1, sizeof(rdev))) {
        MessageBoxW(nullptr, L"Failed to attach raw input device", L"Error!",
                    MB_OK | MB_ICONERROR);
        UnregisterClassW(wc.lpszClassName, wc.hInstance);
        return 1;
    }

    // Initialize Direct3D
    if (!CreateDeviceD3D(hwnd)) {
        MessageBoxW(nullptr, L"Failed to initialize Direct3D", L"Error!",
                    MB_OK | MB_ICONERROR);

        CleanupDeviceD3D();

        UnregisterClassW(wc.lpszClassName, wc.hInstance);
        return 1;
    }

    // Show the window
    ::ShowWindow(hwnd, SW_SHOWDEFAULT);
    ::UpdateWindow(hwnd);
    //
    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    AppGui *gui = new AppGui();

    // Setup Platform/Renderer backends
    ImGui_ImplWin32_Init(hwnd);
    ImGui_ImplDX11_Init(g_pd3dDevice, g_pd3dDeviceContext);

    // Main loop
    bool done = false;
    DWORD last_time = GetTickCount();
    DWORD now;

    while (!done) {

        // Poll and handle messages (inputs, window resize, etc.)
        // See the WndProc() function below for our to dispatch events to the
        // Win32 backend.
        MSG msg;
        while (::PeekMessageW(&msg, nullptr, 0U, 0U, PM_REMOVE)) {
            ::TranslateMessage(&msg);
            ::DispatchMessageW(&msg);
            if (msg.message == WM_QUIT)
                done = true;
        }
        if (done)
            break;

        // Handle window being minimized or screen locked
        if (g_SwapChainOccluded &&
            g_pSwapChain->Present(0, DXGI_PRESENT_TEST) ==
                DXGI_STATUS_OCCLUDED) {
            ::Sleep(10);
            continue;
        }
        g_SwapChainOccluded = false;

        // Handle window resize (we don't resize directly in the WM_SIZE
        // handler)
        if (g_ResizeWidth != 0 && g_ResizeHeight != 0) {
            CleanupRenderTarget();
            g_pSwapChain->ResizeBuffers(0, g_ResizeWidth, g_ResizeHeight,
                                        DXGI_FORMAT_UNKNOWN, 0);
            g_ResizeWidth = g_ResizeHeight = 0;
            CreateRenderTarget();
        }

        // Start the Dear ImGui frame
        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();

        gui->ImGuiNewFrame();

        // Rendering
        ImGui::Render();
        const float clear_color_with_alpha[4] = {
            gui->clear_color.x * gui->clear_color.w,
            gui->clear_color.y * gui->clear_color.w,
            gui->clear_color.z * gui->clear_color.w, gui->clear_color.w};
        g_pd3dDeviceContext->OMSetRenderTargets(1, &g_mainRenderTargetView,
                                                nullptr);
        g_pd3dDeviceContext->ClearRenderTargetView(g_mainRenderTargetView,
                                                   clear_color_with_alpha);
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

        // Present
        HRESULT hr = g_pSwapChain->Present(1, 0); // Present with vsync
        // HRESULT hr = g_pSwapChain->Present(0, 0); // Present without vsync
        g_SwapChainOccluded = (hr == DXGI_STATUS_OCCLUDED);

        now = GetTickCount();
        if (((GOAL_FPS * 1000 - now - last_time) / 1000) == 0) {
            Sleep(1000 / GOAL_FPS);
        } else {
            DWORD toSleep =
                1000 / ((GOAL_FPS * 1000 - (now - last_time)) / 1000);
            // std::cout << "sleeping: " << toSleep << std::endl;
            Sleep(toSleep);
        }

        last_time = GetTickCount();
    }

    rdev.dwFlags = RIDEV_REMOVE;
    RegisterRawInputDevices(&rdev, 1, sizeof(rdev));

    // Cleanup
    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();

    CleanupDeviceD3D();
    ::DestroyWindow(hwnd);
    ::UnregisterClassW(wc.lpszClassName, wc.hInstance);

    return 0;
}

// Helper functions

bool CreateDeviceD3D(HWND hWnd) {
    // Setup swap chain
    DXGI_SWAP_CHAIN_DESC sd;
    ZeroMemory(&sd, sizeof(sd));
    sd.BufferCount = 2;
    sd.BufferDesc.Width = 0;
    sd.BufferDesc.Height = 0;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferDesc.RefreshRate.Numerator = 60;
    sd.BufferDesc.RefreshRate.Denominator = 1;
    sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = hWnd;
    sd.SampleDesc.Count = 1;
    sd.SampleDesc.Quality = 0;
    sd.Windowed = TRUE;
    sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

    UINT createDeviceFlags = 0;
    // createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
    D3D_FEATURE_LEVEL featureLevel;
    const D3D_FEATURE_LEVEL featureLevelArray[2] = {
        D3D_FEATURE_LEVEL_11_0,
        D3D_FEATURE_LEVEL_10_0,
    };
    HRESULT res = D3D11CreateDeviceAndSwapChain(
        nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, createDeviceFlags,
        featureLevelArray, 2, D3D11_SDK_VERSION, &sd, &g_pSwapChain,
        &g_pd3dDevice, &featureLevel, &g_pd3dDeviceContext);
    if (res == DXGI_ERROR_UNSUPPORTED) // Try high-performance WARP software
                                       // driver if hardware is not available.
        res = D3D11CreateDeviceAndSwapChain(
            nullptr, D3D_DRIVER_TYPE_WARP, nullptr, createDeviceFlags,
            featureLevelArray, 2, D3D11_SDK_VERSION, &sd, &g_pSwapChain,
            &g_pd3dDevice, &featureLevel, &g_pd3dDeviceContext);
    if (res != S_OK)
        return false;

    CreateRenderTarget();
    return true;
}

void CleanupDeviceD3D() {
    CleanupRenderTarget();
    if (g_pSwapChain) {
        g_pSwapChain->Release();
        g_pSwapChain = nullptr;
    }
    if (g_pd3dDeviceContext) {
        g_pd3dDeviceContext->Release();
        g_pd3dDeviceContext = nullptr;
    }
    if (g_pd3dDevice) {
        g_pd3dDevice->Release();
        g_pd3dDevice = nullptr;
    }
}

void CreateRenderTarget() {
    ID3D11Texture2D *pBackBuffer;
    g_pSwapChain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer));
    g_pd3dDevice->CreateRenderTargetView(pBackBuffer, nullptr,
                                         &g_mainRenderTargetView);
    pBackBuffer->Release();
}

void CleanupRenderTarget() {
    if (g_mainRenderTargetView) {
        g_mainRenderTargetView->Release();
        g_mainRenderTargetView = nullptr;
    }
}

// Forward declare message handler from imgui_impl_win32.cpp
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd,
                                                             UINT msg,
                                                             WPARAM wParam,
                                                             LPARAM lParam);

// Win32 message handler
// You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if
// dear imgui wants to use your inputs.
// - When io.WantCaptureMouse is true, do not dispatch mouse input data to your
// main application, or clear/overwrite your copy of the mouse data.
// - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to
// your main application, or clear/overwrite your copy of the keyboard data.
// Generally you may always pass all inputs to dear imgui, and hide them from
// your application based on those two flags.
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (msg == WM_INPUT) {
        UINT dwSize;
        HRESULT hResult;

        GetRawInputData((HRAWINPUT)lParam, RID_INPUT, NULL, &dwSize,
                        sizeof(RAWINPUTHEADER));

        RAWINPUT *raw = (RAWINPUT *)malloc(dwSize);

        if (GetRawInputData((HRAWINPUT)lParam, RID_INPUT, raw, &dwSize,
                            sizeof(RAWINPUTHEADER)) != dwSize)
            std::cout << "GetRawInputData does not return correct size !"
                      << std::endl;

        uint32_t pcbSize = 0;

        GetRawInputDeviceInfoW(raw->header.hDevice, RIDI_DEVICENAME, nullptr,
                               &pcbSize);

        if (pcbSize == 0) {
            std::cout << "GetRawInputDeviceInfo returned 0 for size!"
                      << std::endl;
            free(raw);
            return 0;
        }

        LPTSTR devName = (LPTSTR)malloc(pcbSize * sizeof(TCHAR));

        if (devName == nullptr) {
            std::cout << "malloc failed" << std::endl;
            free(raw);
            exit(1);
        }

        if (GetRawInputDeviceInfo(raw->header.hDevice, RIDI_DEVICENAME, devName,
                                  &pcbSize) < 0) {
            std::cout << "GetRawInputDeviceInfo returned value < 0"
                      << std::endl;
            free(raw);
            free(devName);
            return 0;
        }

        uint8_t tries = 5;
        LPWSTR productStr = (LPWSTR)malloc((1 << tries) * sizeof(WCHAR));

        if (productStr == nullptr) {
            std::cout << "malloc failed" << std::endl;
            free(raw);
            free(devName);
            exit(1);
        }

        HANDLE dev = CreateFile(devName, GENERIC_READ | GENERIC_WRITE,
                                FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);

        while (!HidD_GetProductString(dev, (PVOID)productStr,
                                      (1 << tries) * sizeof(WCHAR))) {
            if (tries > 10) {
                break;
            }

            tries++;

            LPWSTR newProductStr = (LPWSTR)realloc(productStr, (1 << tries) * sizeof(WCHAR));

            if (newProductStr == nullptr) {
                std::cout << "realloc failed" << std::endl;
                free(raw);
                free(devName);
                free(productStr);
                CloseHandle(dev);
                exit(1);
            }

            productStr = newProductStr;
        };

        std::cout << std::hex << "TYPE: " << raw->header.dwType << std::endl;

        if (raw->header.dwType == RIM_TYPEKEYBOARD) {
            RAWKEYBOARD kbd = raw->data.keyboard;

            std::wcout << std::hex << "Kbd: make=" << kbd.MakeCode
                       << " Flags:" << kbd.Flags << " Reserved:" << kbd.Reserved
                       << " ExtraInformation:" << kbd.ExtraInformation
                       << ", msg=" << kbd.Message << " VK=" << kbd.VKey
                       << " deviceName='" << productStr << "'" << std::dec
                       << std::endl;
        } else if (raw->header.dwType == RIM_TYPEMOUSE) {
            std::cout << "mouse?";
        }

        free(productStr);
        CloseHandle(dev);
        free(devName);
        free(raw);

        return 0;
    }

    if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
        return true;

    switch (msg) {
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
