#include "input_handler.h"

void updateStateAccordingToClick(sEditorState* const pEditorState,
        const LPARAM lParam) {
    
    const unsigned int clickX = 0xFFFF&lParam, clickY = lParam >> 16;
    
    pEditorState->prevHighlight = pEditorState->curHighlight;
    pEditorState->curHighlight.relativeFocusLineIndex = clickY /
        ES_LAYOUT_LINECOUNT_FONT_HEIGHT;
    pEditorState->curHighlight.top = ES_LAYOUT_LINECOUNT_FONT_HEIGHT
        * pEditorState->curHighlight.relativeFocusLineIndex;
    return;
}