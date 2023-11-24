#include <assert.h>
#include <ctype.h>
#include <string.h>
#include <math.h>

#include "MathExpressionsHandler.h"
#include "../Common/StringFuncs.h"
#include "../Common/Log.h"
#include "../FastInput/InputOutput.h"
#include "../Common/DoubleFuncs.h"

//TODO: подумать над всеми названиями, что не начинаются с MathExpression
//TODO: накидать inline когда будет не лень, я их тут проебал

const int POISON = 0xDEAD;

static double CalculateADD(const double val1, const double val2);
static double CalculateSUB(const double val1, const double val2);
static double CalculateMUL(const double val1, const double val2);
static double CalculateDIV(const double val1, const double val2);

static double CalculatePOW(const double base, const double power);
static double CalculateLOG(const double base, const double val);

static double CalculateSIN(const double val, const double val2 = NAN);
static double CalculateCOS(const double val, const double val2 = NAN);
static double CalculateTAN(const double val, const double val2 = NAN);
static double CalculateCOT(const double val, const double val2 = NAN);

static double CalculateARCSIN(const double val, const double val2 = NAN);
static double CalculateARCCOS(const double val, const double val2 = NAN);
static double CalculateARCTAN(const double val, const double val2 = NAN);
static double CalculateARCCOT(const double val, const double val2 = NAN);

//Important - don't use for nothing else than construction of standard operations.
static MathExpressionOperationType MathExpressionOperationTypeCtor(
                                                    MathExpressionsOperationsEnum operationId,
                                                    MathExpressionOperationFormat operationFormat,
                                                    MathExpressionOperationFormat operationTexFormat, 

                                                    bool isUnaryOperation,

                                                    const char* longName,
                                                    const char* shortName,

                                                    const char* texName,
                                                    bool needTexLeftBraces,
                                                    bool needTexRightBraces,

                                                    CalculationFuncType* CalculationFunc);

static const char* const ADD_STR = "add";
static const char* const SUB_STR = "sub";
static const char* const MUL_STR = "mul";
static const char* const DIV_STR = "div";

static const char* const SIN_STR = "sin";
static const char* const COS_STR = "cos";
static const char* const TAN_STR = "tan";
static const char* const COT_STR = "cot";

static const char* const ARCSIN_STR = "arcsin";
static const char* const ARCCOS_STR = "arccos";
static const char* const ARCTAN_STR = "arctan";
static const char* const ARCCOT_STR = "arccot";

static const char* const POW_STR = "pow";
static const char* const LOG_STR = "log";

