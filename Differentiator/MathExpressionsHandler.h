#ifndef MATH_EXPRESSIONS_HADNLER_H
#define MATH_EXPRESSIONS_HADNLER_H

#include <stdio.h>
#include <math.h>

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

//TODO: rename Enum -> id, Expressions->expression
enum class MathExpressionsOperationsEnum
{
    ADD,
    SUB,
    MUL,
    DIV,

    POW,
    LOG,

    SIN,
    COS,
    TAN,
    COT,
    ARCSIN,
    ARCCOS,
    ARCTAN,
    ARCCOT,
};

enum class MathExpressionOperationFormat
{
    PREFIX,
    INFIX,
    POSTFIX,
};

typedef double (CalculationFuncType)(double firstVal, double secondVal);

struct MathExpressionOperationType
{

    MathExpressionsOperationsEnum operationId;
    MathExpressionOperationFormat operationFormat;

    const char* longName;
    const char* shortName;

    CalculationFuncType* CalculationFunc;
};

union MathExpressionTokenValue
{
    double             value;
    int                varId;
    MathExpressionOperationType operation;
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

MathExpressionErrors MathExpressionCtor(MathExpressionType* expression);
MathExpressionErrors MathExpressionDtor(MathExpressionType* expression);

MathExpressionErrors MathExpressionPrintPrefixFormat     (const MathExpressionType* expression, 
                                                          FILE* outStream = stdout);
MathExpressionErrors MathExpressionPrintEquationFormat   (const MathExpressionType* expression, 
                                                          FILE* outStream = stdout);
MathExpressionErrors MathExpressionPrintEquationFormatTex(const MathExpressionType* expression,
                                                          FILE* outStream = stdout, 
                                                          const char* funnyString = nullptr);

MathExpressionErrors MathExpressionReadPrefixFormat(MathExpressionType* expression, 
                                                    FILE* inStream = stdin);
MathExpressionErrors MathExpressionReadInfixFormat (MathExpressionType* expression, 
                                                    FILE* inStream = stdin);
MathExpressionErrors MathExpressionReadVariables(MathExpressionType* expression);

#define MATH_EXPRESSION_TEXT_DUMP(expression) MathExpressionTextDump((expression), __FILE__, \
                                                                                   __func__, \
                                                                                   __LINE__)

void MathExpressionTextDump(const MathExpressionType* expression, const char* fileName, 
                                                                  const char* funcName,
                                                                  const int   line);

void MathExpressionGraphicDump(const MathExpressionType* expression, bool openImg = false);

#define MATH_EXPRESSION_DUMP(expression) MathExpressionDump((expression), __FILE__,  \
                                                                          __func__,  \
                                                                          __LINE__)

void MathExpressionDump(const MathExpressionType* expression, const char* fileName,
                                                              const char* funcName,
                                                              const int   line);

double MathExpressionCalculate(const MathExpressionType* expression);

MathExpressionType MathExpressionDifferentiate(const MathExpressionType* expression);

#endif // MATH_EXPRESSIONS_HADNLER_H