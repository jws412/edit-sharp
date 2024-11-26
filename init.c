#include "dpi_manager.h"
#include "init.h"

HWND initEditor(LRESULT (*pProcedure)(HWND, unsigned int, WPARAM, LPARAM)) {
    const RECT initialWindowRect = {0, 0, 720, 540};
    HMODULE hEditorModule;
    if (!GetModuleHandleEx(0, NULL, &hEditorModule)) {
        return NULL;
        
    }
    
    WNDCLASSEX editorWindowClass = {
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
    
    HWND hEditorWindow = CreateWindowEx(
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
    updateAccordingToDpi(hEditorWindow, &initialWindowRect);
    
    return hEditorWindow;
}