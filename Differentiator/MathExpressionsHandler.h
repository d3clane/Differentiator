#ifndef MATH_EXPRESSIONS_HADNLER_H
#define MATH_EXPRESSIONS_HADNLER_H

#include <stdio.h>

//TODO: ваще было бы логично поменять название на что-то типо equation, потому что это реально просто equation хранит
struct MathExpressionVariableType
{
    char* variableName;
    double      variableValue; 
};

struct MathExpressionVariablesArrayType
{
    MathExpressionVariableType* data;

    size_t capacity;
    size_t size;
};

enum class MathExpressionsOperations
{
    ADD,
    SUB,
    MUL,
    DIV,
};

union MathExpressionTokenValue
{
    double             value;
    int                varId;
    MathExpressionsOperations operation;
}; 

enum class MathExpressionTokenValueTypeof
{
    VALUE,
    VARIABLE,
    OPERATION, 
};

struct MathExpressionTokenType
{
    MathExpressionTokenValue        value;
    MathExpressionTokenValueTypeof valueType;
    
    MathExpressionTokenType*  left;
    MathExpressionTokenType* right;
};

struct MathExpressionType
{
    MathExpressionTokenType* root;

    MathExpressionVariablesArrayType variables;
};

enum class MathExpressionErrors
{
    NO_ERR,

    MEM_ERR,

    READING_ERR
};

MathExpressionErrors MathExpressionCtor(MathExpressionType* MathExpression, MathExpressionTokenType* root = nullptr);
MathExpressionErrors MathExpressionDtor(MathExpressionType* MathExpression);

MathExpressionErrors MathExpressionPrintPrefixFormat     (const MathExpressionType* MathExpression, FILE* outStream = stdout);
MathExpressionErrors MathExpressionPrintEquationFormat   (const MathExpressionType* MathExpression, FILE* outStream = stdout);
MathExpressionErrors MathExpressionPrintEquationFormatTex(const MathExpressionType* MathExpression, FILE* outStream = stdout, 
                                      const char* string = nullptr);

MathExpressionErrors MathExpressionReadPrefixFormat(MathExpressionType* MathExpression, FILE* inStream = stdin);
MathExpressionErrors MathExpressionReadInfixFormat (MathExpressionType* MathExpression, FILE* inStream = stdin);
MathExpressionErrors MathExpressionReadVariables(MathExpressionType* MathExpression);

#define MathExpression_TEXT_DUMP(tree) MathExpressionTextDump((tree), __FILE__, __func__, __LINE__)
void MathExpressionTextDump(const MathExpressionType* tree, const char* fileName, 
                                            const char* funcName,
                                            const int   line);

void MathExpressionGraphicDump(const MathExpressionType* tree, bool openImg = false);

#define MathExpression_DUMP(tree) MathExpressionDump((tree), __FILE__, __func__, __LINE__)
void MathExpressionDump(const MathExpressionType* tree, const char* fileName,
                                        const char* funcName,
                                        const int   line);

double MathExpressionCalculate(const MathExpressionType* tree);

MathExpressionErrors MathExpressionMathExpressionerentiate();

#endif // MATH_EXPRESSIONS_HADNLER_H