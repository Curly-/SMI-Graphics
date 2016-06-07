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

#endif

#define SME_WINDOW_BORDERLESS_FULLSCREEN 1
#define SME_WINDOW_RESIZABLE 2
#define SME_WINDOW_MAXIMISED 4
#define SME_WINDOW_MINIMISED 8

namespace SME { namespace Window {
#if defined _WIN32
    HWND hwnd;
#elif defined __linux__

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
    
#if defined _WIN32
    /*
     * Called when the window receives an event
     */
    void msgCheck();
#endif
}}

#endif	/* SME_WINDOW_H */