#define GENERATE_STRUCT(NAME, SHORT_CUT_STRING, FORMAT, TEX_FORMAT, IS_UNARY,                     \
                        TEX_NAME, NEED_LEFT_TEX_BRACES, NEED_RIGHT_TEX_BRACES)                    \
    static const MathExpressionOperationType NAME##_STRUCT =                                      \
                                                    MathExpressionOperationTypeCtor(              \
                                                        MathExpressionsOperationsEnum::NAME,      \
                                                        MathExpressionOperationFormat::FORMAT,    \
                                                        MathExpressionOperationFormat::TEX_FORMAT,\
                                                        IS_UNARY,                                 \
                                                        NAME##_STR, SHORT_CUT_STRING,             \
                                                        TEX_NAME,                                 \
                                                        NEED_LEFT_TEX_BRACES,                     \
                                                        NEED_RIGHT_TEX_BRACES,                    \
                                                        Calculate##NAME)

GENERATE_STRUCT(ADD, "+", INFIX,  INFIX, false, "+",      false, false);
GENERATE_STRUCT(SUB, "-", INFIX,  INFIX, false, "-",      false, false);
GENERATE_STRUCT(MUL, "*", INFIX,  INFIX, false, "\\cdot", false, false);
GENERATE_STRUCT(DIV, "/", INFIX, PREFIX, false, "\\frac", true,  true);

GENERATE_STRUCT(POW, "^",     INFIX, INFIX, false, "^",      false, true);
GENERATE_STRUCT(LOG, LOG_STR, INFIX, INFIX, false, "\\log_", true, false);

GENERATE_STRUCT(SIN, SIN_STR, PREFIX, PREFIX, true, "\\sin", false, false);
GENERATE_STRUCT(COS, COS_STR, PREFIX, PREFIX, true, "\\cos", false, false);
GENERATE_STRUCT(TAN, TAN_STR, PREFIX, PREFIX, true, "\\tan", false, false);
GENERATE_STRUCT(COT, COT_STR, PREFIX, PREFIX, true, "\\cot", false, false);

GENERATE_STRUCT(ARCSIN, ARCSIN_STR, PREFIX, PREFIX, true, "\\arcsin", false, false);
GENERATE_STRUCT(ARCCOS, ARCCOS_STR, PREFIX, PREFIX,  true, "\\arccos", false, false);
GENERATE_STRUCT(ARCTAN, ARCTAN_STR, PREFIX, PREFIX,  true, "\\arctan", false, false);
GENERATE_STRUCT(ARCCOT, ARCCOT_STR, PREFIX, PREFIX,  true, "\\arccot", false, false);

#undef GENERATE_STRUCT

static void MathExpressionDtor     (MathExpressionTokenType* token);
static void MathExpressionTokenDtor(MathExpressionTokenType* token);

static MathExpressionTokenType* MathExpressionTokenCtor(MathExpressionTokenValue value, 
                                                        MathExpressionTokenValueTypeof valueType,
                                                        MathExpressionTokenType* left  = nullptr,
                                                        MathExpressionTokenType* right = nullptr);

static void MathExpressionTokenSetEdges(MathExpressionTokenType* token, 
                                       MathExpressionTokenType* left, 
                                       MathExpressionTokenType* right);
 
static MathExpressionErrors MathExpressionPrintPrefixFormat     (
                                                const MathExpressionTokenType* token, 
                                                const MathExpressionVariablesArrayType* varsArr, 
                                                FILE* outStream);

static MathExpressionErrors MathExpressionPrintEquationFormat   (
                                                const MathExpressionTokenType* token, 
                                                const MathExpressionVariablesArrayType* varsArr,
                                                FILE* outStream);

static MathExpressionErrors MathExpressionPrintEquationFormatTex(
                                                const MathExpressionTokenType* token, 
                                                const MathExpressionVariablesArrayType* varsArr, 
                                                FILE* outStream);

static void       MathExpressionTokenPrintValue                 (
                                                const MathExpressionTokenType* token, 
                                                const MathExpressionVariablesArrayType* varsArr, 
                                                FILE* outStream);

static MathExpressionTokenType* MathExpressionReadPrefixFormat(
                                                const char* const string, 
                                                const char** stringEndPtr,
                                                MathExpressionVariablesArrayType* varsArr);

static MathExpressionTokenType* MathExpressionReadInfixFormat(
                                                const char* const string, 
                                                const char** stringEndPtr,
                                                MathExpressionVariablesArrayType* varsArr);

static const char* MathExpressionReadTokenValue(MathExpressionTokenValue* value, 
                                               MathExpressionTokenValueTypeof* valueType, 
                                               MathExpressionVariablesArrayType* varsArr,
                                               const char* stringPtr);

static bool HaveToPutBrackets(const MathExpressionTokenType* parent, 
                              const MathExpressionTokenType* son,
                              bool inTex = false);

static int         GetOperationId(const char* operationId);

static void MathExpressionGraphicDump(const MathExpressionTokenType* token, FILE* outDotFile);
static void DotFileCreateTokens(const MathExpressionTokenType* token, 
                               const MathExpressionVariablesArrayType* varsArr, FILE* outDotFile);

static inline void CreateImgInLogFile(const size_t imgIndex, bool openImg);
static inline void DotFileBegin(FILE* outDotFile);
static inline void DotFileEnd(FILE* outDotFile);

static double MathExpressionCalculate(const MathExpressionTokenType* token, 
                                      const MathExpressionVariablesArrayType* varsArr);

static inline int AddVariable(MathExpressionVariablesArrayType* varsArr,  
                              const char*  variableName, 
                              const double variableValue = 0);

static int GetVariableIdByName(const MathExpressionVariablesArrayType* varsArr, 
                               const char* variableName);

static MathExpressionTokenType* MathExpressionDifferentiate(const MathExpressionTokenType* token);
static MathExpressionTokenType* MathExpressionCopy(const MathExpressionTokenType* token);
static void MathExpressionsCopyVariables(      MathExpressionType* target, 
                                         const MathExpressionType* source);

static MathExpressionOperationType GetOperationStruct(
                                        MathExpressionsOperationsEnum operationId);

static bool IsPrefixOperation   (const MathExpressionOperationType* operation, bool inTex = false);
static bool IsUnaryOperation    (const MathExpressionOperationType* operation);

MathExpressionErrors MathExpressionCtor(MathExpressionType* expression)
{
    assert(expression);

    expression->root = nullptr;

    expression->variables.capacity = 100;
    expression->variables.size     =   0;
    expression->variables.data     = (MathExpressionVariableType*)
                                        calloc(expression->variables.capacity, 
                                               sizeof(*(expression->variables.data)));
    
    return MathExpressionErrors::NO_ERR;
}

MathExpressionErrors MathExpressionDtor(MathExpressionType* expression)
{
    assert(expression);

    MathExpressionDtor(expression->root);
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


    return MathExpressionErrors::NO_ERR;
}

static void MathExpressionDtor(MathExpressionTokenType* token)
{
    if (token == nullptr)
        return;
    
    assert(token);

    MathExpressionDtor(token->left);
    MathExpressionDtor(token->right);

    MathExpressionTokenDtor(token);
}

#define PRINT(outStream, ...)                          \
do                                                     \
{                                                      \
    if (outStream) fprintf(outStream, __VA_ARGS__);    \
    Log(__VA_ARGS__);                                  \
} while (0)

MathExpressionErrors MathExpressionPrintPrefixFormat(const MathExpressionType* expression, 
                                                     FILE* outStream)
{
    assert(expression);
    assert(outStream);

    LOG_BEGIN();

    MathExpressionErrors err = MathExpressionPrintPrefixFormat(expression->root, 
                                                               &expression->variables, 
                                                               outStream);

    PRINT(outStream, "\n");

    LOG_END();

    return err;
}

static MathExpressionErrors MathExpressionPrintPrefixFormat(
                                                    const MathExpressionTokenType* token, 
                                                    const MathExpressionVariablesArrayType* varsArr, 
                                                    FILE* outStream)
{
    if (token == nullptr)
    {
        PRINT(outStream, "nil ");
        return MathExpressionErrors::NO_ERR;
    }

    PRINT(outStream, "(");
    
    if (token->valueType == MathExpressionTokenValueTypeof::VALUE)
        PRINT(outStream, "%.2lf ", token->value.value);
    else if (token->valueType == MathExpressionTokenValueTypeof::VARIABLE)
        PRINT(outStream, "%s ", varsArr->data[token->value.varId].variableName);
    else
        PRINT(outStream, "%s ", token->value.operation.longName);

    MathExpressionErrors err = MathExpressionErrors::NO_ERR;

    err = MathExpressionPrintPrefixFormat(token->left,  varsArr, outStream);
    
    err = MathExpressionPrintPrefixFormat(token->right, varsArr, outStream);

    PRINT(outStream, ")");
    
    return err;
}

MathExpressionErrors MathExpressionPrintEquationFormat(const MathExpressionType* expression, 
                                                       FILE* outStream)
{
    assert(expression);
    assert(outStream);

    LOG_BEGIN();

    MathExpressionErrors err = MathExpressionPrintEquationFormat(expression->root, 
                                                                 &expression->variables, 
                                                                 outStream);
    PRINT(outStream, "\n");

    LOG_END();

    return err; 
}

static MathExpressionErrors MathExpressionPrintEquationFormat(
                                          const MathExpressionTokenType* token, 
                                          const MathExpressionVariablesArrayType* varsArr,
                                          FILE* outStream)
{
    if (token->left == nullptr && token->right == nullptr)
    {
        MathExpressionTokenPrintValue(token, varsArr, outStream);

        return MathExpressionErrors::NO_ERR;
    }

    assert(token->valueType == MathExpressionTokenValueTypeof::OPERATION);

    bool isPrefixOperation = IsPrefixOperation(&token->value.operation);
    if (isPrefixOperation) fprintf(outStream, token->value.operation.shortName);

    bool needLeftBrackets = HaveToPutBrackets(token, token->left);
    if (needLeftBrackets) PRINT(outStream, "(");

    MathExpressionErrors err = MathExpressionErrors::NO_ERR;
    err = MathExpressionPrintEquationFormat(token->left, varsArr, outStream);

    if (needLeftBrackets) PRINT(outStream, ")");

    if (!isPrefixOperation) fprintf(outStream, token->value.operation.shortName);

    if (IsUnaryOperation(&token->value.operation))  
        return err;

    bool needRightBrackets = HaveToPutBrackets(token, token->right);
    if (needRightBrackets) PRINT(outStream, "(");

    err = MathExpressionPrintEquationFormat(token->right, varsArr, outStream);

    if (needRightBrackets) PRINT(outStream, ")");

    return err;
}

static bool HaveToPutBrackets(const MathExpressionTokenType* parent, 
                              const MathExpressionTokenType* son,
                              bool inTex)
{
    assert(parent);

    assert(parent->valueType == MathExpressionTokenValueTypeof::OPERATION);

    if (son->valueType != MathExpressionTokenValueTypeof::OPERATION)
        return false;

    MathExpressionsOperationsEnum parentOperation = parent->value.operation.operationId;
    MathExpressionsOperationsEnum sonOperation    = son->value.operation.operationId;

    if (IsPrefixOperation(&son->value.operation, inTex))
        return false;

    if ((sonOperation    == MathExpressionsOperationsEnum::MUL  || 
         sonOperation    == MathExpressionsOperationsEnum::DIV) &&
        (parentOperation == MathExpressionsOperationsEnum::SUB  || 
         parentOperation == MathExpressionsOperationsEnum::ADD))
        return false;

    if (sonOperation    == MathExpressionsOperationsEnum::ADD && 
        parentOperation == MathExpressionsOperationsEnum::ADD)
        return false;
    
    if (sonOperation    == MathExpressionsOperationsEnum::MUL && 
        parentOperation == MathExpressionsOperationsEnum::MUL)
        return false;

    return true;
}

static bool IsPrefixOperation(const MathExpressionOperationType* operation, bool inTex)
{
    assert(operation);

    if (inTex)
        return operation->operationTexFormat == MathExpressionOperationFormat::PREFIX;

    return operation->operationFormat == MathExpressionOperationFormat::PREFIX;
}

static bool IsUnaryOperation(const MathExpressionOperationType* operation)
{
    assert(operation);

    return operation->isUnaryOperation;
}

static void MathExpressionTokenPrintValue(const MathExpressionTokenType* token, 
                                          const MathExpressionVariablesArrayType* varsArr, 
                                          FILE* outStream)
{
    assert(token->valueType != MathExpressionTokenValueTypeof::OPERATION);

    if (token->valueType == MathExpressionTokenValueTypeof::VALUE)
        PRINT(outStream, "%.2lf ", token->value.value);
    else if (token->valueType == MathExpressionTokenValueTypeof::VARIABLE)
        PRINT(outStream, "%s ", varsArr->data[token->value.varId].variableName);
}

#undef PRINT

MathExpressionErrors MathExpressionPrintEquationFormatTex(const MathExpressionType* expression, 
                                                          FILE* outStream,
                                                          const char* string)
{
    assert(expression);
    assert(outStream);

    static const size_t            numberOfRoflStrings  = 8;
    static const char* roflStrings[numberOfRoflStrings] = 
    {
        "Очевидно, что",
        "Несложно заметить, что", 
        "Любопытный читатель может показать, что",
        "Не буду утруждать себя доказательством, что",
        "Я нашел удивительное решение, но здесь маловато места, чтобы его поместить, ",
        "Без комментариев, ",
        "Это же не рокет саенс, поэтому легко видеть, что",

        "Ребят, вы че издеваетесь?"
        "Я понимаю, что вам хочется просто расслабиться и наслаждаться жизнью."
        "И не думать о дифференцировании, решении уравнений."
        "У меня просто завален весь direct"
        "\"Арман, ты же умеешь дифференцировать, продифференцируй, тебе жалко что ли?\""
        "Мне не сложно, но я не могу дифференцировать просто так! Поэтому давайте поступим так."
        "Целый год мои дифференциалы были платными."
        "Для того, чтобы получить дифференцирование, нужно было заплатить." 
        "Сегодня мне захотелось, чтобы через мой продукт смог пройти каждый."
        "Чтобы у каждого была возможность не отчислиться."
        "Потому что не каждый может позволить себе дифференциал, "
        "когда в приоритете по расходам сначала идёт семья/кредиты/ипотеки."
        "Не упусти свой шанс! Бесплатное дифференцирование: "
    };

    if (string == nullptr)
        fprintf(outStream, "%s\n", roflStrings[rand() % numberOfRoflStrings]);
    else
        fprintf(outStream, "%s\n", string);
    
    fprintf(outStream, "$");

    MathExpressionErrors err = MathExpressionPrintEquationFormatTex(expression->root, 
                                                                    &expression->variables,
                                                                    outStream);

    fprintf(outStream, "$\n");

    return err;
}

static MathExpressionErrors MathExpressionPrintEquationFormatTex(
                                             const MathExpressionTokenType* token, 
                                             const MathExpressionVariablesArrayType* varsArr, 
                                             FILE* outStream)
{
    assert(token);
    assert(outStream);

    if (token->left == nullptr && token->right == nullptr)
    {
        MathExpressionTokenPrintValue(token, varsArr, outStream);

        return MathExpressionErrors::NO_ERR;
    }

    MathExpressionErrors err = MathExpressionErrors::NO_ERR;
    assert((token->valueType == MathExpressionTokenValueTypeof::OPERATION));

    bool isPrefixOperation    = IsPrefixOperation(&token->value.operation, true);

    if (isPrefixOperation) fprintf(outStream, "%s ", token->value.operation.texName);

    bool needLeftBrackets  = HaveToPutBrackets(token, token->left);
    bool needTexLeftBraces = token->value.operation.needTexLeftBraces;

    if (needTexLeftBraces)                           fprintf(outStream, "{");
    if (!needTexLeftBraces && needLeftBrackets) fprintf(outStream, "(");

    err = MathExpressionPrintEquationFormatTex(token->left, varsArr, outStream);

    if (!needTexLeftBraces && needLeftBrackets) fprintf(outStream, ")");
    if (needTexLeftBraces)                           fprintf(outStream, "}");

    if (!isPrefixOperation) fprintf(outStream, "%s ", token->value.operation.texName);

    if (IsUnaryOperation(&token->value.operation))
        return err;

    bool needTexRightBraces   = token->value.operation.needTexRightBraces;
    bool needRightBrackets    = HaveToPutBrackets(token, token->right);
    
    if (needTexRightBraces)                            fprintf(outStream, "{");
    if (!needTexRightBraces && needRightBrackets) fprintf(outStream, "(");
    err = MathExpressionPrintEquationFormatTex(token->right, varsArr, outStream);
    if (!needTexRightBraces && needRightBrackets) fprintf(outStream, ")");
    if (needTexRightBraces)                            fprintf(outStream, "}");

    return err;   
}

MathExpressionErrors MathExpressionReadPrefixFormat(MathExpressionType* expression, FILE* inStream)
{
    assert(expression);
    assert(inStream);

    char* inputExpression = ReadText(inStream);

    if (inputExpression == nullptr)
        return MathExpressionErrors::MEM_ERR;

    const char* inputExpressionEndPtr = inputExpression;

    expression->root = MathExpressionReadPrefixFormat(inputExpression, 
                                                      &inputExpressionEndPtr, 
                                                      &expression->variables);

    free(inputExpression);

    return MathExpressionErrors::NO_ERR;
}

static MathExpressionTokenType* MathExpressionReadPrefixFormat(
                                                        const char* const string, 
                                                        const char** stringEndPtr,
                                                        MathExpressionVariablesArrayType* varsArr)
{
    assert(string);

    const char* stringPtr = string;

    stringPtr = SkipSymbolsWhileStatement(stringPtr, isspace);

    int symbol = *stringPtr;
    stringPtr++;
    if (symbol != '(') //skipping nils
    {
        int shift = 0;
        sscanf(stringPtr, "%*s%n", &shift);
        stringPtr += shift;

        *stringEndPtr = stringPtr;
        return nullptr;
    }

    MathExpressionTokenValue value;
    MathExpressionTokenValueTypeof valueType;

    stringPtr = MathExpressionReadTokenValue(&value, &valueType, varsArr, stringPtr);
    MathExpressionTokenType* token = MathExpressionTokenCtor(value, valueType);
    
    MathExpressionTokenType* left  = MathExpressionReadPrefixFormat(stringPtr, &stringPtr, varsArr);

    MathExpressionTokenType* right = nullptr;
    if (!IsUnaryOperation(&token->value.operation))
        right = MathExpressionReadPrefixFormat(stringPtr, &stringPtr, varsArr);

    stringPtr = SkipSymbolsUntilStopChar(stringPtr, ')');
    ++stringPtr;

    MathExpressionTokenSetEdges(token, left, right);

    *stringEndPtr = stringPtr;
    return token;
}

MathExpressionErrors MathExpressionReadInfixFormat (MathExpressionType* expression, FILE* inStream)
{
    assert(expression);
    assert(inStream);

    char* inputExpression = ReadText(inStream);

    if (inputExpression == nullptr)
        return MathExpressionErrors::MEM_ERR;
    
    const char* inputExpressionEndPtr = inputExpression;

    expression->root = MathExpressionReadInfixFormat(inputExpression, 
                                                     &inputExpressionEndPtr, 
                                                     &expression->variables);

    free(inputExpression);

    return MathExpressionErrors::NO_ERR;
}

static MathExpressionTokenType* MathExpressionReadInfixFormat(
                                            const char* const string, 
                                            const char** stringEndPtr,
                                            MathExpressionVariablesArrayType* varsArr)
{
    assert(string);

    const char* stringPtr = string;

    stringPtr = SkipSymbolsWhileStatement(stringPtr, isspace);

    MathExpressionTokenValue     value;
    MathExpressionTokenValueTypeof valueType;

    int symbol = *stringPtr;
    stringPtr++;

    if (symbol != '(')
    {
        --stringPtr;

        stringPtr = MathExpressionReadTokenValue(&value, &valueType, varsArr, stringPtr);
        MathExpressionTokenType* token = MathExpressionTokenCtor(value, valueType);

        *stringEndPtr = stringPtr;
        return token;
    }

    MathExpressionTokenType* left  = MathExpressionReadInfixFormat(stringPtr, &stringPtr, varsArr);

    stringPtr = MathExpressionReadTokenValue(&value, &valueType, varsArr, stringPtr);
    MathExpressionTokenType* token = MathExpressionTokenCtor(value, valueType);
    MathExpressionTokenType* right = MathExpressionReadInfixFormat(stringPtr, &stringPtr, varsArr);

    MathExpressionTokenSetEdges(token, left, right);

    stringPtr = SkipSymbolsWhileChar(stringPtr, ')');

    *stringEndPtr = stringPtr;
    return token;
}

static const char* MathExpressionReadTokenValue(MathExpressionTokenValue* value, 
                                               MathExpressionTokenValueTypeof* valueType, 
                                               MathExpressionVariablesArrayType* varsArr,
                                               const char* string)
{
    assert(value);
    assert(string);
    assert(valueType);

    double readenValue = POISON;
    int shift = 0;
    int scanResult = sscanf(string, "%lf%n\n", &readenValue, &shift);

    if (scanResult != 0)
    {
        value->value = readenValue;
        *valueType   = MathExpressionTokenValueTypeof::VALUE;
        return string + shift;
    }

    shift = 0;

    static const size_t      maxInputStringSize  = 128;
    static char  inputString[maxInputStringSize] =  "";

    const char* stringPtr = string;
    sscanf(string, "%s%n", inputString, &shift);

    stringPtr = string + shift;
    assert(isspace(*stringPtr));

    int operationId = GetOperationId(inputString);
    if (operationId != -1)
    {
        //TODO: подумать над именем функции
        value->operation = GetOperationStruct((MathExpressionsOperationsEnum) operationId);
        *valueType                   = MathExpressionTokenValueTypeof::OPERATION;
        return stringPtr;
    }

    int varId = GetVariableIdByName(varsArr, inputString);
    if (varId == -1)
        varId = AddVariable(varsArr, inputString);

    assert(varId != -1);

    value->varId = varId;
    *valueType   = MathExpressionTokenValueTypeof::VARIABLE;

    return stringPtr;
}

static MathExpressionOperationType GetOperationStruct(
                                        MathExpressionsOperationsEnum operationId)
{
    switch (operationId)
    {
        case MathExpressionsOperationsEnum::ADD:
            return ADD_STRUCT;
        case MathExpressionsOperationsEnum::SUB:
            return SUB_STRUCT;
        case MathExpressionsOperationsEnum::MUL:
            return MUL_STRUCT;
        case MathExpressionsOperationsEnum::DIV:
            return DIV_STRUCT;    

        case MathExpressionsOperationsEnum::POW:
            return POW_STRUCT;
        case MathExpressionsOperationsEnum::LOG:
            return LOG_STRUCT;

        case MathExpressionsOperationsEnum::SIN:
            return SIN_STRUCT;
        case MathExpressionsOperationsEnum::COS:
            return COS_STRUCT;
        case MathExpressionsOperationsEnum::TAN:
            return TAN_STRUCT;
        case MathExpressionsOperationsEnum::COT:
            return COT_STRUCT;    

        case MathExpressionsOperationsEnum::ARCSIN:
            return ARCSIN_STRUCT;
        case MathExpressionsOperationsEnum::ARCCOS:
            return ARCCOS_STRUCT;
        case MathExpressionsOperationsEnum::ARCTAN:
            return ARCTAN_STRUCT;
        case MathExpressionsOperationsEnum::ARCCOT:
            return ARCCOT_STRUCT;    

        default:
            break;
    }

    return {};
}

static int GetOperationId(const char* string)
{
    assert(string);

    //TODO: можно создать массив с моими стандартными структурами и пройтись по нему, это лучше выглядеть будет
    if (     strcasecmp(string, "/") == 0 || strcasecmp(string, DIV_STR) == 0)
        return (int)MathExpressionsOperationsEnum::DIV;
    else if (strcasecmp(string, "*") == 0 || strcasecmp(string, MUL_STR) == 0)
        return (int)MathExpressionsOperationsEnum::MUL;
    else if (strcasecmp(string, "-") == 0 || strcasecmp(string, SUB_STR) == 0)
        return (int)MathExpressionsOperationsEnum::SUB;
    else if (strcasecmp(string, "+") == 0 || strcasecmp(string, ADD_STR) == 0)
        return (int)MathExpressionsOperationsEnum::ADD;

    else if (strcasecmp(string, POW_STR) == 0)
        return (int)MathExpressionsOperationsEnum::POW;

    else if (strcasecmp(string, LOG_STR) == 0)
        return (int)MathExpressionsOperationsEnum::LOG;

    else if (strcasecmp(string, SIN_STR) == 0)
        return (int)MathExpressionsOperationsEnum::SIN;
    else if (strcasecmp(string, COS_STR) == 0)
        return (int)MathExpressionsOperationsEnum::COS;
    else if (strcasecmp(string, TAN_STR) == 0)
        return (int)MathExpressionsOperationsEnum::TAN;
    else if (strcasecmp(string, COT_STR) == 0)
        return (int)MathExpressionsOperationsEnum::COT;
    
    else if (strcasecmp(string, ARCSIN_STR) == 0)
        return (int)MathExpressionsOperationsEnum::ARCSIN;
    else if (strcasecmp(string, ARCCOS_STR) == 0)
        return (int)MathExpressionsOperationsEnum::ARCCOS;
    else if (strcasecmp(string, ARCTAN_STR) == 0)
        return (int)MathExpressionsOperationsEnum::ARCTAN;
    else if (strcasecmp(string, ARCCOT_STR) == 0)
        return (int)MathExpressionsOperationsEnum::ARCCOT;
    
    return -1;
}

static MathExpressionTokenType* MathExpressionTokenCtor(MathExpressionTokenValue value, 
                                                        MathExpressionTokenValueTypeof valueType,
                                                        MathExpressionTokenType* left,
                                                        MathExpressionTokenType* right)
{   
    MathExpressionTokenType* token = (MathExpressionTokenType*)calloc(1, sizeof(*token));
    token->left      = left;
    token->right     = right;
    token->value     = value;
    token->valueType = valueType;
    
    return token;
}

static void MathExpressionTokenDtor(MathExpressionTokenType* token)
{
    token->left        = nullptr;
    token->right       = nullptr;
    token->value.varId =  POISON;

    free(token);
}

static inline void CreateImgInLogFile(const size_t imgIndex, bool openImg)
{
    static const size_t maxImgNameLength  = 64;
    static char imgName[maxImgNameLength] = "";
    snprintf(imgName, maxImgNameLength, "../imgs/img_%zu_time_%s.png", imgIndex, __TIME__);

    static const size_t     maxCommandLength  = 128;
    static char commandName[maxCommandLength] =  "";
    snprintf(commandName, maxCommandLength, "dot MathExpressionHandler.dot -T png -o %s", imgName);
    system(commandName);

    snprintf(commandName, maxCommandLength, "<img src = \"%s\">\n", imgName);    
    Log(commandName);

    if (openImg)
    {
        snprintf(commandName, maxCommandLength, "open %s", imgName);
        system(commandName);
    }
}

static inline void DotFileBegin(FILE* outDotFile)
{
    fprintf(outDotFile, "digraph G{\nrankdir=TB;\ngraph [bgcolor=\"#31353b\"];\n"
                        "edge[color=\"#00D0D0\"];\n");
}

static inline void DotFileEnd(FILE* outDotFile)
{
    fprintf(outDotFile, "\n}\n");
}

void MathExpressionGraphicDump(const MathExpressionType* expression, bool openImg)
{
    assert(expression);

    static const char* dotFileName = "MathExpressionHandler.dot";
    FILE* outDotFile = fopen(dotFileName, "w");

    if (outDotFile == nullptr)
        return;

    DotFileBegin(outDotFile);

    DotFileCreateTokens(expression->root, &expression->variables, outDotFile);

    MathExpressionGraphicDump(expression->root, outDotFile);

    DotFileEnd(outDotFile);

    fclose(outDotFile);

    static size_t imgIndex = 0;
    CreateImgInLogFile(imgIndex, openImg);
    imgIndex++;
}

static void DotFileCreateTokens(const MathExpressionTokenType* token,    
                               const MathExpressionVariablesArrayType* varsArr, FILE* outDotFile)
{
    assert(varsArr);
    assert(outDotFile);

    if (token == nullptr)
        return;
    
    fprintf(outDotFile, "token%p"
                        "[shape=Mrecord, style=filled, ", token);

    if (token->valueType == MathExpressionTokenValueTypeof::OPERATION)
        fprintf(outDotFile, "fillcolor=\"#89AC76\", label = \"%s\", ", 
                            token->value.operation.longName);
    else if (token->valueType == MathExpressionTokenValueTypeof::VALUE)
        fprintf(outDotFile, "fillcolor=\"#7293ba\", label = \"%.2lf\", ", token->value.value);
    else if (token->valueType == MathExpressionTokenValueTypeof::VARIABLE)
        fprintf(outDotFile, "fillcolor=\"#78DBE2\", label = \"%s\", ", 
                            varsArr->data[token->value.varId].variableName);
    else 
        fprintf(outDotFile, "fillcolor=\"#FF0000\", label = \"ERROR\", ");

    fprintf(outDotFile, "color = \"#D0D000\"];\n");

    DotFileCreateTokens(token->left,  varsArr, outDotFile);
    DotFileCreateTokens(token->right, varsArr, outDotFile);
}

static void MathExpressionGraphicDump(const MathExpressionTokenType* token, FILE* outDotFile)
{
    if (token == nullptr)
    {
        fprintf(outDotFile, "\n");
        return;
    }
    
    fprintf(outDotFile, "token%p;\n", token);

    if (token->left != nullptr) fprintf(outDotFile, "token%p->", token);
    MathExpressionGraphicDump(token->left, outDotFile);

    if (token->right != nullptr) fprintf(outDotFile, "token%p->", token);
    MathExpressionGraphicDump(token->right, outDotFile);
}

void MathExpressionTextDump(const MathExpressionType* expression, const char* fileName, 
                                            const char* funcName,
                                            const int   line)
{
    assert(expression);
    assert(fileName);
    assert(funcName);

    LogBegin(fileName, funcName, line);

    Log("Tree root: %p, value: %s\n", expression->root, expression->root->value);
    Log("Tree: ");
    MathExpressionPrintPrefixFormat(expression, nullptr);

    LOG_END();
}

void MathExpressionDump(const MathExpressionType* expression, const char* fileName,
                                                              const char* funcName,
                                                              const int   line)
{
    assert(expression);
    assert(fileName);
    assert(funcName);

    MathExpressionTextDump(expression, fileName, funcName, line);

    MathExpressionGraphicDump(expression);
}

double MathExpressionCalculate(const MathExpressionType* expression)
{
    assert(expression);

    return MathExpressionCalculate(expression->root, &expression->variables);
}

static double MathExpressionCalculate(const MathExpressionTokenType* token, 
                                      const MathExpressionVariablesArrayType* varsArr)
{
    if (token == nullptr)
        return NAN;
    
    if (token->valueType == MathExpressionTokenValueTypeof::VALUE)
        return token->value.value;

    if (token->valueType == MathExpressionTokenValueTypeof::VARIABLE)
        return varsArr->data[token->value.varId].variableValue;

    double firstVal  = MathExpressionCalculate(token->left,  varsArr);
    double secondVal = MathExpressionCalculate(token->right, varsArr);
    
    return token->value.operation.CalculationFunc(firstVal, secondVal);
}

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

static inline int AddVariable(MathExpressionVariablesArrayType* varsArr,  
                              const char*  variableName, 
                              const double variableValue)
{
    assert(varsArr);
    assert(variableName);
    assert(varsArr->size < varsArr->capacity);

    varsArr->data[varsArr->size].variableName  = strdup(variableName);

    assert(varsArr->data[varsArr->size].variableName);
    if (varsArr->data[varsArr->size].variableName == nullptr)
        return -1;
    
    varsArr->data[varsArr->size].variableValue = variableValue;
    varsArr->size++;

    return (int)varsArr->size - 1;
}

static int GetVariableIdByName(const MathExpressionVariablesArrayType* varsArr, 
                               const char* variableName)
{
    assert(varsArr);
    assert(variableName);

    for (size_t i = 0; i < varsArr->size; ++i)
    {
        if (strcmp(varsArr->data[i].variableName, variableName) == 0)
            return (int)i;
    }
    
    return -1;
}

MathExpressionErrors MathExpressionReadVariables(MathExpressionType* expression)
{
    assert(expression);

    printf("Enter variables values: \n");
    for (size_t i = 0; i < expression->variables.size; ++i)
    {
        printf("%s: ", expression->variables.data[i].variableName);
        int scanfResult = scanf("%lf",  &expression->variables.data[i].variableValue);

        if (scanfResult == 0)
            return MathExpressionErrors::READING_ERR;
    }

    return MathExpressionErrors::NO_ERR;
}

MathExpressionType MathExpressionDifferentiate(const MathExpressionType* expression)
{
    assert(expression);
    
    MathExpressionTokenType* root = MathExpressionDifferentiate(expression->root);

    MathExpressionType diffMathExpression = {};
    MathExpressionCtor(&diffMathExpression);
    diffMathExpression.root = root;

    MathExpressionsCopyVariables(&diffMathExpression, expression);

    return diffMathExpression;
}

static inline MathExpressionTokenValue MathExpressionCreateTokenValue(double value)
{
    MathExpressionTokenValue tokenValue =
    {
        .value = value
    };

    return tokenValue;
}

static inline MathExpressionTokenValue MathExpressionCreateTokenValue(
                                                        MathExpressionsOperationsEnum operationId)
{
    MathExpressionTokenValue value =
    {
        .operation = GetOperationStruct(operationId),
    };

    return value;
}

static inline MathExpressionTokenType* MathExpressionCreateNumericToken(double value)
{
    MathExpressionTokenValue tokenVal = MathExpressionCreateTokenValue(value);

    return MathExpressionTokenCtor(tokenVal, MathExpressionTokenValueTypeof::VALUE);
}

#define D(TOKEN) MathExpressionDifferentiate(TOKEN)
#define C(TOKEN) MathExpressionCopy(TOKEN)

#define CONST_TOKEN(VALUE) MathExpressionCreateNumericToken(VALUE)

#define TOKEN(OPERATION_NAME, LEFT_TOKEN, RIGHT_TOKEN)                                        \
    MathExpressionTokenCtor(MathExpressionCreateTokenValue(                                   \
                                            MathExpressionsOperationsEnum::OPERATION_NAME),   \
                            MathExpressionTokenValueTypeof::OPERATION,                        \
                            LEFT_TOKEN, RIGHT_TOKEN)                                               

#define UNARY_TOKEN(OPERATION_NAME, LEFT_TOKEN) TOKEN(OPERATION_NAME, LEFT_TOKEN, nullptr)

static inline MathExpressionTokenType* MathExpressionDifferentiateAdd(
                                                            const MathExpressionTokenType* token)
{
    assert(token);
    assert(token->valueType == MathExpressionTokenValueTypeof::OPERATION);
    assert(token->value.operation.operationId == MathExpressionsOperationsEnum::ADD);

    return TOKEN(ADD, D(token->left), D(token->right));
}

static inline MathExpressionTokenType* MathExpressionDifferentiateSub(
                                                            const MathExpressionTokenType* token)
{
    assert(token);
    assert(token->valueType == MathExpressionTokenValueTypeof::OPERATION);
    assert(token->value.operation.operationId == MathExpressionsOperationsEnum::SUB);

    return TOKEN(SUB, D(token->left), D(token->right));
}

static inline MathExpressionTokenType* MathExpressionDifferentiateMul(
                                                            const MathExpressionTokenType* token)
{
    assert(token);
    assert(token->valueType == MathExpressionTokenValueTypeof::OPERATION);
    assert(token->value.operation.operationId == MathExpressionsOperationsEnum::MUL);

    return TOKEN(ADD, TOKEN(MUL, D(token->left), C(token->right)), 
                      TOKEN(MUL, C(token->left), D(token->right)));
}

static inline MathExpressionTokenType* MathExpressionDifferentiateDiv(
                                                            const MathExpressionTokenType* token)
{
    assert(token);
    assert(token->valueType == MathExpressionTokenValueTypeof::OPERATION);
    assert(token->value.operation.operationId == MathExpressionsOperationsEnum::DIV);

    return TOKEN(DIV, TOKEN(SUB, TOKEN(MUL, D(token->left), C(token->right)), 
                                 TOKEN(MUL, C(token->left), D(token->right))),
                      TOKEN(MUL, C(token->right), C(token->right)));
}

static inline MathExpressionTokenType* MathExpressionDifferentiateSin(
                                                            const MathExpressionTokenType* token)
{
    assert(token);
    assert(token->valueType == MathExpressionTokenValueTypeof::OPERATION);
    assert(token->value.operation.operationId == MathExpressionsOperationsEnum::SIN);

    return TOKEN(MUL, UNARY_TOKEN(COS, C(token->left)), D(token->left));
}

static inline MathExpressionTokenType* MathExpressionDifferentiateCos(
                                                            const MathExpressionTokenType* token)
{
    assert(token);
    assert(token->valueType == MathExpressionTokenValueTypeof::OPERATION);
    assert(token->value.operation.operationId == MathExpressionsOperationsEnum::COS);

    return TOKEN(MUL, CONST_TOKEN(-1), 
                      TOKEN(MUL, UNARY_TOKEN(SIN, C(token->left)), D(token->left)));
}

static inline MathExpressionTokenType* MathExpressionDifferentiateTan(
                                                            const MathExpressionTokenType* token)
{
    assert(token);
    assert(token->valueType == MathExpressionTokenValueTypeof::OPERATION);
    assert(token->value.operation.operationId == MathExpressionsOperationsEnum::TAN);

    return TOKEN(DIV, D(token->left), 
                      TOKEN(POW, UNARY_TOKEN(COS, C(token->left)), CONST_TOKEN(2)));
}

static inline MathExpressionTokenType* MathExpressionDifferentiateCot(
                                                            const MathExpressionTokenType* token)
{
    assert(token);
    assert(token->valueType == MathExpressionTokenValueTypeof::OPERATION);
    assert(token->value.operation.operationId == MathExpressionsOperationsEnum::COT);

    return TOKEN(MUL, CONST_TOKEN(-1), 
                      TOKEN(DIV, D(token->left), 
                                 TOKEN(POW, UNARY_TOKEN(SIN, C(token->left)), CONST_TOKEN(2))));
} 

static inline MathExpressionTokenType* MathExpressionDifferentiateArcsin(
                                                            const MathExpressionTokenType* token)
{
    assert(token);
    assert(token->valueType == MathExpressionTokenValueTypeof::OPERATION);
    assert(token->value.operation.operationId == MathExpressionsOperationsEnum::ARCSIN);

    return TOKEN(DIV, D(token->left),
                      TOKEN(POW, TOKEN(SUB, CONST_TOKEN(1), 
                                            TOKEN(POW, C(token->left), CONST_TOKEN(2))),
                                 CONST_TOKEN(0.5)));
} 

static inline MathExpressionTokenType* MathExpressionDifferentiateArccos(
                                                            const MathExpressionTokenType* token)
{
    assert(token);
    assert(token->valueType == MathExpressionTokenValueTypeof::OPERATION);
    assert(token->value.operation.operationId == MathExpressionsOperationsEnum::ARCCOS);

    return TOKEN(DIV, CONST_TOKEN(-1),
                      TOKEN(MUL, D(token->left),
                                 TOKEN(POW, TOKEN(SUB, CONST_TOKEN(1), 
                                                       TOKEN(POW, C(token->left), CONST_TOKEN(2))),
                                            CONST_TOKEN(0.5))));
}

static inline MathExpressionTokenType* MathExpressionDifferentiateArctan(
                                                            const MathExpressionTokenType* token)
{
    assert(token);
    assert(token->valueType == MathExpressionTokenValueTypeof::OPERATION);
    assert(token->value.operation.operationId == MathExpressionsOperationsEnum::ARCTAN);

    return TOKEN(DIV, D(token->left), 
                      TOKEN(ADD, CONST_TOKEN(1),
                                 TOKEN(POW, C(token->left), CONST_TOKEN(2))));
}

static inline MathExpressionTokenType* MathExpressionDifferentiateArccot(
                                                            const MathExpressionTokenType* token)
{
    assert(token);
    assert(token->valueType == MathExpressionTokenValueTypeof::OPERATION);
    assert(token->value.operation.operationId == MathExpressionsOperationsEnum::ARCTAN);

    return TOKEN(MUL, CONST_TOKEN(-1),
                      TOKEN(DIV, D(token->left),
                                 TOKEN(ADD, CONST_TOKEN(1),
                                            TOKEN(POW, C(token->left), CONST_TOKEN(2)))));
}

static MathExpressionTokenType* MathExpressionDifferentiate(const MathExpressionTokenType* token)
{
    assert(token);

    switch(token->valueType)
    {
        case MathExpressionTokenValueTypeof::VALUE:
        {
            MathExpressionTokenValue val = {};
            val.value = 0;

            return MathExpressionTokenCtor(val, MathExpressionTokenValueTypeof::VALUE);
        }
        case MathExpressionTokenValueTypeof::VARIABLE:
        {
            MathExpressionTokenValue val = {};
            val.value = 1;

            return MathExpressionTokenCtor(val, MathExpressionTokenValueTypeof::VALUE);
        }
        case MathExpressionTokenValueTypeof::OPERATION:
        default:
            break;
    }

    assert(token->valueType == MathExpressionTokenValueTypeof::OPERATION);
    
    switch (token->value.operation.operationId)
    {
        case MathExpressionsOperationsEnum::ADD:
            return MathExpressionDifferentiateAdd(token);    
        case MathExpressionsOperationsEnum::SUB:
            return MathExpressionDifferentiateSub(token);
        case MathExpressionsOperationsEnum::MUL:
            return MathExpressionDifferentiateMul(token);
        case MathExpressionsOperationsEnum::DIV:
            return MathExpressionDifferentiateDiv(token);

        case MathExpressionsOperationsEnum::POW:
            break; //TODO: самая большая, оставляю на потом
        case MathExpressionsOperationsEnum::LOG:
            break; //TODO: тоже большая, попозже

        case MathExpressionsOperationsEnum::SIN:
            return MathExpressionDifferentiateSin(token);
        case MathExpressionsOperationsEnum::COS:
            return MathExpressionDifferentiateCos(token);
        case MathExpressionsOperationsEnum::TAN:
            return MathExpressionDifferentiateTan(token);
        case MathExpressionsOperationsEnum::COT:
            return MathExpressionDifferentiateCot(token);

        case MathExpressionsOperationsEnum::ARCSIN:
            return MathExpressionDifferentiateArcsin(token);
        case MathExpressionsOperationsEnum::ARCCOS:
            return MathExpressionDifferentiateArccos(token);
        case MathExpressionsOperationsEnum::ARCTAN:
            return MathExpressionDifferentiateArctan(token);
        case MathExpressionsOperationsEnum::ARCCOT:
            return MathExpressionDifferentiateArccot(token);
                  
        default:
            break;
    }
}

static MathExpressionTokenType* MathExpressionCopy(const MathExpressionTokenType* token)
{
    if (token == nullptr)
        return nullptr;

    MathExpressionTokenType* left  = MathExpressionCopy(token->left);
    MathExpressionTokenType* right = MathExpressionCopy(token->right);

    return MathExpressionTokenCtor(token->value, token->valueType, left, right);
}

static void MathExpressionsCopyVariables(      MathExpressionType* target, 
                                         const MathExpressionType* source)
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

static void MathExpressionTokenSetEdges(MathExpressionTokenType* token, MathExpressionTokenType* left, 
                                                     MathExpressionTokenType* right)
{
    assert(token);

    token->left  = left;
    token->right = right;
}

static MathExpressionOperationType MathExpressionOperationTypeCtor(
                                                    MathExpressionsOperationsEnum operationId,
                                                    MathExpressionOperationFormat operationFormat,
                                                    MathExpressionOperationFormat operationTexFormat,
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

    MathExpressionOperationType operation = 
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
