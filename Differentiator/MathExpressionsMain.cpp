#include <assert.h>
#include <ctype.h>
#include <string.h>
#include <math.h>

#include "MathExpressionsMain.h"
#include "Common/StringFuncs.h"
#include "Common/Log.h"
#include "FastInput/InputOutput.h"
#include "Common/DoubleFuncs.h"
#include "MathExpressionInOut.h"

//---------------------------------------------------------------------------------------

static void ExpressionDtor     (ExpressionTokenType* token);

static void ExpressionGraphicDump(const ExpressionTokenType* token, FILE* outDotFile);
static void DotFileCreateTokens(const ExpressionTokenType* token, 
                               const ExpressionVariablesArrayType* varsArr, FILE* outDotFile);

static inline void CreateImgInLogFile(const size_t imgIndex, bool openImg);
static inline void DotFileBegin(FILE* outDotFile);
static inline void DotFileEnd(FILE* outDotFile);

//---------------------------------------------------------------------------------------

ExpressionErrors ExpressionCtor(ExpressionType* expression)
{
    assert(expression);

    expression->root = nullptr;

    expression->variables.capacity = 100;
    expression->variables.size     =   0;
    expression->variables.data     = (ExpressionVariableType*)
                                        calloc(expression->variables.capacity, 
                                               sizeof(*(expression->variables.data)));
    
    return ExpressionErrors::NO_ERR;
}

//---------------------------------------------------------------------------------------

ExpressionErrors ExpressionDtor(ExpressionType* expression)
{
    assert(expression);

    ExpressionDtor(expression->root);
    expression->root = nullptr;

    for (size_t i = 0; i < expression->variables.capacity; ++i)
    {
        if      (expression->variables.data->variableName)
            free(expression->variables.data->variableName);

        expression->variables.data->variableName  = nullptr;
        expression->variables.data->variableValue = NAN;
    }

    free(expression->variables.data);
    expression->variables.data     = nullptr;
    expression->variables.size     = 0;
    expression->variables.capacity = 0;


    return ExpressionErrors::NO_ERR;
}

//---------------------------------------------------------------------------------------

static void ExpressionDtor(ExpressionTokenType* token)
{
    if (token == nullptr)
        return;
    
    assert(token);

    ExpressionDtor(token->left);
    ExpressionDtor(token->right);

    ExpressionTokenDtor(token);
}

//---------------------------------------------------------------------------------------

ExpressionTokenType* ExpressionTokenCtor(ExpressionTokenValue value, 
                                         ExpressionTokenValueTypeof valueType,
                                         ExpressionTokenType* left,
                                         ExpressionTokenType* right)
{   
    ExpressionTokenType* token = (ExpressionTokenType*)calloc(1, sizeof(*token));
    token->left      = left;
    token->right     = right;
    token->value     = value;
    token->valueType = valueType;
    
    return token;
}

//---------------------------------------------------------------------------------------

void ExpressionTokenDtor(ExpressionTokenType* token)
{
    token->left         = nullptr;
    token->right        = nullptr;
    token->value.varPtr = nullptr;

    free(token);
}

//---------------------------------------------------------------------------------------

static inline void CreateImgInLogFile(const size_t imgIndex, bool openImg)
{
    static const size_t maxImgNameLength  = 64;
    static char imgName[maxImgNameLength] = "";
    snprintf(imgName, maxImgNameLength, "../imgs/img_%zu_time_%s.png", imgIndex, __TIME__);

    static const size_t     maxCommandLength  = 128;
    static char commandName[maxCommandLength] =  "";
    snprintf(commandName, maxCommandLength, "dot ExpressionHandler.dot -T png -o %s", imgName);
    system(commandName);

    snprintf(commandName, maxCommandLength, "<img src = \"%s\">\n", imgName);    
    Log(commandName);

    if (openImg)
    {
        snprintf(commandName, maxCommandLength, "open %s", imgName);
        system(commandName);
    }
}

//---------------------------------------------------------------------------------------

static inline void DotFileBegin(FILE* outDotFile)
{
    fprintf(outDotFile, "digraph G{\nrankdir=TB;\ngraph [bgcolor=\"#31353b\"];\n"
                        "edge[color=\"#00D0D0\"];\n");
}

//---------------------------------------------------------------------------------------

static inline void DotFileEnd(FILE* outDotFile)
{
    fprintf(outDotFile, "\n}\n");
}

//---------------------------------------------------------------------------------------

