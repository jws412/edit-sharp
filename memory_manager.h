#include "global_data.h"

#ifndef _HEADER_MEMORY_MANAGER

typedef struct {
    char *pStart;
    unsigned int characters;
} sLine;

typedef struct LineNode {
    struct LineNode *pPrev;
    struct LineNode *pNext;
    sLine line;
} sLineNode;

typedef struct LineDeque {
    unsigned long lines;
    HANDLE hFile;
    sLineNode *pHead;
    sLineNode *pTail;
    sWriteHead writeHead;
} sLineDeque;

enum EsError loadFileIntoEditorState(const char *pFilepath, 
    sEditorState *pEditorState);
sLineNode *constructLineNode(unsigned int characters);

#define _HEADER_MEMORY_MANAGER
#endif
