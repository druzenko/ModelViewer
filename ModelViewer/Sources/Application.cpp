#include "pch.h"
#include "Application.h"
#include "Utility.h"
#include "GPUResource.h"
#include <iostream>
#include <WindowsX.h>

uint32_t g_DisplayWidth = 1920;
uint32_t g_DisplayHeight = 1080;

HWND g_hWnd = nullptr;
IApplication* g_App;

LARGE_INTEGER g_PriviousStartFrameTime;
LARGE_INTEGER g_Frequency;

LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);

void InitializeApplication(IApplication& game)
{
    int argc = 0;
    LPWSTR* argv = CommandLineToArgvW(GetCommandLineW(), &argc);

    QueryPerformanceFrequency(&g_Frequency);
    QueryPerformanceCounter(&g_PriviousStartFrameTime);

    g_App = &game;
    Graphics::Initialize();
    game.Startup();
}

bool UpdateApplication(IApplication& app)
{
    LARGE_INTEGER  currentTime;
    QueryPerformanceCounter(&currentTime);
    LONGLONG deltaInMicroSeconds = currentTime.QuadPart - g_PriviousStartFrameTime.QuadPart;
    deltaInMicroSeconds *= 1000000;
    deltaInMicroSeconds /= g_Frequency.QuadPart;
    g_PriviousStartFrameTime = currentTime;

    double deltaInSeconds = static_cast<double>(deltaInMicroSeconds) / 1000000;
    app.Update(deltaInSeconds);
    app.RenderScene();

    Graphics::Present();

    return !app.IsDone();
}

void TerminateApplication(IApplication& app)
{
    app.Cleanup();

    //GameInput::Shutdown();
}

int RunApplication(IApplication& app, const wchar_t* className, HINSTANCE hInst, int nCmdShow)
{
    std::cout << "RunApplication" << std::endl;

    WCHAR path[MAX_PATH];
    HMODULE hModule = GetModuleHandleW(NULL);
    if (GetModuleFileNameW(hModule, path, MAX_PATH) > 0)
    {
        PathRemoveFileSpecW(path);
        SetCurrentDirectoryW(path);
    }


    Microsoft::WRL::Wrappers::RoInitializeWrapper InitializeWinRT(RO_INIT_MULTITHREADED);
    //ASSERT(InitializeWinRT, "Windows runtime was not initialize.");

    RAWINPUTDEVICE Rid[2];

    Rid[0].usUsagePage = 0x01;          // HID_USAGE_PAGE_GENERIC
    Rid[0].usUsage = 0x02;              // HID_USAGE_GENERIC_MOUSE
    Rid[0].dwFlags = 0;    // adds mouse and also ignores legacy mouse messages
    Rid[0].hwndTarget = g_hWnd;

    Rid[1].usUsagePage = 0x01;          // HID_USAGE_PAGE_GENERIC
    Rid[1].usUsage = 0x06;              // HID_USAGE_GENERIC_KEYBOARD
    Rid[1].dwFlags = RIDEV_NOLEGACY;    // adds keyboard and also ignores legacy keyboard messages
    Rid[1].hwndTarget = g_hWnd;

    if (RegisterRawInputDevices(Rid, 1, sizeof(Rid[0])) == FALSE)
    {
        ASSERT(false, "Registration failed.");//Call GetLastError for the cause of the error
    }

    WNDCLASSEX wcex;
    wcex.cbSize = sizeof(WNDCLASSEX);
    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = WndProc;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = 0;
    wcex.hInstance = hInst;
    wcex.hIcon = LoadIcon(hInst, IDI_APPLICATION);
    wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wcex.lpszMenuName = nullptr;
    wcex.lpszClassName = className;
    wcex.hIconSm = LoadIcon(hInst, IDI_APPLICATION);

    ASSERT(0 != RegisterClassEx(&wcex), "Unable to register a window.");

    // Create window
    RECT rc = { 0, 0, (LONG)g_DisplayWidth, (LONG)g_DisplayHeight };
    AdjustWindowRect(&rc, WS_OVERLAPPEDWINDOW, FALSE);

    g_hWnd = CreateWindow(className, className, WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, rc.right - rc.left, rc.bottom - rc.top, nullptr, nullptr, hInst, nullptr);

    ASSERT(g_hWnd != 0, "Window was not created.");

    InitializeApplication(app);

    ShowWindow(g_hWnd, nCmdShow/*SW_SHOWDEFAULT*/);

    do
    {
        MSG msg = {};
        bool isDone = false;
        while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
            isDone |= msg.message == WM_QUIT;
        }

        if (isDone)
        {
            break;
        }
       
    } while (UpdateApplication(app));	// returns false to quit loop

    TerminateApplication(app);
    Graphics::Shutdown();
    return 0;
}

