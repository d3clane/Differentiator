#ifndef _EXPRESSIONS_HADNLER_H
#define _EXPRESSIONS_HADNLER_H

#include <stdio.h>
#include <math.h>

struct ExpressionVariableType
{
    char*  variableName;
    double variableValue; 
};

struct ExpressionVariablesArrayType
{
    ExpressionVariableType* data;

    size_t capacity;
    size_t size;
};

#define GENERATE_OPERATION_CMD(NAME, ...) NAME, 

enum class ExpressionOperationsIds
{
    #include "Operations.h"
};

#undef GENERATE_OPERATION_CMD

enum class ExpressionOperationFormat
{
    PREFIX,
    INFIX,
};

typedef double (CalculationFuncType)(double firstVal, double secondVal);

struct ExpressionOperationType
{

    ExpressionOperationsIds   operationId;
    ExpressionOperationFormat operationFormat;
    ExpressionOperationFormat operationTexFormat;
    
    bool isUnaryOperation;

    const char* longName;
    const char* shortName;

    const char* texName;
    bool needTexLeftBraces;
    bool needTexRightBraces;

    CalculationFuncType* CalculationFunc;
};

union ExpressionTokenValue
{
    double                  value;
    ExpressionVariableType* varPtr;
    ExpressionOperationType operation;
}; 

enum class ExpressionTokenValueTypeof
{
    VALUE,
    VARIABLE,
    OPERATION, 
};

struct ExpressionTokenType
{
    ExpressionTokenValue        value;
    ExpressionTokenValueTypeof  valueType;
    
    ExpressionTokenType*  left;
    ExpressionTokenType* right;
};

struct ExpressionType
{
    ExpressionTokenType* root;

    ExpressionVariablesArrayType variables;
};

enum class ExpressionErrors
{
    NO_ERR,

    MEM_ERR,

    READING_ERR,
};

ExpressionErrors ExpressionCtor(ExpressionType* expression);
ExpressionErrors ExpressionDtor(ExpressionType* expression);

ExpressionTokenType* ExpressionTokenCtor(ExpressionTokenValue value, 
                                                        ExpressionTokenValueTypeof valueType,
                                                        ExpressionTokenType* left  = nullptr,
                                                        ExpressionTokenType* right = nullptr);
void ExpressionTokenDtor(ExpressionTokenType* token);

ExpressionTokenValue ExpressionTokenValueСreate(double value);
ExpressionTokenValue ExpressionTokenValueСreate(ExpressionOperationsIds operationId);
ExpressionTokenType* ExpressionNumericTokenCreate(double value);

#define _EXPRESSION_TEXT_DUMP(expression) ExpressionTextDump((expression), __FILE__, \
                                                                                   __func__, \
                                                                                   __LINE__)

void ExpressionTextDump(const ExpressionType* expression, const char* fileName, 
                                                                  const char* funcName,
                                                                  const int   line);

void ExpressionGraphicDump(const ExpressionType* expression, bool openImg = false);

#define _EXPRESSION_DUMP(expression) ExpressionDump((expression), __FILE__,  \
                                                                          __func__,  \
                                                                          __LINE__)

void ExpressionDump(const ExpressionType* expression, const char* fileName,
                                                              const char* funcName,
                                                              const int   line);

void ExpressionTokenSetEdges(ExpressionTokenType* token, ExpressionTokenType* left, 
                                                         ExpressionTokenType* right);

ExpressionType       ExpressionCopy(const ExpressionType* expression);
ExpressionTokenType* ExpressionTokenCopy(const ExpressionTokenType* token);
void ExpressionsCopyVariables(ExpressionType* target, const ExpressionType* source);

double ExpressionCalculate(const ExpressionType* expression);

int  ExpressionOperationGetId(const char* string);
bool ExpressionOperationIsPrefix(const ExpressionOperationType* operation, bool inTex = false);
bool ExpressionOperationIsUnary(const ExpressionOperationType* operation);

#endif // _EXPRESSIONS_HADNLER_H