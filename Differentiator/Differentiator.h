#ifndef DIFFERENTIATOR_H
#define DIFFERENTIATOR_H

#include <stdio.h>


struct DiffVariableType
{
    char* variableName;
    double      variableValue; 
};

struct DiffVariablesArrayType
{
    DiffVariableType* data;

    size_t capacity;
    size_t size;
};

union DiffValue
{
    double   value;
    int      varId;
    char operation;
}; 

enum class DiffValueType
{
    VALUE,
    VARIABLE,
    OPERATION, 
};

struct DiffTreeNodeType
{
    DiffValue     value;
    DiffValueType valueType;
    
    DiffTreeNodeType*  left;
    DiffTreeNodeType* right;
};

struct DiffTreeType
{
    DiffTreeNodeType* root;

    DiffVariablesArrayType variables;
};

enum class DiffErrors
{
    NO_ERR,

    MEM_ERR,

    READING_ERR
};

DiffErrors DiffCtor(DiffTreeType* diff, DiffTreeNodeType* root = nullptr);
DiffErrors DiffDtor(DiffTreeType* diff);

DiffErrors DiffPrintPrefixFormat     (const DiffTreeType* diff, FILE* outStream = stdout);
DiffErrors DiffPrintEquationFormat   (const DiffTreeType* diff, FILE* outStream = stdout);
DiffErrors DiffPrintEquationFormatTex(const DiffTreeType* diff, FILE* outStream = stdout, 
                                                                const char* string = nullptr);
DiffErrors DiffReadPrefixFormat(DiffTreeType* diff, FILE* inStream = stdin);

DiffErrors DiffReadVariables(DiffTreeType* diff);

#define DIFF_TEXT_DUMP(tree) DiffTextDump((tree), __FILE__, __func__, __LINE__)
void DiffTextDump(const DiffTreeType* tree, const char* fileName, 
                                            const char* funcName,
                                            const int   line);

void DiffGraphicDump(const DiffTreeType* tree, bool openImg = false);

#define DIFF_DUMP(tree) DiffDump((tree), __FILE__, __func__, __LINE__)
void DiffDump(const DiffTreeType* tree, const char* fileName,
                                        const char* funcName,
                                        const int   line);

double DiffCalculate(const DiffTreeType* tree);

#endif // DIFFERENTIATOR_H