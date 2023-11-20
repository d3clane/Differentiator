#include <assert.h>

#include "StringFuncs.h"

const char* SkipSymbolsUntilStopChar(const char* string, const char stopChar)
{
    assert(string);
    
    const char* stringPtr = string;
    while (*stringPtr != stopChar && *stringPtr != '\0') 
        ++stringPtr;
    
    if (*stringPtr == '\0')
        return nullptr;

    return stringPtr;
}

const char* SkipSymbolsWhileStatement(const char* string, int (*statementFunc)(int))
{
    assert(string);
    assert(statementFunc);

    const char* stringPtr = string;

    while (statementFunc(*stringPtr) && *stringPtr != '\0')
        ++stringPtr;
    
    if (*stringPtr == '\0')
        return nullptr;

    return stringPtr;
}
