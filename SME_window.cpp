#include "SME_window.h"
#include <SME_events.h>
#include <SME_core.h>
#include <SME_keyboard.h>
#include <iostream>

#if defined _WIN32

#include <Windowsx.h>

HWND SME::Window::hwnd;

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
//TODO isPressed(key) (SME::Keyboard::KeyStates)

LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    SME::Events::Event event;
    switch (uMsg) {
        /*
         * Alt+F4, close button, etc
         */
        case WM_CLOSE:
            event.type = SME::Events::SME_WINDOW;
            event.windowEvent.event = SME::Events::SME_WINDOW_CLOSE;
            SME::Events::createEvent(event);
            return 0; //Return 0 to avoid default implementation (Window closing)
        /*
         * Called when the window is resized, maximised, minimised
         */
        case WM_SIZE:
            std::cout << "size" << std::endl;
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
        /*
         * Called when done resizing/moving
         * Used to avoid event spam when the window is resized
         */
        case WM_EXITSIZEMOVE:
            event.type = SME::Events::SME_WINDOW;
            event.windowEvent.event = SME::Events::SME_WINDOW_RESIZE;
            event.windowEvent.width = newWidth;
            event.windowEvent.height = newHeight;
            SME::Events::createEvent(event);
            break;
        /*
         * Called when a key is pressed
         */
        case WM_KEYDOWN:
            event.type = SME::Events::SME_KEYBOARD;
            event.keyboardEvent.event = SME::Events::SME_KEYBOARD_KEYDOWN;
            event.keyboardEvent.repeated = lParam & (1 << 30); //repeat count always returns 1, so use bit 30 instead
            event.keyboardEvent.scancode = SME::Keyboard::OSScancodeTable[lParam >> 16 & 0xFF]; //bits 16-23: scan code
            event.keyboardEvent.keycode = SME::Keyboard::OSKeycodeTable[wParam];
            SME::Events::createEvent(event);
            
            SME::Keyboard::KeyStates[event.keyboardEvent.scancode] = true;
            break;
        /*
         * Called when a key is released
         */
        case WM_KEYUP:
            event.type = SME::Events::SME_KEYBOARD;
            event.keyboardEvent.event = SME::Events::SME_KEYBOARD_KEYUP;
            event.keyboardEvent.repeated = 0; //can't be anything else
            event.keyboardEvent.scancode = SME::Keyboard::OSScancodeTable[lParam >> 16 & 0xFF];
            event.keyboardEvent.keycode = SME::Keyboard::OSKeycodeTable[wParam];
            SME::Events::createEvent(event);
            
            SME::Keyboard::KeyStates[event.keyboardEvent.scancode] = false;
            break;
        /*
         * Called when the mouse (maybe cursor in general) is moved in the window
         */
        case WM_MOUSEMOVE:
            event.type = SME::Events::SME_MOUSE;
            event.mouseEvent.event = SME::Events::SME_MOUSE_MOVE;
            event.mouseEvent.xdelta = GET_X_LPARAM(lParam) - mouseX;
            event.mouseEvent.ydelta = GET_Y_LPARAM(lParam) - mouseY;
            mouseX = event.mouseEvent.x = GET_X_LPARAM(lParam);
            mouseY = event.mouseEvent.y = GET_Y_LPARAM(lParam);
            SME::Events::createEvent(event);
            break;
        /*
         * Called when the left mouse button is pressed (button 1)
         */
        case WM_LBUTTONDOWN:
            createClickEvent(1, SME::Events::SME_MOUSE_MOUSEDOWN);
            break;
        /*
         * Called when the right mouse button is pressed (button 2)
         */
        case WM_RBUTTONDOWN:
            createClickEvent(2, SME::Events::SME_MOUSE_MOUSEDOWN);
            break;
        /*
         * Called when the middle mouse button is pressed (button 3)
         */
        case WM_MBUTTONDOWN:
            createClickEvent(3, SME::Events::SME_MOUSE_MOUSEDOWN);
            break;
        /*
         * Called when any other mouse button is pressed (button >3)
         */
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
        /*
         * Called when the mouse wheel is scrolled
         */
        case WM_MOUSEWHEEL:
            event.type = SME::Events::SME_MOUSE;
            event.mouseEvent.event = SME::Events::SME_MOUSE_WHEEL;
            event.mouseEvent.scroll = ((int16_t)HIWORD(wParam))/WHEEL_DELTA;
            event.mouseEvent.x = mouseX;
            event.mouseEvent.y = mouseY;
            SME::Events::createEvent(event);
    }
    return DefWindowProc(hWnd, uMsg, wParam, lParam); //Forward the event to windows to call the default implementation
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

    ShowWindow(hwnd, SW_SHOW); //Make the window visible
    SetForegroundWindow(hwnd);
    SetFocus(hwnd);

    SME::Core::addLoopUpdateHook(msgCheck);

    return hwnd != 0; //TODO work on this. this is either going to be true, or crash before
#elif defined __linux__

#endif
    SME::Core::addCleanupHook(cleanup);
}

void SME::Window::cleanup() {
#if defined _WIN32
    DestroyWindow(hwnd);
#elif defined __linux__
    
#endif
}