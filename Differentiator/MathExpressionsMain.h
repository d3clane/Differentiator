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

enum class ExpressionOperationId
{
    #include "Operations.h"
};

#undef GENERATE_OPERATION_CMD

enum class ExpressionOperationFormat
{
    PREFIX,
    INFIX,
    POSTFIX
};

typedef double (CalculationFuncType)(double firstVal, double secondVal);

union ExpressionTokenValue
{
    double                  value;
    ExpressionVariableType* varPtr;
    ExpressionOperationId operation;
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

//-------------Expression main funcs----------

ExpressionErrors ExpressionCtor(ExpressionType* expression);
ExpressionErrors ExpressionDtor(ExpressionType* expression);

ExpressionTokenType* ExpressionTokenCtor(ExpressionTokenValue value, 
                                                        ExpressionTokenValueTypeof valueType,
                                                        ExpressionTokenType* left  = nullptr,
                                                        ExpressionTokenType* right = nullptr);
void ExpressionTokenDtor(ExpressionTokenType* token);

ExpressionTokenValue ExpressionTokenValueСreate(double value);
ExpressionTokenValue ExpressionTokenValueСreate(ExpressionOperationId operationId);
ExpressionTokenValue ExpressionTokenValueСreate(ExpressionVariableType* varPtr);

ExpressionTokenType* ExpressionNumericTokenCreate(double value);
ExpressionTokenType* ExpressionVariableTokenCreate(ExpressionVariablesArrayType* varsArr,
                                                   const char* varName);

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

ExpressionVariableType* ExpressionVariableSet(ExpressionType* expression, 
                                              const char*  variableName, 
                                              const double variableValue = 0);

ExpressionVariableType* ExpressionVariableSet(ExpressionVariablesArrayType* varsArr, 
                                              const char*  variableName, 
                                              const double variableValue = 0);

ExpressionVariableType* ExpressionVariableChangeName(ExpressionType* expression,
                                                     const char* prevName,
                                                     const char* newName);
ExpressionVariableType* ExpressionVariableChangeName(ExpressionVariablesArrayType* varsArr,
                                                     const char* prevName,
                                                     const char* newName);

void ExpressionsCopyVariables(ExpressionType* target, const ExpressionType* source);

//-------------Operations funcs-----------

int  ExpressionOperationGetId(const char* string);
const char* ExpressionOperationGetLongName (const  ExpressionOperationId operation);
const char* ExpressionOperationGetShortName(const ExpressionOperationId operation);
bool ExpressionOperationIsUnary(const ExpressionOperationId operation);

#endif // _EXPRESSIONS_HADNLER_H