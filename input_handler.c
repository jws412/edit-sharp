#include <stdio.h>
#include "input_handler.h"


void updateStateAccordingToClick(sEditorState* const pEditorState,
        const LPARAM lParam) {
    
    const unsigned int clickY = lParam >> 16;
    
    pEditorState->prevRelativeFocusLineIndex =
        pEditorState->curRelativeFocusLineIndex;
    pEditorState
    return;
}