void ExpressionGraphicDump(const ExpressionType* expression, bool openImg)
{
    assert(expression);

    static const char* dotFileName = "ExpressionHandler.dot";
    FILE* outDotFile = fopen(dotFileName, "w");

    if (outDotFile == nullptr)
        return;

    DotFileBegin(outDotFile);

    DotFileCreateTokens(expression->root, &expression->variables, outDotFile);

    ExpressionGraphicDump(expression->root, outDotFile);

    DotFileEnd(outDotFile);

    fclose(outDotFile);

    static size_t imgIndex = 0;
    CreateImgInLogFile(imgIndex, openImg);
    imgIndex++;
}

//---------------------------------------------------------------------------------------

static void DotFileCreateTokens(const ExpressionTokenType* token,    
                               const ExpressionVariablesArrayType* varsArr, FILE* outDotFile)
{
    assert(varsArr);
    assert(outDotFile);

    if (token == nullptr)
        return;
    
    fprintf(outDotFile, "token%p"
                        "[shape=Mrecord, style=filled, ", token);

    if (token->valueType == ExpressionTokenValueTypeof::OPERATION)
        fprintf(outDotFile, "fillcolor=\"#89AC76\", label = \"%s\", ", 
                            ExpressionOperationGetLongName(token->value.operation));
    else if (token->valueType == ExpressionTokenValueTypeof::VALUE)
        fprintf(outDotFile, "fillcolor=\"#7293ba\", label = \"%.2lf\", ", token->value.value);
    else if (token->valueType == ExpressionTokenValueTypeof::VARIABLE)
        fprintf(outDotFile, "fillcolor=\"#78DBE2\", label = \"%s\", ",
                            token->value.varPtr->variableName);
    else 
        fprintf(outDotFile, "fillcolor=\"#FF0000\", label = \"ERROR\", ");

    fprintf(outDotFile, "color = \"#D0D000\"];\n");

    DotFileCreateTokens(token->left,  varsArr, outDotFile);
    DotFileCreateTokens(token->right, varsArr, outDotFile);
}

//---------------------------------------------------------------------------------------

static void ExpressionGraphicDump(const ExpressionTokenType* token, FILE* outDotFile)
{
    if (token == nullptr)
    {
        fprintf(outDotFile, "\n");
        return;
    }
    
    fprintf(outDotFile, "token%p;\n", token);

    if (token->left != nullptr) fprintf(outDotFile, "token%p->", token);
    ExpressionGraphicDump(token->left, outDotFile);

    if (token->right != nullptr) fprintf(outDotFile, "token%p->", token);
    ExpressionGraphicDump(token->right, outDotFile);
}

//---------------------------------------------------------------------------------------

void ExpressionTextDump(const ExpressionType* expression, const char* fileName, 
                                                          const char* funcName,
                                                          const int   line)
{
    assert(expression);
    assert(fileName);
    assert(funcName);

    LogBegin(fileName, funcName, line);

    Log("Tree root: %p, value: %s\n", expression->root, expression->root->value);
    Log("Tree: ");
    ExpressionPrintPrefixFormat(expression, nullptr);

    LOG_END();
}

//---------------------------------------------------------------------------------------

void ExpressionDump(const ExpressionType* expression, const char* fileName,
                                                              const char* funcName,
                                                              const int   line)
{
    assert(expression);
    assert(fileName);
    assert(funcName);

    ExpressionTextDump(expression, fileName, funcName, line);

    ExpressionGraphicDump(expression);
}

//---------------------------------------------------------------------------------------

void ExpressionsCopyVariables(      ExpressionType* target, 
                              const ExpressionType* source)
{
    assert(target);
    assert(source);

    assert(target->variables.capacity == source->variables.capacity);
    assert(target->variables.size == 0);

    //TODO: подумать над созданием функции копирования одной переменной, как будто не нужна
    for (size_t i = 0; i < source->variables.size; ++i)
    {
        target->variables.data[i].variableName  = strdup(source->variables.data->variableName);
        target->variables.data[i].variableValue = source->variables.data->variableValue;
    }
}

//---------------------------------------------------------------------------------------

ExpressionErrors ExpressionReadVariables(ExpressionType* expression)
{
    assert(expression);

    printf("Enter variables values: \n");
    for (size_t i = 0; i < expression->variables.size; ++i)
    {
        printf("%s: ", expression->variables.data[i].variableName);
        int scanfResult = scanf("%lf",  &expression->variables.data[i].variableValue);

        if (scanfResult == 0)
            return ExpressionErrors::READING_ERR;
    }

    return ExpressionErrors::NO_ERR;
}

//---------------------------------------------------------------------------------------

ExpressionType ExpressionCopy(const ExpressionType* expression)
{
    ExpressionTokenType* copyExprRoot = ExpressionTokenCopy(expression->root);
    ExpressionType copyExpr = {};
    ExpressionCtor(&copyExpr);

    copyExpr.root = copyExprRoot;

    ExpressionsCopyVariables(&copyExpr, expression);

    return copyExpr;
}

