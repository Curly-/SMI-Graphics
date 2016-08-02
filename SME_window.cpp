#include "SME_window.h"
#include "SME_ui_events.h"
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

void createMouseUpEvent(int button, HWND hWnd) {
    SME::Event::UI::MouseUpEvent e;
    e.hwnd = hWnd;
    e.x = mouseX;
    e.y = mouseY;
    e.button = button;
    SME::Event::sendEvent(e);
}

void createMouseDownEvent(int button, HWND hWnd) {
    SME::Event::UI::MouseDownEvent e;
    e.hwnd = hWnd;
    e.x = mouseX;
    e.y = mouseY;
    e.button = button;
    SME::Event::sendEvent(e);
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
        /*
         * Alt+F4, close button, etc
         */
        case WM_CLOSE: {
            SME::Event::UI::WindowCloseEvent e;
            e.hwnd = hWnd;
            SME::Event::sendEvent(e);
            return 0; //Return 0 to avoid default implementation (Window closing)
        }
        /*
         * Called when the window is resized, maximised, minimised
         */
        case WM_SIZE: {
            newWidth = LOWORD(lParam);
            newHeight = HIWORD(lParam);
            if (wParam == SIZE_MINIMIZED) {
                SME::Event::UI::WindowMinimiseEvent e;
                e.hwnd = hWnd;
                e.width = newWidth;
                e.height = newHeight;
                SME::Event::sendEvent(e);
            } else if (wParam == SIZE_MAXIMIZED) {
                SME::Event::UI::WindowMaximiseEvent e;
                e.hwnd = hWnd;
                e.width = newWidth;
                e.height = newHeight;
                SME::Event::sendEvent(e);
            }
            break;
        }
        /*
         * Called when done resizing/moving
         * Used to avoid event spam when the window is resized
         */
        case WM_EXITSIZEMOVE: {
            SME::Event::UI::WindowResizeEvent e;
            e.hwnd = hWnd;
            e.width = newWidth;
            e.height = newHeight;
            SME::Event::sendEvent(e);
            break;
        }
        /*
         * Called when a key is pressed
         */
        case WM_KEYDOWN: {
            SME::Event::UI::KeyDownEvent e;
            e.hwnd = hWnd;
            e.repeated = lParam & (1 << 30); //repeat count always returns 1, so use bit 30 instead
            e.scancode = SME::Keyboard::OSScancodeTable[lParam >> 16 & 0xFF]; //bits 16-23: scan code
            e.keycode = SME::Keyboard::OSKeycodeTable[wParam];
            
            SME::Keyboard::KeyStates[e.scancode] = true;
            SME::Event::sendEvent(e);
            break;
        }
        /*
         * Called when a key is released
         */
        case WM_KEYUP: {
            SME::Event::UI::KeyUpEvent e;
            e.hwnd = hWnd;
            e.scancode = SME::Keyboard::OSScancodeTable[lParam >> 16 & 0xFF];
            e.keycode = SME::Keyboard::OSKeycodeTable[wParam];
            
            SME::Keyboard::KeyStates[e.scancode] = false;
            SME::Event::sendEvent(e);
            break;
        }
        /*
         * Called when the mouse (maybe cursor in general) is moved in the window
         */
        case WM_MOUSEMOVE: {
            SME::Event::UI::MouseMoveEvent e;
            e.hwnd = hWnd;
            e.xdelta = GET_X_LPARAM(lParam) - mouseX;
            e.ydelta = GET_Y_LPARAM(lParam) - mouseY;
            mouseX = e.x = GET_X_LPARAM(lParam);
            mouseY = e.y = GET_Y_LPARAM(lParam);
            SME::Event::sendEvent(e);
            break;
        }
        /*
         * Called when the left mouse button is pressed (button 1)
         */
        case WM_LBUTTONDOWN: {
            createMouseDownEvent(1, hWnd);
            break;
        }
        /*
         * Called when the right mouse button is pressed (button 2)
         */
        case WM_RBUTTONDOWN:
            createMouseDownEvent(2, hWnd);
            break;
        /*
         * Called when the middle mouse button is pressed (button 3)
         */
        case WM_MBUTTONDOWN:
            createMouseDownEvent(3, hWnd);
            break;
        /*
         * Called when any other mouse button is pressed (button >3)
         */
        case WM_XBUTTONDOWN:
            createMouseDownEvent(GET_XBUTTON_WPARAM(wParam)+3, hWnd);
            break;
        case WM_LBUTTONUP:
            createMouseUpEvent(1, hWnd);
            break;
        case WM_RBUTTONUP:
            createMouseUpEvent(2, hWnd);
            break;
        case WM_MBUTTONUP:
            createMouseUpEvent(3, hWnd);
            break;
        case WM_XBUTTONUP:
            createMouseUpEvent(GET_XBUTTON_WPARAM(wParam)+3, hWnd);
            break;
        /*
         * Called when the mouse wheel is scrolled
         */
        case WM_MOUSEWHEEL: {
            SME::Event::UI::MouseWheelEvent e;
            e.hwnd = hWnd;
            e.scroll = ((int16_t)HIWORD(wParam))/WHEEL_DELTA;
            e.x = mouseX;
            e.y = mouseY;
            SME::Event::sendEvent(e);
        }
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

#include <xcb/randr.h>
#include <string.h>
#include <xkbcommon/xkbcommon-x11.h>

//Window stuff
xcb_connection_t* SME::Window::connection;
xcb_window_t SME::Window::window;

//Close event
xcb_intern_atom_reply_t* closeCookie;

//Resize event
int resizeCheck = 0; //count from last resize update tick to prevent event spamming

//Keyboard stuff
struct xkb_context *ctx;
struct xkb_keymap *keymap;
struct xkb_state *state;

void msgCheck() {
    xcb_generic_event_t *event;
    resizeCheck++;
    while ((event = xcb_poll_for_event(SME::Window::connection))) {
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
                    SME::Event::UI::WindowCloseEvent e;
                    e.window = SME::Window::window;
                    SME::Event::sendEvent(e);
                }
                break;
            case XCB_KEY_PRESS:
                xcb_key_press_event_t *kp = (xcb_key_press_event_t*)event;
                xkb_keycode_t keycode = kp->detail;
                //xkb_keysym_t keysym = xkb_state_key_get_one_sym(state, keycode);
                //char name[64];
                //xkb_keysym_get_name(keysym, name, sizeof(name));
                //fprintf(stdout, "%u (%s)\n", keysym, name);
                SME::Event::UI::KeyDownEvent e;
                e.window = SME::Window::window;
                e.scancode = SME::Keyboard::OSScancodeTable[kp->detail];
                e.keycode = SME::Keyboard::Key::KEY_UNKNOWN;
                e.repeated = false;
                
                SME::Keyboard::KeyStates[e.scancode] = true;
                SME::Event::sendEvent(e);
                break;
        }
    }
    if(resizeCheck == 4){
        resizeCheck = 0;
        if(windowWidth != newWidth || windowHeight != newHeight){
            SME::Event::UI::WindowResizeEvent e;
            e.window = SME::Window::window;
            e.width = newWidth;
            e.height = newHeight;
            SME::Event::sendEvent(e);

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
        winstyle = (WS_OVERLAPPEDWINDOW & ~WS_THICKFRAME)
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
    
    //keyboard layout stuff
    
    ctx = xkb_context_new(XKB_CONTEXT_NO_FLAGS);
    if (!ctx){
        fprintf(stderr, "Couldn't create XKB context.\n");
        return false;
    }
    
    if(!xkb_x11_setup_xkb_extension(connection, XKB_X11_MIN_MAJOR_XKB_VERSION, XKB_X11_MIN_MINOR_XKB_VERSION, XKB_X11_SETUP_XKB_EXTENSION_NO_FLAGS, nullptr, nullptr, nullptr, nullptr)){
        fprintf(stderr, "Couldn't setup XKB extension.\n");
        return false;
    }
        
    int32_t device_id = xkb_x11_get_core_keyboard_device_id(connection);
    if (device_id == -1) {
        fprintf(stderr, "Couldn't get a keyboard device id.\n");
        return false;
    }
    
    keymap = xkb_x11_keymap_new_from_device(ctx, connection, device_id,
                                            XKB_KEYMAP_COMPILE_NO_FLAGS);
    if (!keymap) {
        fprintf(stderr, "Couldn't create keymap.\n");
        return false;
    }
    
    state = xkb_x11_state_new_from_device(keymap, connection, device_id);
    if(!state){
        fprintf(stderr, "Couldn't create an XKB state.\n");
        return false;
    }
    
    //apply changes        
    
    xcb_flush(connection);
#endif
    windowWidth = width;
    windowHeight = height;
    
    SME::Core::addLoopUpdateHook(msgCheck);
    SME::Core::addCleanupHook(cleanup);
    return true;
}

void SME::Window::cleanup() {
#if defined _WIN32
    DestroyWindow(hwnd);
#elif defined __linux__
    xkb_state_unref(state);
    xkb_keymap_unref(keymap);
    xkb_context_unref(ctx);
    xcb_disconnect(connection);
#endif
}