#include <stdio.h>
#include "input_handler.h"

void updateStateAccordingToClick(sEditorState* const pEditorState,
        const LPARAM lParam) {
    
    const unsigned int clickX = lParam & 0x0000FFFF;
    const unsigned int clickY = lParam >> 16;
    
    pEditorState->relativeFocusLineIndex = clickY /
        ES_LAYOUT_LINECOUNT_FONT_HEIGHT;
    return;
}