#include "SME_window.h"
#include <SME_events.h>
#include <SME_core.h>
#include <SME_keyboard.h>
#include <iostream>

int windowWidth;
int windowHeight;

int newWidth = 0; //to avoid resize spam
int newHeight = 0;

int SME::Window::getWidth(){
    return windowWidth;
}
    
int SME::Window::getHeight(){
    return windowHeight;
}

#if defined _WIN32

#include <Windowsx.h>

HWND SME::Window::hwnd;
HINSTANCE SME::Window::hInstance;

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

void msgCheck() {
    MSG msg;
    while (PeekMessage(&msg, SME::Window::hwnd, 0, 0, PM_REMOVE)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
}
#elif defined __linux__

#include <xcb/xcb_keysyms.h>
#include <xcb/randr.h>
#include <X11/keysym.h> //I blame XCB
#include <string.h>  //I also blame netbeans for messing up my passive aggresive comments

xcb_connection_t* SME::Window::connection;
xcb_window_t SME::Window::window;

xcb_key_symbols_t *symbols;

xcb_intern_atom_reply_t* closeCookie;

int resizeCheck = 0; //count from last resize update tick to prevent event spamming

void msgCheck() {
    xcb_generic_event_t *event;
    SME::Events::Event smeEvent;
    resizeCheck++;
    while (event = xcb_poll_for_event(SME::Window::connection)) {
        switch(event->response_type & ~0x80){
            case XCB_CONFIGURE_NOTIFY:
            {
                xcb_configure_notify_event_t *cne = (xcb_configure_notify_event_t *)event;
                resizeCheck = 1;
                newWidth = cne->width;
                newHeight = cne->height;
                break;
            }
            case XCB_CLIENT_MESSAGE:
                if((*(xcb_client_message_event_t*)event).data.data32[0] == (*closeCookie).atom){
                    smeEvent.type = SME::Events::SME_WINDOW;
                    smeEvent.windowEvent.event = SME::Events::SME_WINDOW_CLOSE;
                    SME::Events::createEvent(smeEvent);
                }
                break;
            case XCB_KEY_PRESS:
                xcb_key_press_event_t *kp = (xcb_key_press_event_t*)event;
                fprintf(stdout, "%u", kp->detail);
                xcb_keysym_t sym = xcb_key_press_lookup_keysym(symbols, kp, 0);
                fprintf(stdout, "\t%u\n", sym);
                break;
        }
    }
    if(resizeCheck == 4){
        resizeCheck = 0;
        if(windowWidth != newWidth || windowHeight != newHeight){
            smeEvent.type = SME::Events::SME_WINDOW;
            smeEvent.windowEvent.event = SME::Events::SME_WINDOW_RESIZE;
            smeEvent.windowEvent.width = newWidth;
            smeEvent.windowEvent.height = newHeight;
            SME::Events::createEvent(smeEvent);

            windowWidth = newWidth;
            windowHeight = newHeight;
        }
    }
}
#endif

bool SME::Window::create(int width, int height, std::string title, int style) {
    bool borderlessfs = style & SME_WINDOW_BORDERLESS_FULLSCREEN;
    bool resizable = style & SME_WINDOW_RESIZABLE;
    bool maximised = style & SME_WINDOW_MAXIMISED;
    bool minimised = style & SME_WINDOW_MINIMISED;
    int monitorNumber = 0;//first monitor. TODO: config this

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
    
    hInstance = GetModuleHandle(NULL);

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

    RECT cRect; //Resize to account for border
    RECT wRect;
    GetClientRect(hwnd, &cRect); //Get drawable size
    GetWindowRect(hwnd, &wRect); //Get overall window size
    
    int newWidth = width + ((wRect.right-wRect.left)-(cRect.right-cRect.left));
    int newHeight = height + ((wRect.bottom-wRect.top)-(cRect.bottom-cRect.top));
    
    SetWindowPos(hwnd, HWND_TOP, //Set the window to on top
            screenWidth / 2 - newWidth / 2,
            screenHeight / 2 - newHeight / 2,
            newWidth,
            newHeight,
            SWP_SHOWWINDOW); //make the window visible
#elif defined __linux__
    //Window creation
    connection = xcb_connect(NULL, NULL);
    const xcb_setup_t      *setup  = xcb_get_setup (connection);
    xcb_screen_iterator_t   iter   = xcb_setup_roots_iterator (setup);
    xcb_screen_t           *screen = iter.data;
    xcb_cw_t mask = XCB_CW_EVENT_MASK;
    uint32_t values[] = {XCB_EVENT_MASK_KEY_PRESS | XCB_EVENT_MASK_STRUCTURE_NOTIFY};
    window = xcb_generate_id(connection);
    xcb_create_window(
            connection,
            XCB_COPY_FROM_PARENT,
            window,
            screen->root,
            0,
            0,
            width,
            height,
            0,
            XCB_WINDOW_CLASS_INPUT_OUTPUT,
            screen->root_visual,
            mask,
            values);  
    xcb_change_property(connection,
            XCB_PROP_MODE_REPLACE,
            window,
            XCB_ATOM_WM_NAME,
            XCB_ATOM_STRING,
            8,
            strlen(title.c_str()),
            title.c_str());
    xcb_map_window (connection, window);
    xcb_flush (connection);
    
    //Window centering code (yeah ik, blame xcb)
    
    xcb_randr_get_screen_resources_cookie_t screenResourcesCookie = {};
    screenResourcesCookie = xcb_randr_get_screen_resources(connection, window);
    
    xcb_randr_get_screen_resources_reply_t* screenResourcesReply = {};
    screenResourcesReply = xcb_randr_get_screen_resources_reply(connection,
            screenResourcesCookie, nullptr);
    
    int crtcsNum = 0;
    xcb_randr_crtc_t* firstCRTC;
    
    if(screenResourcesReply){
        crtcsNum = xcb_randr_get_screen_resources_crtcs_length(screenResourcesReply);
        
        firstCRTC = xcb_randr_get_screen_resources_crtcs(screenResourcesReply);
        
        xcb_randr_get_crtc_info_cookie_t crtcResourcesCookie = {};
        crtcResourcesCookie = xcb_randr_get_crtc_info(connection, *(firstCRTC+monitorNumber), 0);
        
        xcb_randr_get_crtc_info_reply_t* crtcResReply = {};
        crtcResReply = xcb_randr_get_crtc_info_reply(connection, crtcResourcesCookie, 0);
        
        const uint32_t values[] = {(uint32_t)(crtcResReply->width / 2 - width / 2),
                (uint32_t)(crtcResReply->height / 2 - height / 2)};
        
        xcb_configure_window(connection, window, XCB_CONFIG_WINDOW_X | XCB_CONFIG_WINDOW_Y, values);
    } else {
        printf("Warning, couldn't get reply from XServer while requesting for screen resources.");
    }
    
    //window closing detection
    
    xcb_intern_atom_cookie_t cookie1 = xcb_intern_atom(connection, 1,12, "WM_PROTOCOLS");
    xcb_intern_atom_reply_t* reply = xcb_intern_atom_reply(connection, cookie1, 0);
    
    xcb_intern_atom_cookie_t cookie2 = xcb_intern_atom(connection, 0, 16, "WM_DELETE_WINDOW");
    closeCookie = xcb_intern_atom_reply(connection, cookie2, 0);
    
    xcb_change_property(connection, XCB_PROP_MODE_REPLACE, window, (*reply).atom, 4, 32, 1, &(*closeCookie).atom);
    
    //apply changes        
    
    xcb_flush(connection);
    
    symbols = xcb_key_symbols_alloc(connection);
#endif
    windowWidth = width;
    windowHeight = height;
    
    SME::Core::addLoopUpdateHook(msgCheck);
    SME::Core::addCleanupHook(cleanup);
}

void SME::Window::cleanup() {
#if defined _WIN32
    DestroyWindow(hwnd);
#elif defined __linux__
    xcb_disconnect(connection);
#endif
}