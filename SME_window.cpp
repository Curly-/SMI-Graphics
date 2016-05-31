#include "SME_window.h"
#include <iostream>
#include <SME_events.h>
#include <SME_core.h>

#if defined _WIN32

#include <Windowsx.h>

int newWidth = 0; //to avoid resize spam
int newHeight = 0;

int mouseX; //for delta
int mouseY;

void createClickEvent(int button, SME::Events::EventType type) {
    SME::Events::Event event;
    event.type = SME::Events::SME_MOUSE;
    event.mouseEvent.event = type;
    event.mouseEvent.x = mouseX;
    event.mouseEvent.y = mouseY;
    event.mouseEvent.button = button;
    SME::Events::createEvent(event);
}

//TODO keycodes
//TODO isPressed(key)

LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    SME::Events::Event event;
    switch (uMsg) {
        case WM_CLOSE:
            event.type = SME::Events::SME_WINDOW;
            event.windowEvent.event = SME::Events::SME_WINDOW_CLOSE;
            SME::Events::createEvent(event);
            break;
        case WM_SIZE:
            newWidth = LOWORD(lParam);
            newHeight = HIWORD(lParam);
            if (wParam == SIZE_MINIMIZED) {
                event.type = SME::Events::SME_WINDOW;
                event.windowEvent.event = SME::Events::SME_WINDOW_MINIMISE;
                SME::Events::createEvent(event);
            } else if (wParam == SIZE_MAXIMIZED) {
                event.type = SME::Events::SME_WINDOW;
                event.windowEvent.event = SME::Events::SME_WINDOW_MAXIMISE;
                SME::Events::createEvent(event);
            }
            break;
        case WM_EXITSIZEMOVE: //called when done resizing
            event.type = SME::Events::SME_WINDOW;
            event.windowEvent.event = SME::Events::SME_WINDOW_RESIZE;
            event.windowEvent.width = newWidth;
            event.windowEvent.height = newHeight;
            SME::Events::createEvent(event);
            break;
        case WM_KEYDOWN:
            event.type = SME::Events::SME_KEYBOARD;
            event.keyboardEvent.event = SME::Events::SME_KEYBOARD_KEYDOWN;
            event.keyboardEvent.repeated = lParam & (1 << 30); //repeat count always returns 1, so use bit 30 instead
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
        case WM_MOUSEMOVE:
            event.type = SME::Events::SME_MOUSE;
            event.mouseEvent.event = SME::Events::SME_MOUSE_MOVE;
            event.mouseEvent.xdelta = GET_X_LPARAM(lParam) - mouseX;
            event.mouseEvent.ydelta = GET_Y_LPARAM(lParam) - mouseY;
            mouseX = event.mouseEvent.x = GET_X_LPARAM(lParam);
            mouseY = event.mouseEvent.y = GET_Y_LPARAM(lParam);
            SME::Events::createEvent(event);
            break;
        case WM_LBUTTONDOWN:
            createClickEvent(1, SME::Events::SME_MOUSE_MOUSEDOWN);
            break;
        case WM_RBUTTONDOWN:
            createClickEvent(2, SME::Events::SME_MOUSE_MOUSEDOWN);
            break;
        case WM_MBUTTONDOWN:
            createClickEvent(3, SME::Events::SME_MOUSE_MOUSEDOWN);
            break;
        case WM_XBUTTONDOWN:
            createClickEvent(GET_XBUTTON_WPARAM(wParam)+3, SME::Events::SME_MOUSE_MOUSEDOWN);
            break;
        case WM_LBUTTONUP:
            createClickEvent(1, SME::Events::SME_MOUSE_MOUSEUP);
            break;
        case WM_RBUTTONUP:
            createClickEvent(2, SME::Events::SME_MOUSE_MOUSEUP);
            break;
        case WM_MBUTTONUP:
            createClickEvent(3, SME::Events::SME_MOUSE_MOUSEUP);
            break;
        case WM_XBUTTONUP:
            createClickEvent(GET_XBUTTON_WPARAM(wParam)+3, SME::Events::SME_MOUSE_MOUSEUP);
            break;
        case WM_MOUSEWHEEL:
            event.type = SME::Events::SME_MOUSE;
            event.mouseEvent.event = SME::Events::SME_MOUSE_WHEEL;
            event.mouseEvent.scroll = ((int16_t)HIWORD(wParam))/WHEEL_DELTA; //positive = forward, negative = backward
            event.mouseEvent.x = mouseX;
            event.mouseEvent.y = mouseY;
            SME::Events::createEvent(event);
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
#elif defined __linux__

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