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

RECT updateHighlight(sEditorState* pEditorState,
        const unsigned short windowWidth,
        const unsigned short curRelativeIndex);
void jumpHead(sEditorState *pState, const RECT *pRefreshRectangle);

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
            
            /*XXX: Consider multiple open files.*/
            editorState.pActiveHead = &(editorState.dequeArr[0].writeHead);
            
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
            
            RECT refreshRectangle;
            
            switch(wParam) {
                
                case VK_RETURN: {
                    sLineNode *pBehind = 
                        editorState.pActiveHead->pNode->pNext;
                    sLineNode *pAddition = constructLineNode(0);
                    
                    pAddition->pPrev = editorState.pActiveHead->pNode;
                    pAddition->pNext = pBehind;
                    
                    editorState.pActiveHead->pNode->pNext = pAddition;
                    pBehind->pPrev = pAddition;
                    
                    editorState.pActiveHead->pNode = pAddition;
                    
                    refreshRectangle = updateHighlight(&editorState, 
                        editorWidth, 
                        editorState.curHighlight.relativeFocusLineIndex+1);
                    refreshRectangle.bottom = editorHeight;
                    
                    break;
                }
                
                
                case VK_BACK: {
                    
                    if (editorState.pActiveHead->pNode->line.characters==0
                            && editorState.pActiveHead->pNode->pPrev!=NULL) {
                        
                        sLineNode *pTarget = editorState.pActiveHead->pNode;
                        sLineNode *pOtherEnd = pTarget->pNext;
                        
                        // Move the node that the write head points to back
                        // one line.
                        editorState.pActiveHead->pNode = 
                            editorState.pActiveHead->pNode->pPrev;
                        
                        editorState.pActiveHead->pNode->pNext = pOtherEnd;
                        pOtherEnd->pPrev = editorState.pActiveHead->pNode;
                        
                        free(pTarget);
                        
                        editorState.prevHighlight.relativeFocusLineIndex
                            = editorState.curHighlight.relativeFocusLineIndex;
                        editorState.prevHighlight.top = 
                            editorState.curHighlight.top;
                        
                        refreshRectangle = updateHighlight(&editorState, 
                            editorWidth, 
                            editorState.curHighlight.relativeFocusLineIndex
                            - 1);
                        refreshRectangle.bottom = editorHeight;
                        
                        break;
                        
                    }
                    
                    // Short-cut to the end to avoid rendering.
                    return ERROR_SUCCESS;
                }
                
                case VK_UP: {
                    
                    if (editorState.pActiveHead->pNode->pPrev != NULL) {
                        
                        editorState.pActiveHead->pNode = 
                            editorState.pActiveHead->pNode->pPrev;
                        refreshRectangle = updateHighlight(&editorState, 
                            editorWidth, 
                            editorState.curHighlight.relativeFocusLineIndex
                            - 1);
                        break;
                        
                    }
                    return ERROR_SUCCESS;
                }
                case VK_DOWN: {
                    
                    if (editorState.pActiveHead->pNode->pNext != NULL) {
                        
                        editorState.pActiveHead->pNode = 
                            editorState.pActiveHead->pNode->pNext;
                        refreshRectangle = updateHighlight(&editorState, 
                            editorWidth, 
                            editorState.curHighlight.relativeFocusLineIndex
                            + 1);
                        break;
                    }
                    return ERROR_SUCCESS;
                }
                
            }
            
            InvalidateRect(hWindow, &refreshRectangle, TRUE);
            break;
        }
        
        case WM_LBUTTONDOWN: {
            
            // The `lParam` parameter describes the position of the 
            // user's cursor.
            const unsigned short clickX = (0xFFFF & lParam), 
                clickY = (lParam >> 16);
            const unsigned short lines = clickY
                / ES_LAYOUT_LINECOUNT_FONT_HEIGHT;
            RECT refreshRectangle;
            
            refreshRectangle = updateHighlight(&editorState, 
                editorWidth, clickY/ES_LAYOUT_LINECOUNT_FONT_HEIGHT);
            jumpHead(&editorState, &refreshRectangle);
            
            editorState.pActiveHead->characterIndex = 
                clickX >= ES_LAYOUT_LINECOUNT_WIDTH ?
                clickX / ES_LAYOUT_LINECOUNT_FONT_WIDTH : 0;
            
            InvalidateRect(hWindow, &refreshRectangle, FALSE);
            break;
        }
        
        case WM_PAINT: {
            // Storage for layouts in the rendering process.
            PAINTSTRUCT ps;
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
            const sLineNode *pNode = editorState.pActiveHead->pNode;
            signed long jumps = editorState.pActiveHead->lineIndex
                - editorState.firstVisibleLineIndex;
            
            if (jumps > 0) {
                while (jumps--) pNode = pNode->pPrev;
                
            } else {
                while (jumps++) pNode = pNode->pNext;
                
            }
            
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
                        && DrawText(hCanvas, pNode->line.pStart, 
                        pNode->line.characters+1, &codeLineRect, 
                        DT_SINGLELINE|DT_NOCLIP|DT_NOPREFIX);
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
        
        case WM_MOUSEWHEEL: {
            
            const signed short jumps = -GET_WHEEL_DELTA_WPARAM(wParam)
                / ES_SCROLL_NUMBNESS;
            const signed long update = editorState.firstVisibleLineIndex
                + jumps;
            
            /*XXX: Consider multiple files later on.*/
            if (update>=0
                    && update<(signed long)editorState.dequeArr[0].lines) {
                editorState.firstVisibleLineIndex = update;
            }
            
            InvalidateRect(hWindow, NULL, TRUE);
            
            break;
        }
        
        default: {
            return DefWindowProc(hWindow, messageId, wParam, lParam);
        }
    }
    
    return ERROR_SUCCESS;
}

