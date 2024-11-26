#include <stdio.h>
#include <stdlib.h>
#include "memory_manager.h"

// Lines in source code never exceed 79 characters according to 
// the programmer's experience. Two extra characters constitute a
// carriage return-line feed pair. However, a line necessarily ends in 
// such a pair. As such, the line can omit these characters. However, 
// the buffer must include these characters to properly detect them. 
// An additional character is necessary to consider the null character.
#define BUFFER_CHARACTERS 82

#define ARRAY_SIZE(arr) (sizeof(arr)/sizeof((arr)[0]))
#define SALLOC(s) (malloc(sizeof(s)))

sLineNode *constructLineNode(const unsigned int characters, 
    const char *pBuffer);
void appendNodeToDeque(sLineNode *pNode, sLineDeque *pDeque);

// Debug functions
void printDeque(sLineDeque *pDeque);

enum EsError loadFileIntoEditorState(const char *pFilepath, 
        sEditorState *pEditorState) {
    
    char buffer[BUFFER_CHARACTERS] = { 0 };     // Buffer for file.
    sLine dynamicBuffer;                        // Persistent buffer.
    sLineDeque *pFileLineDeque;                 // Line deque of file.
    HANDLE hFile;                               // Handle to file.
    unsigned long int outputCharacters;         // Characters to read.
    unsigned int files;                         // Open file amount.
    
    // Remember to call the `CloseHandle` function to close the file.
    hFile = CreateFile(pFilepath, 
        GENERIC_READ|GENERIC_WRITE,
        0, /*Does not allow file sharing.*/
        NULL, /*Do not adorn with auxiliary descriptors.*/
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        NULL /*Any `CreateFile` function call ignores templates when 
            opening existing files*/);
    if (hFile == INVALID_HANDLE_VALUE) {
        return ES_ERROR_FILE_NOT_FOUND;
        
    }
    
    /*XXX: Consider the case that a file is already open.*/
    // Add a deque.
    for (const sLineDeque *pDeque = pEditorState->dequeArr;
            pDeque != NULL; 
            ++pDeque, ++files);
    pEditorState->dequeArr = realloc(pEditorState->dequeArr, 
        (files+1 /*Consider null-terminator pointer.*/)*sizeof(sLineDeque));
    if (pEditorState->dequeArr == NULL) {
        return ES_ERROR_ALLOCATION_FAIL;
        
    }
    
    // Initialize the deque specific to the open file.
    pFileLineDeque = &(pEditorState->dequeArr[files-1]);
    pFileLineDeque->lines = 0;
    pFileLineDeque->hFile = hFile;
    pFileLineDeque->pTail = pFileLineDeque->pHead = NULL;
    
    // Prepare the buffer that stores file contents across iterations
    // searching for carriage return characters.
    dynamicBuffer.characters = 0;
    dynamicBuffer.pStart = malloc(sizeof(char));
    dynamicBuffer.pStart[0] = '\0';
    
    // Read lines from the open file.
    do {
        unsigned int characterIndex, beginningIndex, lineCharacters;
        
        if (!ReadFile(hFile, buffer, 
                ARRAY_SIZE(buffer)-1 /*Ignore null terminator*/, 
                &outputCharacters, 
                NULL)) {
            return ES_ERROR_PARSING_ERROR;
            
        }
        
        printf("Read %li bytes\n", outputCharacters);
        
        for (characterIndex = 0, beginningIndex = 0; 
                characterIndex < outputCharacters; 
                characterIndex += 2 /*Skip the CR+LF*/) {
            
            sLineNode *pNode;
            
            for (lineCharacters = 0; 
                    buffer[characterIndex] != '\r' 
                    && buffer[characterIndex] != '\0'
                    && characterIndex < outputCharacters;
                    ++characterIndex, ++lineCharacters);
            
            
            if (buffer[characterIndex] == '\0') {
                
                // The function reached the end of the buffer without
                // seeing any carriage return.
                dynamicBuffer.characters += lineCharacters;
                printf("Added %i characters.\n", lineCharacters);
                dynamicBuffer.pStart = realloc(dynamicBuffer.pStart, 
                    sizeof(char)*dynamicBuffer.characters);
                memcpy(dynamicBuffer.pStart, buffer, 
                    sizeof(char)*dynamicBuffer.characters);
                
                // Read another chunk of data from the input file.
                break;
                
            }
            
            
            // Otherwise, the parser found a carriage return character.
            // The dynamic buffer can remain at its last size.
            // However, the parsing process must decide from which 
            // buffer to read data from. A dynamic buffer size of zero
            // can hint to not read from it.
            if (dynamicBuffer.characters == 0) {
                pNode = constructLineNode(lineCharacters, 
                    buffer+beginningIndex /*Start at the substring*/);
                
            } else {
                pNode = constructLineNode(dynamicBuffer.characters, 
                    dynamicBuffer.pStart);
                
                // Hint for the next iteration to not read from the
                // dynamic buffer.
                dynamicBuffer.characters = 0;
                
            }
            
            // Append the node to the line deque.
            appendNodeToDeque(pNode, pFileLineDeque);
            
            // Prepare the index in the buffer delimiting the start of 
            // the next line. This index is only useful for reading 
            // next lines when the buffer has information. Two 
            // additional characters are necessary to consider the 
            // carriage return and line feed.
            beginningIndex += lineCharacters+2;
        }
        
        printf("%s", buffer);
        
    } while (outputCharacters 
            == ARRAY_SIZE(buffer)-1 /*Ignore null terminator*/);
    
    free(dynamicBuffer.pStart);
    
    printDeque(pFileLineDeque);
    
    return ES_ERROR_SUCCESS;
}

sLineNode *constructLineNode(const unsigned int characters, 
        const char *pBuffer) {
    
    // Allocate memory for node
    sLineNode *pNode = SALLOC(sLineNode);
    if (pNode == NULL) {
        return NULL;
        
    }
    
    // Allocate memory for null-terminating string.
    pNode->line.pStart = malloc(sizeof(char)*(characters+1/*Null sentinel*/));
    if (pNode->line.pStart == NULL) {
        return NULL;
        
    }
    pNode->line.characters = characters;
    
    // The memory copy may not read all characters in the buffer. This 
    // situation arises when the buffer holds multiple lines.
    memcpy(pNode->line.pStart, pBuffer, sizeof(char)*characters);
    
    // Add null-terminating character.
    pNode->line.pStart[characters] = '\0';
    
    printf("Added new node: %i %s\n", characters, pNode->line.pStart);
    
    return pNode;
}

void destructLineNode(sLineNode *pNode) {
    free(pNode->line.pStart);
    free(pNode);
    return;
}

void appendNodeToDeque(sLineNode *pAddition, sLineDeque *pDeque) {
    sLineNode *pPenultimate = pDeque->pTail;
    
    pAddition->pPrev = pPenultimate;
    pAddition->pNext = NULL;
    
    if (pDeque->lines < 1) {
        pDeque->pHead = pAddition;
        
    } else {        
        pPenultimate->pNext = pAddition;
        
    }
    
    pDeque->pTail = pAddition;
    
    ++(pDeque->lines);
    
    printf("Current appearance: ");
    printDeque(pDeque);
    
    return;
}

void printDeque(sLineDeque *pDeque) {
    sLineNode *pNode = pDeque->pHead;
    
    
    while (pNode != NULL) {
        printf("Node: \"%s\" -> ", pNode->line.pStart);
        
        pNode = pNode->pNext;
        
    }
    puts("X");
    
}