bool IApplication::IsDone(void)
{
    return false;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_SIZE:
        Graphics::Resize((UINT)(UINT64)lParam & 0xFFFF, (UINT)(UINT64)lParam >> 16);
        g_App->OnResize((UINT)(UINT64)lParam & 0xFFFF, (UINT)(UINT64)lParam >> 16);
        break;

    case WM_DESTROY:
    //case WM_CLOSE:
        PostQuitMessage(0);
        break;

    case WM_MOUSEMOVE:
        //g_App->OnMouseMoved(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
        break;

    case WM_SYSKEYDOWN:
    case WM_KEYDOWN:
    {
        MSG charMsg;
        // Get the Unicode character (UTF-16)
        unsigned int c = 0;
        // For printable characters, the next message will be WM_CHAR.
        // This message contains the character code we need to send the KeyPressed event.
        // Inspired by the SDL 1.2 implementation.
        if (PeekMessage(&charMsg, g_hWnd, 0, 0, PM_NOREMOVE) && charMsg.message == WM_CHAR)
        {
            GetMessage(&charMsg, g_hWnd, 0, 0);
            c = static_cast<unsigned int>(charMsg.wParam);
        }
        bool shift = (GetAsyncKeyState(VK_SHIFT) & 0x8000) != 0;
        bool control = (GetAsyncKeyState(VK_CONTROL) & 0x8000) != 0;
        bool alt = (GetAsyncKeyState(VK_MENU) & 0x8000) != 0;
 
        g_App->OnKeyEvent({ static_cast<KeyEvent::KeyCode>(wParam), KeyEvent::KeyState::Pressed, c, control, shift, alt });
    }
    case WM_SYSKEYUP:
    case WM_KEYUP:
    {
        bool shift = (GetAsyncKeyState(VK_SHIFT) & 0x8000) != 0;
        bool control = (GetAsyncKeyState(VK_CONTROL) & 0x8000) != 0;
        bool alt = (GetAsyncKeyState(VK_MENU) & 0x8000) != 0;
        unsigned int c = 0;
        unsigned int scanCode = (lParam & 0x00FF0000) >> 16;

        // Determine which key was released by converting the key code and the scan code
        // to a printable character (if possible).
        // Inspired by the SDL 1.2 implementation.
        unsigned char keyboardState[256];
        GetKeyboardState(keyboardState);
        wchar_t translatedCharacters[4];
        if (int result = ToUnicodeEx(static_cast<UINT>(wParam), scanCode, keyboardState, translatedCharacters, 4, 0, NULL) > 0)
        {
            c = translatedCharacters[0];
        }

        g_App->OnKeyEvent({ static_cast<KeyEvent::KeyCode>(wParam), KeyEvent::KeyState::Released, c, control, shift, alt });
    }
    case WM_INPUT:
    {
        //https://learn.microsoft.com/uk-ua/windows/win32/inputdev/using-raw-input?redirectedfrom=MSDN

        HRAWINPUT inputHandle = reinterpret_cast<HRAWINPUT>(lParam);
        USHORT inputCode = GET_RAWINPUT_CODE_WPARAM(wParam);

        if (inputCode != RIM_INPUT && inputCode != RIM_INPUTSINK)
            break;


        UINT dwSize;

        GetRawInputData((HRAWINPUT)lParam, RID_INPUT, NULL, &dwSize, sizeof(RAWINPUTHEADER));
        LPBYTE lpb = reinterpret_cast<LPBYTE>(alloca(sizeof(BYTE) * dwSize));
        if (lpb == NULL)
        {
            return 0;
        }

        if (GetRawInputData((HRAWINPUT)lParam, RID_INPUT, lpb, &dwSize, sizeof(RAWINPUTHEADER)) != dwSize)
            OutputDebugString(TEXT("GetRawInputData does not return correct size !\n"));

        RAWINPUT* raw = (RAWINPUT*)lpb;

        if (raw->header.dwType == RIM_TYPEMOUSE)
        {
            g_App->OnMouseMoved(raw->data.mouse.lLastX, raw->data.mouse.lLastY);
        }

        //if (inputCode == RIM_INPUT)
        //return DefWindowProc(hWnd, message, wParam, lParam);
        break;
    }
    break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }

    return 0;
}