void jumpHead(sEditorState *pState, const RECT *pRefreshRectangle) {
    
    sLineNode *pNode = pState->pActiveHead->pNode;
    
    if (pState->curHighlight.relativeFocusLineIndex 
            > pState->prevHighlight.relativeFocusLineIndex) {
    
        long int top = pRefreshRectangle->top;
        
        while ((top += ES_LAYOUT_LINECOUNT_FONT_HEIGHT)
                != pRefreshRectangle->bottom && pNode->pNext != NULL) {
            
            pNode = pNode->pNext;
        }
        
    } else {
        
        long int bottom = pRefreshRectangle->bottom;
        
        while ((bottom -= ES_LAYOUT_LINECOUNT_FONT_HEIGHT)
                != pRefreshRectangle->top && pNode->pPrev != NULL) {
            
            pNode = pNode->pPrev;
        }
        
    }
    
    pState->pActiveHead->pNode = pNode;
    return;
}

RECT updateHighlight(sEditorState* pEditorState,
        const unsigned short windowWidth,
        const unsigned short curRelativeIndex) {
    
    RECT refreshRectangle;
    
    pEditorState->prevHighlight = pEditorState->curHighlight;
    pEditorState->curHighlight.relativeFocusLineIndex = curRelativeIndex;
    pEditorState->curHighlight.top = ES_LAYOUT_LINECOUNT_FONT_HEIGHT
        * pEditorState->curHighlight.relativeFocusLineIndex;
    
    refreshRectangle.left = ES_LAYOUT_LINECOUNT_WIDTH;
    refreshRectangle.right = windowWidth;
    if (pEditorState->curHighlight.top 
            > pEditorState->prevHighlight.top) {
        
        refreshRectangle.top = pEditorState->prevHighlight.top;
        refreshRectangle.bottom = pEditorState->curHighlight.top
            + ES_LAYOUT_LINECOUNT_FONT_HEIGHT;
            
    } else {
        refreshRectangle.top = pEditorState->curHighlight.top;
        refreshRectangle.bottom = pEditorState->prevHighlight.top
            + ES_LAYOUT_LINECOUNT_FONT_HEIGHT;
        
    }
    
    // Update the index for the write head.
    pEditorState->pActiveHead->lineIndex += curRelativeIndex
        - pEditorState->prevHighlight.relativeFocusLineIndex;
    
    return refreshRectangle;
}