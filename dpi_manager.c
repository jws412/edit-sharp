#include <windows.h>
#include "dpi_manager.h"

static BOOL (*pSetAwarenessFunction)(DPI_AWARENESS_CONTEXT) = NULL;
static unsigned int (*pGetDpiFunction)(HWND) = NULL;

enum EsError initDpiManager(void) { // Initializes the DPI manager by dynamically loading required functions for windows
    
    #ifdef __MINGW32__
    #pragma GCC diagnostic push
    #pragma GCC diagnostic ignored "-Wpedantic"
    #endif
    // These functions are used to dynamically load 2 functions from the user3d.dll file to ensure compatibility with different versions of Windows
    void *pRawGetFunction = 
        GetProcAddress(GetModuleHandle(TEXT("user32.dll")),
        "GetDpiForWindow"); // GetDPiForWindow sets the DPI awareness context for the process
    void *pRawSetFunction = 
        GetProcAddress(GetModuleHandle(TEXT("user32.dll")),
        "SetProcessDpiAwarenessContext"); // SetProcessDpiAwarenessContext retrieves the DPI value for a specific window
    
    if (pRawGetFunction == NULL || pRawSetFunction == NULL) {
        return ES_ERROR_FAILED_DPI;
        
    }
    
    // Pointers
    pGetDpiFunction = pRawGetFunction;
    pSetAwarenessFunction = pRawSetFunction;
    #ifdef __MINGW32__
    #pragma GCC diagnostic pop
    #endif
    
    return ES_ERROR_SUCCESS;
}


enum EsError updateAccordingToDpi(HWND hWindow, const RECT *pWindowRect) { // Updates teh window position and sized based on the DPI of the display
    unsigned int dpi, dpiX, dpiY, dpiWidth, dpiHeight;
    
    // Cast into the function signature of the 
    // `SetProcessDpiAwarenessContext` function.
    if (!pSetAwarenessFunction(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2)) {
        return ES_ERROR_FAILED_DPI;
        
    }
    
    // Cast into the function signature of the `GetDpiForWindow` 
    // function.
    dpi = pGetDpiFunction(hWindow);
    
    // Translate the editor's coordinate system to the coordinate 
    // system native to the host system. This translation takes 
    // the system's characteristic DPI into consideration.
    dpiX = MulDiv(pWindowRect->left, dpi, USER_DEFAULT_SCREEN_DPI);
    dpiY = MulDiv(pWindowRect->top, dpi, USER_DEFAULT_SCREEN_DPI);
    dpiWidth = MulDiv(pWindowRect->right - pWindowRect->left, dpi, 
        USER_DEFAULT_SCREEN_DPI);
    dpiHeight = MulDiv(pWindowRect->bottom - pWindowRect->top, dpi, 
        USER_DEFAULT_SCREEN_DPI);
    
    SetWindowPos(hWindow, hWindow, dpiX, dpiY, dpiWidth, dpiHeight, 
        SWP_NOZORDER|SWP_NOACTIVATE);
    
    return ES_ERROR_SUCCESS;
}