#include <assert.h>
#include <ctype.h>
#include <string.h>
#include <math.h>

#include "MathExpressionsHandler.h"
#include "Common/StringFuncs.h"
#include "Common/Log.h"
#include "FastInput/InputOutput.h"
#include "Common/DoubleFuncs.h"
#include "MathExpressionInOut.h"

//TODO: подумать над всеми названиями, что не начинаются с Expression
//TODO: накидать inline когда будет не лень, я их тут проебал

const double POISON = NAN;

//---------------------------------------------------------------------------------------

//Warning - don't use for nothing else than construction of standard operations.
static ExpressionOperationType ExpressionOperationTypeCtor(
                                                    ExpressionOperationsIds operationId,
                                                    ExpressionOperationFormat operationFormat,
                                                    ExpressionOperationFormat operationTexFormat, 
                                                    bool isUnaryOperation,
                                                    const char* longName,
                                                    const char* shortName,
                                                    const char* texName,
                                                    bool needTexLeftBraces,
                                                    bool needTexRightBraces,
                                                    CalculationFuncType* CalculationFunc);

static inline double CalculateADD(const double val1, const double val2);
static inline double CalculateSUB(const double val1, const double val2);
static inline double CalculateMUL(const double val1, const double val2);
static inline double CalculateDIV(const double val1, const double val2);

static inline double CalculatePOW(const double base, const double power);
static inline double CalculateLOG(const double base, const double val);
static inline double CalculateLN(const double val,   const double val2);

static inline double CalculateSIN(const double val, const double val2 = NAN);
static inline double CalculateCOS(const double val, const double val2 = NAN);
static inline double CalculateTAN(const double val, const double val2 = NAN);
static inline double CalculateCOT(const double val, const double val2 = NAN);

static inline double CalculateARCSIN(const double val, const double val2 = NAN);
static inline double CalculateARCCOS(const double val, const double val2 = NAN);
static inline double CalculateARCTAN(const double val, const double val2 = NAN);
static inline double CalculateARCCOT(const double val, const double val2 = NAN);

//---------------------------------------------------------------------------------------

