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
#define TRUE 1
#define FALSE 0

#define ARRAY_CHARS(arr) (sizeof(arr)/sizeof(char))
#define SALLOC(s) (malloc(sizeof(s)))

void appendNodeToDeque(sLineNode *pNode, sLineDeque *pDeque);

// Debug functions
void printDeque(sLineDeque *pDeque);

enum EsError loadFileIntoEditorState(const char *pFilepath, 
        sEditorState *pEditorState) {
    
    char buffer[BUFFER_CHARACTERS] = { 0 };     // Buffer for file.
    sLine dynamicBuffer;                        // Persistent buffer.
    sLineDeque *pDeque;                         // Line deque of file.
    HANDLE hFile;                               // Handle to file.
    unsigned long int outputCharacters;         // Characters to read.
    unsigned long int files = 0;                // Open file amount.
    
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
    pEditorState->dequeArr = SALLOC(sLineDeque);
    
    // Initialize the deque specific to the open file.
    pDeque = pEditorState->dequeArr;
    pDeque->lines = 0;
    pDeque->hFile = hFile;
    pDeque->pTail = pDeque->pHead = NULL;
    
    // Prepare the buffer that stores file contents across iterations
    // searching for carriage return characters.
    dynamicBuffer.characters = 0;
    dynamicBuffer.pStart = malloc(1*sizeof(char));
    dynamicBuffer.pStart[0] = '\0';
    
    // Read lines from the open file.
    do {
        unsigned int characterIndex, beginningIndex, lineCharacters;
        
        if (!ReadFile(hFile, buffer, 
                ARRAY_CHARS(buffer)-1 /*Ignore null terminator*/, 
                &outputCharacters, 
                NULL)) {
            return ES_ERROR_PARSING_ERROR;
            
        }
        
        // The CR+LF may be across two subsequent iterations. 
        // It is important to skip the line feed character in this 
        // case.
        characterIndex = beginningIndex = buffer[0] == '\n';
        do {
            sLineNode *pNode;
            
            for (lineCharacters = 0; 
                    buffer[characterIndex] != '\r'
                    && characterIndex < outputCharacters;
                    ++characterIndex, ++lineCharacters);
            
            if (characterIndex >= outputCharacters) {
                unsigned int biggerStrCharacters = dynamicBuffer.characters 
                    + lineCharacters;
                unsigned int remainerSegmentIndex = ARRAY_CHARS(buffer) 
                    - lineCharacters - 1;
                
                // The function reached the end of the buffer without
                // seeing any carriage return.
                
                dynamicBuffer.pStart = realloc(dynamicBuffer.pStart, 
                    sizeof(char)*(biggerStrCharacters + 1 /*Null*/));
                if (dynamicBuffer.pStart == NULL) {
                    return ES_ERROR_ALLOCATION_FAIL;
                    
                }
                
                memcpy(dynamicBuffer.pStart+dynamicBuffer.characters, 
                    buffer+remainerSegmentIndex, lineCharacters);
                
                // Read another chunk of data from the input file if 
                // not the last line.
                if (outputCharacters == ARRAY_CHARS(buffer)
                        - 1 /*Ignore null terminator.*/) {
                    
                    // Only update the number of characters when not 
                    // at the last line. There are no more characters 
                    // to expect after the last line.
                    dynamicBuffer.characters = biggerStrCharacters;
                    break;
                    
                }
                
                // Otherwise, no more characters are present.
                
            }
            
            // Otherwise, the parser found a carriage return character.
            // The dynamic buffer can remain at its last size.
            // However, the parsing process must decide from which 
            // buffer to read data from. A dynamic buffer size of zero
            // can hint to not read from it.
            if (dynamicBuffer.characters == 0) {
                
                pNode = constructLineNode(lineCharacters);
                
                // Initialize the line that the node points to.
                memcpy(pNode->line.pStart, 
                    buffer+beginningIndex /*Start at the substring*/, 
                    sizeof(char)*lineCharacters);
                
            } else {
                // A line exceeding the capacity of the buffer 
                // requires a special initialization procedure.
                
                pNode = constructLineNode(dynamicBuffer.characters
                    + lineCharacters /*Plus the buffer's line.*/);
                
                memcpy(pNode->line.pStart,
                    dynamicBuffer.pStart,
                    sizeof(char)*dynamicBuffer.characters);                
                memcpy(pNode->line.pStart+dynamicBuffer.characters,
                    buffer,
                    sizeof(char)*lineCharacters);
                
                // Hint for the next iteration to not read from the
                // dynamic buffer.
                dynamicBuffer.characters = 0;
                
            }
            
            // Append the node to the line deque.
            appendNodeToDeque(pNode, pDeque);
            
            // Prepare the index in the buffer delimiting the start of 
            // the next line. This index is only useful for reading 
            // next lines when the buffer has information. Two 
            // additional characters are necessary to consider the 
            // carriage return and line feed.
            beginningIndex += lineCharacters+2;
            characterIndex += 2; /*Skip the CR+LF*/
        } while (characterIndex < outputCharacters);
    } while (outputCharacters != 0);
    
    free(dynamicBuffer.pStart);
    
    // Set the head node of the deque as the initial line subject to
    // edits.
    pDeque->writeHead.pNode = pDeque->pHead;
    pDeque->writeHead.index = 0;
    
    return ES_ERROR_SUCCESS;
}

sLineNode *constructLineNode(unsigned int characters) {
    
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
    
    // Add null-terminating character.
    pNode->line.pStart[characters] = '\0';
    
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
    
    return;
}


void printDeque(sLineDeque *pDeque) {
    sLineNode *pNode = pDeque->pHead;
    while (pNode != NULL) {
        printf("%s\n", pNode->line.pStart);
        pNode = pNode->pNext;
    }
    puts("X");
    return;
}