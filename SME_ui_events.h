/* 
 * File:   SME_core_events.h
 * Author: Sam
 *
 * Created on 22 July 2016, 17:35
 */

#ifndef SME_CORE_EVENTS_H
#define	SME_CORE_EVENTS_H

#include "SME_event.h"
#include <SME_keys.h>

#ifdef _WIN32
#include <windows.h>
#elif defined __linux__
#include <xcb/xproto.h>
#endif

namespace SME {
    namespace Event {
        namespace UI {
            
            #define d(name) const std::string name = #name
            namespace UIEventKeys {
                d(WindowCloseEvent);
                d(WindowResizeEvent);
                d(WindowMaximiseEvent);
                d(WindowMinimiseEvent);
                d(MouseUpEvent);
                d(MouseDownEvent);
                d(MouseMoveEvent);
                d(MouseWheelEvent);
                d(KeyDownEvent);
                d(KeyUpEvent);
            };
            #undef d
            
            struct WindowEvent : public Event {
                WindowEvent(std::string type);
                #ifdef _WIN32
                    HWND hwnd;
                #elif defined __linux__
                    xcb_window_t window;
                #endif
            };
            
            struct MouseEvent : public WindowEvent {
                MouseEvent(std::string type);
                int x; int y;
            };
            
            struct KeyEvent : public WindowEvent {
                KeyEvent(std::string type);
                SME::Keyboard::Key scancode; SME::Keyboard::Key keycode;
            };
            
            struct WindowCloseEvent : public WindowEvent { WindowCloseEvent(); };
            struct WindowResizeEvent : public WindowEvent { WindowResizeEvent(); int width; int height; };
            struct WindowMaximiseEvent : public WindowEvent { WindowMaximiseEvent(); int width; int height; };
            struct WindowMinimiseEvent : public WindowEvent { WindowMinimiseEvent(); int width; int height; };

            struct MouseUpEvent : public MouseEvent { MouseUpEvent(); int button; };
            struct MouseDownEvent : public MouseEvent { MouseDownEvent(); int button; };
            struct MouseMoveEvent : public MouseEvent { MouseMoveEvent(); int xdelta; int ydelta; };
            struct MouseWheelEvent : public MouseEvent { MouseWheelEvent(); int scroll; };

            struct KeyDownEvent : public KeyEvent { KeyDownEvent(); bool repeated; };
            struct KeyUpEvent : public KeyEvent { KeyUpEvent(); };
            
        }
    }
}

#endif	/* SME_CORE_EVENTS_H */