ExpressionTokenType* ExpressionTokenCopy(const ExpressionTokenType* token)
{
    if (token == nullptr)
        return nullptr;

    ExpressionTokenType* left  = ExpressionTokenCopy(token->left);
    ExpressionTokenType* right = ExpressionTokenCopy(token->right);

    return ExpressionTokenCtor(token->value, token->valueType, left, right);
}

//---------------------------------------------------------------------------------------

ExpressionTokenValue ExpressionTokenValueСreate(double value)
{
    ExpressionTokenValue tokenValue =
    {
        .value = value
    };

    return tokenValue;
}

//---------------------------------------------------------------------------------------

ExpressionTokenValue ExpressionTokenValueСreate(ExpressionOperationId operation)
{
    ExpressionTokenValue value =
    {
        .operation = operation,
    };

    return value;
}

//---------------------------------------------------------------------------------------

ExpressionTokenType* ExpressionNumericTokenCreate(double value)
{
    ExpressionTokenValue tokenVal = ExpressionTokenValueСreate(value);

    return ExpressionTokenCtor(tokenVal, ExpressionTokenValueTypeof::VALUE);
}

//---------------------------------------------------------------------------------------

void ExpressionTokenSetEdges(ExpressionTokenType* token, ExpressionTokenType* left, 
                                                     ExpressionTokenType* right)
{
    assert(token);

    token->left  = left;
    token->right = right;
}

int ExpressionOperationGetId(const char* string)
{
    assert(string);

    #define GENERATE_OPERATION_CMD(NAME, v1, v2, v3, SHORT_NAME, ...) SHORT_NAME,

    static const char* shortNamesArr[] = 
    {
        #include "Operations.h"
    };

    #undef  GENERATE_OPERATION_CMD
    #define GENERATE_OPERATION_CMD(NAME, ...) #NAME, 

    static const char* longNamesArr[] = 
    {
        #include "Operations.h"
    };

    #undef GENERATE_OPERATION_CMD

    static const size_t NumberOfOperations = sizeof(shortNamesArr) / sizeof(*shortNamesArr);

    for (size_t i = 0; i < NumberOfOperations; ++i)
    {
        if (strcasecmp(string, shortNamesArr[i]) == 0 || 
            strcasecmp(string, longNamesArr[i])  == 0)
            return (int)i;
    }

    return -1;
}

bool ExpressionOperationIsPrefix(const ExpressionOperationId operation, bool inTex)
{

    #define GENERATE_OPERATION_CMD(NAME, FORMAT, TEX_FORMAT, ...)                               \
        case ExpressionOperationId::NAME:                                                       \
            return ExpressionOperationFormat::TEX_FORMAT == ExpressionOperationFormat::PREFIX;

    if (inTex)
    {
        switch (operation)
        {
            #include "Operations.h"

            default:
                break;
        }

        return false;
    }

    #undef  GENERATE_OPERATION_CMD
    #define GENERATE_OPERATION_CMD(NAME, FORMAT, ...)                                           \
        case ExpressionOperationId::NAME:                                                       \
            return ExpressionOperationFormat::FORMAT == ExpressionOperationFormat::PREFIX;

    switch(operation)
    {
        #include "Operations.h"

        default:
            break;
    }

    #undef  GENERATE_OPERATION_CMD

    return false;
}

bool ExpressionOperationIsUnary(const ExpressionOperationId operation)
{

    #define GENERATE_OPERATION_CMD(NAME, v1, v2, IS_UNARY, ...)                         \
        case ExpressionOperationId::NAME:                                               \
            return IS_UNARY;
        
    switch (operation)
    {
        #include "Operations.h"

        default:
            break;
    }

    #undef GENERATE_OPERATION_CMD

    return false;
}

const char* ExpressionOperationGetLongName(const  ExpressionOperationId operation)
{
    #define GENERATE_OPERATION_CMD(NAME, ...)           \
        case ExpressionOperationId::NAME:               \
            return #NAME;

    switch (operation)
    {
        #include "Operations.h"

        default:
            break;
    }

    #undef GENERATE_OPERATION_CMD

    return nullptr;
}

const char* ExpressionOperationGetShortName(const ExpressionOperationId operation)
{
    #define GENERATE_OPERATION_CMD(NAME, v1, v2, v3, SHORT_NAME, ...)   \
        case ExpressionOperationId::NAME:                               \
            return SHORT_NAME;
        
    switch(operation)
    {
        #include "Operations.h"

        default:
            break;
    }

    #undef GENERATE_OPERATION_CMD

    return nullptr;
}
