#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#ifndef _HEADER_GLOBAL_DATA
#define MENU_NAME "EDITSHARP"
#define CLASS_NAME MENU_NAME "_WINDOWCLASS"
#define HEADER_NAME "Edit#"

#define ES_LAYOUT_LINECOUNT_FONT_HEIGHT 20
#define ES_LAYOUT_LINECOUNT_FONT_WIDTH (ES_LAYOUT_LINECOUNT_FONT_HEIGHT/2)
#define ES_LAYOUT_LINECOUNT_WIDTH (5*ES_LAYOUT_LINECOUNT_FONT_WIDTH)

#define ES_COLOR_BACKGROUND RGB(10,15,30)
#define ES_COLOR_LINECOUNT RGB(40,50,60)
#define ES_COLOR_HIGHLIGHT RGB(25,30,45)
#define ES_COLOR_WHITE RGB(255,255,255)

#define ES_SCROLL_NUMBNESS 17

#define PANIC(message) (MessageBox(NULL, message, NULL, MB_OK), PostQuitMessage(0), (void) 0)

enum EsError {
    ES_ERROR_SUCCESS,
    ES_ERROR_FAILED_INITIALIZATION,
    ES_ERROR_FAILED_DPI,
    ES_ERROR_FILE_NOT_FOUND,
    ES_ERROR_PARSING_ERROR,
    ES_ERROR_ALLOCATION_FAIL,
};

typedef struct WriteHead {
    struct LineNode *pNode;
    unsigned long lineIndex;
    unsigned int characterIndex;
} sWriteHead;

typedef struct {
    struct LineDeque *dequeArr;
    sWriteHead *pActiveHead;
    unsigned long firstVisibleLineIndex;
    struct {
        unsigned short relativeFocusLineIndex;
        unsigned short top;
    } prevHighlight, curHighlight;
} sEditorState;

#define _HEADER_GLOBAL_DATA
#endif