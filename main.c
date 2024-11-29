#include <stdio.h>
#include "global_data.h"
#include "init.h"

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

#include "memory_manager.h"
#include "dpi_manager.h"

LRESULT editorProcedure(HWND hWindow,
        unsigned int messageId,
        WPARAM wParam,
        LPARAM lParam) {
    
    static HBRUSH hBackgroundBrush = NULL;      // Text background.
    static HBRUSH hLineCounterBrush = NULL;     // Line counter brush.
    static HBRUSH hLineHighlightBrush = NULL;   // Line focus brush.
    static HDC hEditorDC = NULL;                // The editor's DC.
    static HDC hViewportDC = NULL;              // The viewport's DC.
    static HFONT hLabellingFont = NULL;         // Font for menu items.
    static HFONT hMonospaceFont = NULL;         // Font for code.
    
    static unsigned short titlebarHeight = 0;     // Titlebar height.
    static unsigned short editorWidth = 0, editorHeight = 0;
    static RECT currentWindowRect = { 0 };      // Current window size.
    static sEditorState editorState = { 0 };    // System to change.
    static sLineNode *pWriteHead = NULL;
    
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
            
            /*Consider multiple open files.*/
            pWriteHead = editorState.dequeArr[0].pHead;
            
            break;
        }
        
        // Take care of drawing the background of the text editor.
        case WM_CTLCOLORSTATIC: {
            SetBkColor(hEditorDC, ES_COLOR_BACKGROUND);
            return (LRESULT) hBackgroundBrush;
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
        
        case WM_KEYDOWN: {
            switch(wParam) {
                
                case VK_RETURN: {
                    sLineNode *pBehind = pWriteHead->pNext;
                    sLineNode *pAddition = constructLineNode(0);
                    RECT refreshRectangle = {
                        .left = ES_LAYOUT_LINECOUNT_WIDTH,
                        .top = editorState.curHighlight.top,
                        .right = editorWidth,
                        .bottom = editorHeight};
                    
                    pAddition->pPrev = pWriteHead;
                    pAddition->pNext = pBehind;
                    
                    pWriteHead->pNext = pAddition;
                    pBehind->pPrev = pAddition;
                    
                    pWriteHead = pAddition;
                    
                    ++editorState.prevHighlight.relativeFocusLineIndex;
                    editorState.prevHighlight.top += 
                        ES_LAYOUT_LINECOUNT_FONT_HEIGHT;
                    ++editorState.curHighlight.relativeFocusLineIndex;
                    editorState.curHighlight.top += 
                        ES_LAYOUT_LINECOUNT_FONT_HEIGHT;
                    
                    InvalidateRect(hWindow, &refreshRectangle, TRUE);
                    break;
                }
                
            }
            
            break;
        }
        
        case WM_LBUTTONDOWN: {
            
            #include "input_handler.h"
            
            RECT refreshRectangle;
            signed char jumps;
            
            // The `lParam` parameter describes the position of the 
            // user's cursor.
            updateStateAccordingToClick(&editorState, lParam);
            
            refreshRectangle.left = ES_LAYOUT_LINECOUNT_WIDTH;
            refreshRectangle.right = editorWidth;
            
            if (editorState.curHighlight.top 
                    > editorState.prevHighlight.top) {
                
                refreshRectangle.top = editorState.prevHighlight.top;
                refreshRectangle.bottom = editorState.curHighlight.top
                    + ES_LAYOUT_LINECOUNT_FONT_HEIGHT;
                    
            } else {
                refreshRectangle.top = editorState.curHighlight.top;
                refreshRectangle.bottom = editorState.prevHighlight.top
                    + ES_LAYOUT_LINECOUNT_FONT_HEIGHT;
                
            }
            
            jumps = editorState.curHighlight.relativeFocusLineIndex
                - editorState.prevHighlight.relativeFocusLineIndex;
            
            if (jumps > 0) {
                while (jumps--) pWriteHead = pWriteHead->pNext;
                
            } else {
                while (jumps++) pWriteHead = pWriteHead->pPrev;
                
            }
            
            InvalidateRect(hWindow, &refreshRectangle, FALSE);
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
            RECT codeLineRect = {
                .left = ES_LAYOUT_LINECOUNT_WIDTH,
                .top = 0,
                .right = editorWidth,
                .bottom = editorHeight};
            
            // Storage for text line counts.
            const unsigned int visibleLines = editorHeight 
                / ES_LAYOUT_LINECOUNT_FONT_HEIGHT;
            const sLineNode *pNode = editorState.dequeArr->pHead;
            
            // Storage for local renderer references.
            HDC hCanvas = BeginPaint(hWindow, &ps);
            
            // Setup selections for drawing rectangles.
            const HPEN hPrevPen = SelectObject(hCanvas, 
                GetStockObject(DC_PEN));
            const HBRUSH hPrevBrush = SelectObject(hCanvas, 
                hLineCounterBrush);
            
            // Paint the line counter on the right side of the editor.
            SetDCPenColor(hCanvas, ES_COLOR_LINECOUNT);
            Rectangle(hCanvas, lineCounterRect.left, lineCounterRect.top,
                lineCounterRect.right, lineCounterRect.bottom);
            
            // Paint over the previous highlight.
            SetDCPenColor(hCanvas, ES_COLOR_BACKGROUND);
            SelectObject(hCanvas, hBackgroundBrush);
            Rectangle(hCanvas, 
                ES_LAYOUT_LINECOUNT_WIDTH, 
                editorState.prevHighlight.top,
                editorWidth,
                editorState.prevHighlight.top
                + ES_LAYOUT_LINECOUNT_FONT_HEIGHT);
            
            // Render the line in focus.
            SetDCPenColor(hCanvas, ES_COLOR_HIGHLIGHT);
            SelectObject(hCanvas, hLineHighlightBrush);
            Rectangle(hCanvas, 
                ES_LAYOUT_LINECOUNT_WIDTH, 
                editorState.curHighlight.top,
                editorWidth, 
                editorState.curHighlight.top
                + ES_LAYOUT_LINECOUNT_FONT_HEIGHT);
            
            // Render line counter text on the line counter.
            SelectObject(ps.hdc, hMonospaceFont);
            SetTextColor(hCanvas, ES_COLOR_WHITE);
            SetBkMode(hCanvas, TRANSPARENT);
            lineCounterRect.left = 8;
            for (unsigned long lineIndex = 0; lineIndex++ < visibleLines;) {
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
            
            // Undo all selections and finish rendering.
            SelectObject(hCanvas, hPrevPen);
            SelectObject(hCanvas, hPrevBrush);
            EndPaint(hWindow, &ps);
            ReleaseDC(hWindow, hCanvas);
            break;
        }
        
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
    
    return ERROR_SUCCESS;
}