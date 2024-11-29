#include <stdio.h>
#include "global_data.h"
#include "init.h"

LRESULT editorProcedure(HWND windowHandle, unsigned int messageId, 
    WPARAM primary, LPARAM secondary);

int main(void) {
    HWND hEditor = initEditor(&editorProcedure); // Calls the initialization function of initEditor() from init.h which create the GUI of the text editor
    
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

void jumpHead(sEditorState *pState, const RECT *pRefreshRectangle);

LRESULT editorProcedure(HWND hWindow, // Method that processes messages sent by the editor's windows to establish properties like brushes, fonts, colors, etc.
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
    
    switch(messageId) { // Different use cases based on the received messaged by the editor
        
        case WM_CREATE: { // Initialize resources needed for the editor (font, brushes, etc.)
            
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
        
        // Cleans up resources when the editor is closed to free up memory
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
        
        // Updates DPI based on the display the program is ran on
        case WM_DPICHANGED: {
            if (updateAccordingToDpi(hWindow, &currentWindowRect) 
                    != ES_ERROR_SUCCESS) {
                PANIC("The DPI adjustment procedure failed.");
                
            }
            break;
        }
        
        // Handles keyboard inputs from the user
        case WM_KEYDOWN: {
            
            RECT refreshRectangle = {
                .left = ES_LAYOUT_LINECOUNT_WIDTH,
                .top = editorState.curHighlight.top,
                .right = editorWidth,
                .bottom = editorHeight};
            
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
                    
                    ++editorState.prevHighlight.relativeFocusLineIndex;
                    editorState.prevHighlight.top += 
                        ES_LAYOUT_LINECOUNT_FONT_HEIGHT;
                    ++editorState.curHighlight.relativeFocusLineIndex;
                    editorState.curHighlight.top += 
                        ES_LAYOUT_LINECOUNT_FONT_HEIGHT;
                    
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
                        
                        refreshRectangle.top -= 
                            ES_LAYOUT_LINECOUNT_FONT_HEIGHT;
                        editorState.curHighlight.top -= 
                            ES_LAYOUT_LINECOUNT_FONT_HEIGHT;
                        --editorState.curHighlight.relativeFocusLineIndex;
                        
                    }
                    
                    break;
                }
                
            }
            
            InvalidateRect(hWindow, &refreshRectangle, TRUE);
            break;
        }
        
        // Handles left mouse click actions from the user
        case WM_LBUTTONDOWN: {
            
            #include "input_handler.h"
            
            RECT refreshRectangle;
            
            // The `lParam` parameter describes the position of the 
            // user's cursor.
            refreshRectangle = updateStateAccordingToClick(&editorState, 
                editorWidth, lParam);
            jumpHead(&editorState, &refreshRectangle);
            InvalidateRect(hWindow, &refreshRectangle, FALSE);
            break;
        }
        
        // Renders the content in the editor
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
        
        // Handles the dimensions of the window when its being resized (scaling)
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

// Move from line to line in the editor based on the highlighted section of the editor by determining the position of the double linked list
// if we want to go up 1 line, it will read the next index of the double linked list
// else we want to go down 1 line, it will read the previous index of the double linked list
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