#define GENERATE_OPERATION_CMD(NAME, FORMAT, TEX_FORMAT, IS_UNARY, SHORT_CUT_STRING, TEX_NAME,    \
                        NEED_LEFT_TEX_BRACES, NEED_RIGHT_TEX_BRACES, ...)                         \
                                                    ExpressionOperationTypeCtor(              \
                                                        ExpressionOperationsIds::NAME,      \
                                                        ExpressionOperationFormat::FORMAT,    \
                                                        ExpressionOperationFormat::TEX_FORMAT,\
                                                        IS_UNARY,                                 \
                                                        #NAME, SHORT_CUT_STRING,                  \
                                                        TEX_NAME,                                 \
                                                        NEED_LEFT_TEX_BRACES,                     \
                                                        NEED_RIGHT_TEX_BRACES,                    \
                                                        Calculate##NAME),

static const ExpressionOperationType Operations[] = 
{
    #include "Operations.h"
};

static const size_t NumberOfOperations = sizeof(Operations) / sizeof(*Operations);

#undef GENERATE_OPERATION_CMD

//---------------------------------------------------------------------------------------

static inline ExpressionOperationType GetOperationStruct(
                                                ExpressionOperationsIds operationId);

static int GetVariableIdByName(const ExpressionVariablesArrayType* varsArr, 
                               const char* variableName);

//---------------------------------------------------------------------------------------

static void ExpressionDtor     (ExpressionTokenType* token);

//---------------------------------------------------------------------------------------

static void ExpressionGraphicDump(const ExpressionTokenType* token, FILE* outDotFile);
static void DotFileCreateTokens(const ExpressionTokenType* token, 
                               const ExpressionVariablesArrayType* varsArr, FILE* outDotFile);

static inline void CreateImgInLogFile(const size_t imgIndex, bool openImg);
static inline void DotFileBegin(FILE* outDotFile);
static inline void DotFileEnd(FILE* outDotFile);

void ExpressionsCopyVariables(      ExpressionType* target,  const ExpressionType* source);
static bool ExpressionTokenContainVariable(const ExpressionTokenType* token);

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

        expression->variables.data->variableName = nullptr;
        expression->variables.data->variableValue = POISON;
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

static inline ExpressionOperationType GetOperationStruct(
                                            ExpressionOperationsIds operationId)
{
    assert((int)operationId >= 0);
    assert((size_t)operationId < NumberOfOperations);

    return Operations[(size_t)operationId];
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
    token->left        = nullptr;
    token->right       = nullptr;
    token->value.varId =  POISON;

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
                            token->value.operation.longName);
    else if (token->valueType == ExpressionTokenValueTypeof::VALUE)
        fprintf(outDotFile, "fillcolor=\"#7293ba\", label = \"%.2lf\", ", token->value.value);
    else if (token->valueType == ExpressionTokenValueTypeof::VARIABLE)
        fprintf(outDotFile, "fillcolor=\"#78DBE2\", label = \"%s\", ", 
                            varsArr->data[token->value.varId].variableName);
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

static double CalculateADD(const double val1, const double val2)
{
    assert(isfinite(val1));
    assert(isfinite(val2));

    return val1 + val2;
}

static double CalculateSUB(const double val1, const double val2)
{
    assert(isfinite(val1));
    assert(isfinite(val2));

    return val1 - val2;
}

static double CalculateMUL(const double val1, const double val2)
{
    assert(isfinite(val1));
    assert(isfinite(val2));

    return val1 * val2;
}

static double CalculateDIV(const double val1, const double val2)
{
    assert(isfinite(val1));
    assert(isfinite(val2));
    assert(!DoubleEqual(val2, 0));

    return val1 / val2;
}

static double CalculatePOW(const double base, const double power)
{
    assert(isfinite(base));
    assert(isfinite(power));

    return pow(base, power);
}

static double CalculateLOG(const double base, const double val)
{
    assert(isfinite(base));
    assert(isfinite(val));

    double log_base = log(base);

    assert(!DoubleEqual(log_base, 0));

    return log(val) / log_base;
}

static inline double CalculateLN(const double val, const double val2)
{
    assert(isfinite(val));

    return log(val);
}

static double CalculateSIN(const double val1, const double val2)
{
    assert(isfinite(val1));

    return sin(val1);
}

static double CalculateCOS(const double val1, const double val2)
{
    assert(isfinite(val1));

    return cos(val1);
}

static double CalculateTAN(const double val1, const double val2)
{
    assert(isfinite(val1));

    return tan(val1);
}

static double CalculateCOT(const double val1, const double val2)
{
    assert(isfinite(val1));

    double tan_val1 = tan(val1);

    assert(!DoubleEqual(tan_val1, 0));
    assert(isfinite(tan_val1));

    return 1 / tan_val1;
}

static double CalculateARCSIN(const double val1, const double val2)
{
    assert(isfinite(val1));

    return asin(val1);
}

static double CalculateARCCOS(const double val1, const double val2)
{
    assert(isfinite(val1));

    return acos(val1);
}

static double CalculateARCTAN(const double val1, const double val2)
{
    assert(isfinite(val1));

    return atan(val1);
}

static double CalculateARCCOT(const double val1, const double val2)
{
    assert(isfinite(val1));

    return PI / 2 - atan(val1);
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

ExpressionTokenValue ExpressionCreateTokenValue(double value)
{
    ExpressionTokenValue tokenValue =
    {
        .value = value
    };

    return tokenValue;
}

//---------------------------------------------------------------------------------------

ExpressionTokenValue ExpressionCreateTokenValue(ExpressionOperationsIds operationId)
{
    ExpressionTokenValue value =
    {
        .operation = GetOperationStruct(operationId),
    };

    return value;
}

//---------------------------------------------------------------------------------------

ExpressionTokenType* ExpressionCreateNumericToken(double value)
{
    ExpressionTokenValue tokenVal = ExpressionCreateTokenValue(value);

    return ExpressionTokenCtor(tokenVal, ExpressionTokenValueTypeof::VALUE);
}

//---------------------------------------------------------------------------------------

static ExpressionOperationType ExpressionOperationTypeCtor(
                                                    ExpressionOperationsIds operationId,
                                                    ExpressionOperationFormat operationFormat,
                                                    ExpressionOperationFormat operationTexFormat,
                                                    bool isUnaryOperation,
                                                    const char* longName,
                                                    const char* shortName,
                                                    const char* texName,
                                                    bool needTexLeftBraces,
                                                    bool needTexRightBraces,
                                                    CalculationFuncType* CalculationFunc)
{
    assert(longName);
    assert(shortName);
    assert(texName);

    ExpressionOperationType operation = 
    {
        .operationId            = operationId,
        .operationFormat        = operationFormat,
        .operationTexFormat     = operationTexFormat,

        .isUnaryOperation       = isUnaryOperation,

        .longName               = longName, 
        .shortName              = shortName,

        .texName                = texName,
        .needTexLeftBraces    = needTexLeftBraces,
        .needTexRightBraces   = needTexRightBraces, 

        .CalculationFunc        = CalculationFunc,
    };

    return operation;
}

//---------------------------------------------------------------------------------------

void ExpressionTokenSetEdges(ExpressionTokenType* token, ExpressionTokenType* left, 
                                                     ExpressionTokenType* right)
{
    assert(token);

    token->left  = left;
    token->right = right;
}


//TODO: подумать над созданием файла, где будет все, связанное с операциями
int GetOperationId(const char* string)
{
    assert(string);

    for (size_t i = 0; i < NumberOfOperations; ++i)
    {
        if (strcasecmp(string, Operations[i].shortName) == 0 || 
            strcasecmp(string, Operations[i].longName)  == 0)
            return (int)i;
    }
    return -1;
}
