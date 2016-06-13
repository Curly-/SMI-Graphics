/* 
 * File:   SME_window.h
 * Author: Sam
 *
 * Created on 21 May 2016, 14:39
 */

#ifndef SME_WINDOW_H
#define	SME_WINDOW_H

#include <string>

#if defined _WIN32
#include <windows.h>
#elif defined __linux__
#include <xcb/xcb.h>
#endif

#define SME_WINDOW_BORDERLESS_FULLSCREEN 1
#define SME_WINDOW_RESIZABLE 2
#define SME_WINDOW_MAXIMISED 4
#define SME_WINDOW_MINIMISED 8

namespace SME { namespace Window {
#if defined _WIN32
    extern HWND hwnd;
    extern HINSTANCE hInstance;
#elif defined __linux__
    extern xcb_connection_t* connection;
    extern xcb_window_t window;
#endif
    /*
     * Creates and shows the window
     */
    bool create(int width, int height, std::string title, int style);
    
    /*
     * Destroys the window
     * Called automatically, however, can be called manually
     */
    void cleanup();
    
    /**
     * Method to get the current width of the window
     * @return the width in pixels of the window
     */
    int getWidth();
    
    /**
     * Method to get the current height of the window
     * @return the height in pixels of the window
     */
    int getHeight();
}}

#endif	/* SME_WINDOW_H */

