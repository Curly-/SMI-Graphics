#include "SME_window.h"
#include <iostream>
#include <SME_events.h>
#include <SME_core.h>

#if defined _WIN32

LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    SME::Events::Event event;
    switch (uMsg) {
        case WM_CLOSE:
            event.type = SME::Events::SME_WINDOW;
            event.windowEvent.event = SME::Events::SME_WINDOW_CLOSE;
            SME::Events::createEvent(event);
            break;
        case WM_KEYDOWN:
            event.type = SME::Events::SME_KEYBOARD;
            event.keyboardEvent.event = SME::Events::SME_KEYBOARD_KEYDOWN;
            event.keyboardEvent.repeated = lParam & (1<<30); //repeat count always returns 1, so use bit 30 instead
            event.keyboardEvent.scancode = SME::Keyboard::OSScancodeTable[lParam >> 16 & 0xFF]; //bits 16-23: scan code
            SME::Events::createEvent(event);
            break;
        case WM_KEYUP:
            event.type = SME::Events::SME_KEYBOARD;
            event.keyboardEvent.event = SME::Events::SME_KEYBOARD_KEYUP;
            event.keyboardEvent.repeated = 0; //can't be anything else
            event.keyboardEvent.scancode = SME::Keyboard::OSScancodeTable[lParam >> 16 & 0xFF];
            SME::Events::createEvent(event);
            break;
    }
    return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

void SME::Window::msgCheck() {
    MSG msg;
    while (PeekMessage(&msg, hwnd, 0, 0, PM_REMOVE)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
}
#elif define __linux__

#endif

bool SME::Window::create(int width, int height, std::string title, int style) {
    bool borderlessfs = style & SME_WINDOW_BORDERLESS_FULLSCREEN;
    bool resizable = style & SME_WINDOW_RESIZABLE;
    bool maximised = style & SME_WINDOW_MAXIMISED;
    bool minimised = style & SME_WINDOW_MINIMISED;

#if defined _WIN32
    int screenWidth = GetSystemMetrics(SM_CXSCREEN);
    int screenHeight = GetSystemMetrics(SM_CYSCREEN);

    DWORD winstyle = 0;
    if (borderlessfs) {
        winstyle = WS_OVERLAPPED | WS_POPUP;
        width = screenWidth;
        height = screenHeight;
    } else {
        winstyle = WS_OVERLAPPEDWINDOW & ~WS_THICKFRAME
                | (resizable ? WS_THICKFRAME : 0)
                | (maximised ? WS_MAXIMIZE : 0)
                | (minimised ? WS_MINIMIZE : 0);
    }

    HINSTANCE hInstance = GetModuleHandle(NULL);

    WNDCLASSEX wndClass;
    wndClass.cbSize = sizeof (WNDCLASSEX);
    wndClass.style = CS_HREDRAW | CS_VREDRAW;
    wndClass.lpfnWndProc = WndProc;
    wndClass.cbClsExtra = 0;
    wndClass.cbWndExtra = 0;
    wndClass.hInstance = hInstance;
    wndClass.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    wndClass.hCursor = LoadCursor(NULL, IDC_ARROW);
    wndClass.hbrBackground = (HBRUSH) GetStockObject(BLACK_BRUSH);
    wndClass.lpszMenuName = NULL;
    wndClass.lpszClassName = "SME";
    wndClass.hIconSm = LoadIcon(NULL, IDI_WINLOGO);

    RegisterClassEx(&wndClass);

    hwnd = CreateWindowEx(0,
            "SME",
            title.c_str(),
            winstyle,
            screenWidth / 2 - width / 2,
            screenHeight / 2 - height / 2,
            width,
            height,
            NULL,
            NULL,
            hInstance,
            NULL);

    ShowWindow(hwnd, SW_SHOW);
    SetForegroundWindow(hwnd);
    SetFocus(hwnd);
    
    SME::Core::addLoopUpdateHook(msgCheck);

    return hwnd != 0; //TODO work on this. this is either going to be true, or crash before
#elif defined __linux__

#endif
}