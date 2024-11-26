#include <windows.h>
#include "dpi_manager.h"

static BOOL (*pSetAwarenessFunction)(DPI_AWARENESS_CONTEXT) = NULL;
static unsigned int (*pGetDpiFunction)(HWND) = NULL;

enum EsError initDpiManager(void) {
    
    #ifdef __MINGW32__
    #pragma GCC diagnostic push
    #pragma GCC diagnostic ignored "-Wpedantic"
    #endif
    void *pRawGetFunction = 
        GetProcAddress(GetModuleHandle(TEXT("user32.dll")),
        "GetDpiForWindow");
    void *pRawSetFunction = 
        GetProcAddress(GetModuleHandle(TEXT("user32.dll")),
        "SetProcessDpiAwarenessContext");
    
    if (pRawGetFunction == NULL || pRawSetFunction == NULL) {
        return ES_ERROR_FAILED_DPI;
        
    }
    
    pGetDpiFunction = pRawGetFunction;
    pSetAwarenessFunction = pRawSetFunction;
    #ifdef __MINGW32__
    #pragma GCC diagnostic pop
    #endif
    
    return ES_ERROR_SUCCESS;
}


enum EsError updateAccordingToDpi(HWND hWindow, const RECT *pWindowRect) {
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