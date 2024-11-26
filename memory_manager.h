#include "global_data.h"

enum EsError loadFileIntoEditorState(const char *pFilepath, 
    sEditorState *pEditorState);

typedef struct {
    unsigned long characters;
    char *pStart;
} sLine;

typedef struct _LineNode {
    sLine line;
    struct _LineNode *pPrev;
    struct _LineNode *pNext;
} sLineNode;

typedef struct LineDeque {
    unsigned long lines;
    HANDLE hFile;
    sLineNode *pHead;
    sLineNode *pTail;
} sLineDeque;