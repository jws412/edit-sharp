#include "dpi_manager.h"
#include "init.h"

HWND initEditor(LRESULT (*pProcedure)(HWND, unsigned int, WPARAM, LPARAM)) { // Initialize the Editor program based off the dpi_manager.c class used to create windows
    const RECT initialWindowRect = {0, 0, 720, 540}; // Initialize window at 720 x 540 resolution
    HMODULE hEditorModule; // imports a handler module for the .exe program
    if (!GetModuleHandleEx(0, NULL, &hEditorModule)) {
        return NULL;
        
    }
    
    WNDCLASSEX editorWindowClass = { // Registers the structure that defines the properties of the window class, there information can be found on the global_data.h file
        .cbSize = sizeof(WNDCLASSEX),
        .style = CS_HREDRAW|CS_VREDRAW,
        .lpfnWndProc = pProcedure,
        .cbClsExtra = 0,
        .cbWndExtra = 0,
        .hInstance = hEditorModule,
        .hIcon = NULL,
        .hIconSm = NULL,
        .hCursor = LoadCursor(NULL, IDC_ARROW),
        .hbrBackground = CreateSolidBrush(ES_COLOR_BACKGROUND),
        .lpszMenuName = MENU_NAME,
        .lpszClassName = CLASS_NAME,
    };
    if (!RegisterClassEx(&editorWindowClass)) {
        return NULL;
        
    }
    
    HWND hEditorWindow = CreateWindowEx( // Create the window using the registered window class
        WS_EX_APPWINDOW,
        CLASS_NAME,
        HEADER_NAME,
        WS_VISIBLE|WS_TILEDWINDOW,
        initialWindowRect.left /*Horizontal position.*/,
        initialWindowRect.top /*Vertical position.*/,
        initialWindowRect.right,
        initialWindowRect.bottom,
        NULL /*Handle to the parent window.*/,
        NULL /*Child window functionality.*/,
        hEditorModule,
        NULL /*Unessary pointer.*/);
    
    if (initDpiManager() != ES_ERROR_SUCCESS) {
        return NULL;
        
    }
    updateAccordingToDpi(hEditorWindow, &initialWindowRect); // Apply DPI settings to the window
    
    return hEditorWindow;
}