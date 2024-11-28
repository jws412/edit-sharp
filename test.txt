#include <stdio.h>
#include "global_data.h"
#include "init.h"
#include "memory_manager.h"

LRESULT editorProcedure(HWND windowHandle, unsigned int messageId, 
    WPARAM primary, LPARAM secondary);

int main(void) {
    HWND hEditor = initEditor(&editorProcedure);
    
    if (hEditor == NULL) {
        PANIC("Failed to initialize Edit#.");
        return ES_ERROR_FAILED_INITIALIZATION;
    }
    
    MSG currentMessage;
    while (GetMessage(&currentMessage,NULL,0,0) > 0) {
        DispatchMessage(&currentMessage);
    }
    
    
    return ES_ERROR_SUCCESS;
}

LRESULT editorProcedure(HWND hWindow,
        unsigned int messageId,
        WPARAM wParam,
        LPARAM lParam) {
    
    #include "dpi_manager.h"
    
    static HBRUSH hBackgroundBrush = NULL;      // Text background.
    static HBRUSH hLineCounterBrush = NULL;     // Line counter brush.
    static HBRUSH hLineHighlightBrush = NULL;   // Line focus brush.
    static HDC hEditorDC = NULL;                // The editor's DC.
    static HDC hViewportDC = NULL;              // The viewport's DC.
    static HFONT hLabellingFont = NULL;         // Font for menu items.
    static HFONT hMonospaceFont = NULL;         // Font for code.
    
    static unsigned int titlebarHeight = 0;     // Titlebar height.
    static RECT currentWindowRect = { 0 };      // Current window size.
    static unsigned int editorWidth = 0, editorHeight = 0;
    static sEditorState editorState = { 0 };    // Target of change.
    
    #define INITIALIZE_TO_LASTEST_HIGHLIGHT_RECTANGLE() {   \
        .left = ES_LAYOUT_LINECOUNT_WIDTH,                  \
        .top = ES_LAYOUT_LINECOUNT_FONT_HEIGHT              \
            *(editorState.relativeFocusLineIndex),          \
        .right = editorWidth,                               \
        .bottom = ES_LAYOUT_LINECOUNT_FONT_HEIGHT           \
            *(editorState.relativeFocusLineIndex + 1)}
    
    switch(messageId) {
        
        case WM_CREATE: {
            
            // Fetch the device context of the editor's window.
            hEditorDC = GetWindowDC(hWindow);
            // Fetch the device context of the viewport.
            hViewportDC = GetDC(hWindow);
            hBackgroundBrush = CreateSolidBrush(ES_COLOR_BACKGROUND);
            hLineCounterBrush = CreateSolidBrush(ES_COLOR_LINECOUNT);
            hLineHighlightBrush = CreateSolidBrush(ES_COLOR_HIGHLIGHT);
            hLabellingFont = CreateFont(
                ES_LAYOUT_LINECOUNT_FONT_HEIGHT,
                ES_LAYOUT_LINECOUNT_FONT_WIDTH,
                0,
                0,
                400, /*Font weight*/
                FALSE, /*Italicized*/
                FALSE, /*Underlined*/
                FALSE, /*Strikeout*/
                ANSI_CHARSET,
                OUT_DEFAULT_PRECIS, /*Output precision*/
                CLIP_DEFAULT_PRECIS, /*Clipping precision*/
                ANTIALIASED_QUALITY,
                FF_DONTCARE, /*Font pitch and family*/
                TEXT("Microsoft Sans Serif"));
            hMonospaceFont = CreateFont(
                ES_LAYOUT_LINECOUNT_FONT_HEIGHT,
                ES_LAYOUT_LINECOUNT_FONT_WIDTH,
                0,
                0,
                400,
                FALSE,
                FALSE,
                FALSE,
                ANSI_CHARSET,
                OUT_DEFAULT_PRECIS,
                CLIP_DEFAULT_PRECIS,
                ANTIALIASED_QUALITY,
                DEFAULT_PITCH|FF_DONTCARE,
                TEXT("Courier New"));
            
            /*XXX: Make DPI aware?*/
            titlebarHeight = GetSystemMetrics(SM_CYFRAME)
                + GetSystemMetrics(SM_CYCAPTION) 
                + GetSystemMetrics(SM_CXPADDEDBORDER);
            
            if (loadFileIntoEditorState("test.txt", &editorState)
                    != ES_ERROR_SUCCESS) {
                PANIC("The file to edit does not exist.");
                
            }
            
            break;
        }
        
        // Take care of drawing the background of the text editor.
        case WM_CTLCOLORSTATIC: {
            SetBkColor(hEditorDC, ES_COLOR_BACKGROUND);
            return (INT_PTR) hBackgroundBrush;
        }
        
        case WM_DESTROY: {
            ReleaseDC(hWindow, hEditorDC);
            ReleaseDC(hWindow, hViewportDC);
            DeleteObject(hLabellingFont);
            DeleteObject(hBackgroundBrush);
            DeleteObject(hLineCounterBrush);
            DeleteObject(hLineHighlightBrush);
            DeleteObject(hMonospaceFont);
            
            /*XXX: Go to each deque and free its nodes!!!! Then free 
            the array.*/
            free(editorState.dequeArr);
            
            PostQuitMessage(0);
            break;
        }
        
        case WM_DPICHANGED: {
            if (updateAccordingToDpi(hWindow, &currentWindowRect) 
                    != ES_ERROR_SUCCESS) {
                PANIC("The DPI adjustment procedure failed.");
                
            }
            break;
        }
        
        case WM_LBUTTONDOWN: {
            
            #include "input_handler.h"
            
            RECT highlightRect = 
                INITIALIZE_TO_LASTEST_HIGHLIGHT_RECTANGLE();
            
            // Paint over the old line selection highlight.
            FillRect(hViewportDC, &highlightRect, hBackgroundBrush);
            
            // The `lParam` parameter describes the position of the 
            // user's cursor.
            updateStateAccordingToClick(&editorState, lParam);
            
            highlightRect.top = ES_LAYOUT_LINECOUNT_FONT_HEIGHT
                *(editorState.relativeFocusLineIndex);
            highlightRect.bottom = ES_LAYOUT_LINECOUNT_FONT_HEIGHT
                *(editorState.relativeFocusLineIndex+1);
            FillRect(hViewportDC, &highlightRect, hLineHighlightBrush);
            InvalidateRect(hWindow, NULL, FALSE);
            
            break;
        }
        
        case WM_PAINT: {
            
            // Storage for layouts in the rendering process.
            static PAINTSTRUCT ps;
            RECT lineCounterRect = {
                .left = 0, 
                .top = 0, 
                .right = ES_LAYOUT_LINECOUNT_WIDTH, 
                .bottom = editorHeight};
            RECT lineHighlightRect = 
                INITIALIZE_TO_LASTEST_HIGHLIGHT_RECTANGLE();
            RECT codeLineRect = {
                .left = ES_LAYOUT_LINECOUNT_WIDTH,
                .top = 0,
                .right = editorWidth,
                .bottom = editorHeight};
            
            // Storage for text line counts.
            const unsigned int visibleLines = editorHeight 
                / ES_LAYOUT_LINECOUNT_FONT_HEIGHT;
            unsigned long lineIndex;
            sLineNode *pNode = editorState.dequeArr->pHead;
            
            // Storage for local renderer references.
            HDC hCanvas = BeginPaint(hWindow, &ps);
            
            
            // Paint the line counter on the right side of the editor.
            
            
            FillRect(hViewportDC, &lineCounterRect, hLineCounterBrush);
            
            
            // Render the line in focus
            
            
            FillRect(hViewportDC, &lineHighlightRect, hLineHighlightBrush);
            
            
            // Render line counter text on the line counter.
            
            
            SelectObject(ps.hdc, hMonospaceFont);
            SetTextColor(hCanvas, ES_COLOR_WHITE);
            SetBkMode(hCanvas, TRANSPARENT);
            lineCounterRect.left = 8;
            for (lineIndex = 0; lineIndex++ < visibleLines;) {
                char lineNumberTextBuffer[21]; // Assume 64-bit int.
                int successCode;
                
                // Calls to the `sprintf` function automatically 
                // inserts a null terminator character.
                sprintf(lineNumberTextBuffer, "%ld", 
                    editorState.firstVisibleLineIndex+lineIndex);
                
                // Draw the line counter's text line.
                successCode = DrawText(hCanvas, lineNumberTextBuffer, -1,
                    &lineCounterRect, DT_SINGLELINE|DT_NOCLIP);
                
                // Draw the code line.
                if (pNode != NULL) {
                    successCode = successCode
                        && DrawText(hCanvas, pNode->line.pStart, -1,
                        &codeLineRect, DT_SINGLELINE|DT_NOCLIP|DT_NOPREFIX);
                    pNode = pNode->pNext;
                    
                }
                if (successCode == 0) {
                    PANIC("The renderer failed to generate text.");
                    
                }
                
                lineCounterRect.top += ES_LAYOUT_LINECOUNT_FONT_HEIGHT;
                codeLineRect.top += ES_LAYOUT_LINECOUNT_FONT_HEIGHT;
            }
            
            EndPaint(hWindow, &ps);
            ReleaseDC(hWindow, hCanvas);
            break;
        }
        
        case WM_MOVE: {
            
            // Query for redrawing the window. The new position of the 
            // window may have portions that are out of view. This 
            // case causes text to fail to render.
            InvalidateRect(hWindow, NULL, FALSE);
        }
        // fall-thru
        case WM_SIZE: {
            
            // Update the rectangle for the window whenever the user 
            // resizes the window.
            GetWindowRect(hWindow, &currentWindowRect);
            editorWidth = currentWindowRect.right - currentWindowRect.left;
            editorHeight = currentWindowRect.bottom - currentWindowRect.top
                - titlebarHeight;
            break;
        }
        
        default: {
            return DefWindowProc(hWindow, messageId, wParam, lParam);
        }
    }
    
    #undef INITIALIZE_TO_LASTEST_HIGHLIGHT_RECTANGLE
    
    return ERROR_SUCCESS;
}