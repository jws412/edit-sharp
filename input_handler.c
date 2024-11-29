#include "input_handler.h"

RECT updateStateAccordingToClick(sEditorState* const pEditorState,
        const unsigned short windowWidth,
        const LPARAM lParam) {
    
    const unsigned int clickX = 0xFFFF&lParam, clickY = lParam >> 16;
    RECT refreshRectangle;
    
    
    pEditorState->prevHighlight = pEditorState->curHighlight;
    pEditorState->curHighlight.relativeFocusLineIndex = clickY /
        ES_LAYOUT_LINECOUNT_FONT_HEIGHT;
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
    
    pEditorState->pActiveHead->index = clickX >= ES_LAYOUT_LINECOUNT_WIDTH ?
        clickX / ES_LAYOUT_LINECOUNT_FONT_WIDTH : 0;
    
    return refreshRectangle;
}