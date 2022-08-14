#pragma once

#define NOMINMAX
#include "Windows.h"
#include "WindowEvents.h"

class IApplication
{
public:
    
    virtual void Startup(void) = 0;
    virtual void Cleanup(void) = 0;
    virtual bool IsDone(void);
    virtual void Update(double deltaT) = 0;
    virtual void RenderScene(void) = 0;
    virtual void OnResize(int width, int height) {}
    virtual void OnKeyEvent(const KeyEvent& keyEvent) {}
    virtual void OnMouseMoved(int aDeltaX, int aDeltaY) {}
};

extern HWND g_hWnd;

int RunApplication(IApplication& app, const wchar_t* className, HINSTANCE hInst, int nCmdShow);

#define CREATE_APPLICATION( app_class ) \
    int WINAPI wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE /*hPrevInstance*/, _In_ LPWSTR /*lpCmdLine*/, _In_ int nCmdShow) \
    { \
        app_class app;\
        return RunApplication( app, L#app_class, hInstance, nCmdShow ); \
    }