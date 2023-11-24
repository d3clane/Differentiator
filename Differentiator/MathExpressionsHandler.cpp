#include <assert.h>
#include <ctype.h>
#include <string.h>
#include <math.h>

#include "MathExpressionsHandler.h"
#include "../Common/StringFuncs.h"
#include "../Common/Log.h"
#include "../FastInput/InputOutput.h"
#include "../Common/DoubleFuncs.h"

//TODO: накидать inline когда будет не лень, я их тут проебал

const int POISON = 0xDEAD;

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

static void MathExpressionDtor    (MathExpressionTokenType* token);
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
                              const MathExpressionTokenType* son);

static int         GetOperation(const char* operation);
static const char* GetOperationLongName (const MathExpressionsOperations operation);
static const char* GetOperationShortName(const MathExpressionsOperations operation);

static void MathExpressionGraphicDump(const MathExpressionTokenType* token, FILE* outDotFile);
static void DotFileCreateTokens(const MathExpressionTokenType* token, 
                               const MathExpressionVariablesArrayType* varsArr, FILE* outDotFile);

static inline void CreateImgInLogFile(const size_t imgIndex, bool openImg);
static inline void DotFileBegin(FILE* outDotFile);
static inline void DotFileEnd(FILE* outDotFile);

static double MathExpressionCalculate(const MathExpressionTokenType* token, 
                                      const MathExpressionVariablesArrayType* varsArr);
static double MathExpressionCalculateUsingTokenOperation(const MathExpressionsOperations operation,
                                                        double firstVal, double secondVal);

static inline int AddVariable(MathExpressionVariablesArrayType* varsArr,  
                              const char*  variableName, 
                              const double variableValue = 0);

static int GetVariableIdByName(const MathExpressionVariablesArrayType* varsArr, 
                               const char* variableName);

static MathExpressionTokenType* MathExpressionDifferentiate(const MathExpressionTokenType* token);
static MathExpressionTokenType* MathExpressionCopy(const MathExpressionTokenType* token);
static void MathExpressionsCopyVariables(      MathExpressionType* target, 
                                         const MathExpressionType* source);

static bool IsPrefixFunction(const MathExpressionTokenType* token);

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
        PRINT(outStream, "%lf ", token->value.value);
    else if (token->valueType == MathExpressionTokenValueTypeof::VARIABLE)
        PRINT(outStream, "%s ", varsArr->data[token->value.varId].variableName);
    else
        PRINT(outStream, "%s ", GetOperationLongName(token->value.operation));

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

    MathExpressionErrors err = MathExpressionErrors::NO_ERR;
    bool isPrefixOperator = IsPrefixFunction(token);

    assert(token->valueType == MathExpressionTokenValueTypeof::OPERATION);

    if (isPrefixOperator) MathExpressionTokenPrintValue(token, varsArr, outStream);

    bool haveToPutLeftBrackets = (token->left->valueType == MathExpressionTokenValueTypeof::OPERATION) &&
                                  HaveToPutBrackets(token, token->left);
    
    if (haveToPutLeftBrackets) PRINT(outStream, "(");
    err = MathExpressionPrintEquationFormat(token->left, varsArr, outStream);
    if (haveToPutLeftBrackets) PRINT(outStream, ")");

    if (!isPrefixOperator) MathExpressionTokenPrintValue(token, varsArr, outStream);
    
    bool haveToPutRightBrackets = (token->right->valueType == MathExpressionTokenValueTypeof::OPERATION) &&
                                   HaveToPutBrackets(token, token->right);
    if (haveToPutRightBrackets) PRINT(outStream, "(");
    err = MathExpressionPrintEquationFormat(token->right, varsArr, outStream);
    if (haveToPutRightBrackets) PRINT(outStream, ")");

    return err;
}

static bool HaveToPutBrackets(const MathExpressionTokenType* parent, 
                              const MathExpressionTokenType* son)
{
    assert(parent);
    assert(parent->valueType == MathExpressionTokenValueTypeof::OPERATION);
    assert(son->valueType    == MathExpressionTokenValueTypeof::OPERATION);

    MathExpressionsOperations parentOperation = parent->value.operation;
    MathExpressionsOperations sonOperation    = son->value.operation;

    if ((sonOperation    == MathExpressionsOperations::MUL  || 
         sonOperation    == MathExpressionsOperations::DIV) &&
        (parentOperation == MathExpressionsOperations::SUB  || 
         parentOperation == MathExpressionsOperations::ADD))
        return false;

    if (sonOperation    == MathExpressionsOperations::ADD && 
        parentOperation == MathExpressionsOperations::ADD)
        return false;
    
    if (sonOperation    == MathExpressionsOperations::MUL && 
        parentOperation == MathExpressionsOperations::MUL)
        return false;

    return true;
}

static bool IsPrefixFunction(const MathExpressionTokenType* token)
{
    assert(token);

    return token->value.operation.operationType;
        
    if (token->valueType != MathExpressionTokenValueTypeof::OPERATION)
        return false;

    if ((token->value.operation == MathExpressionsOperations::ADD) ||
        (token->value.operation == MathExpressionsOperations::SUB) ||
        (token->value.operation == MathExpressionsOperations::MUL) ||
        (token->value.operation == MathExpressionsOperations::DIV) ||
        (token->value.operation == MathExpressionsOperations::))
        return true;
    
}

static void MathExpressionTokenPrintValue(const MathExpressionTokenType* token, 
                                         const MathExpressionVariablesArrayType* varsArr, 
                                         FILE* outStream)
{
    if (token->valueType == MathExpressionTokenValueTypeof::VALUE)
        PRINT(outStream, "%lf ", token->value.value);
    else if (token->valueType == MathExpressionTokenValueTypeof::VARIABLE)
        PRINT(outStream, "%s ", varsArr->data[token->value.varId].variableName);
    else
        PRINT(outStream, "%s ", GetOperationShortName(token->value.operation));
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

    bool isDivideOperation     = (token->valueType       == MathExpressionTokenValueTypeof::OPERATION) &&
                                 (token->value.operation == MathExpressionsOperations::DIV);
    bool haveToPutLeftBrackets = (token->left->valueType == MathExpressionTokenValueTypeof::OPERATION) &&
                                 (HaveToPutBrackets(token, token->left));

    if (isDivideOperation)                           fprintf(outStream, "\\frac{");
    if (!isDivideOperation && haveToPutLeftBrackets) fprintf(outStream, "(");
    err = MathExpressionPrintEquationFormatTex(token->left, varsArr, outStream);
    if (!isDivideOperation && haveToPutLeftBrackets) fprintf(outStream, ")");
    if (isDivideOperation)                           fprintf(outStream, "}");

    if (!isDivideOperation) MathExpressionTokenPrintValue(token, varsArr, outStream);
    
    bool haveToPutRightBrackets = (token->right->valueType == MathExpressionTokenValueTypeof::OPERATION) &&
                                   (HaveToPutBrackets(token, token->right));
    
    if (isDivideOperation)                            fprintf(outStream, "{");
    if (!isDivideOperation && haveToPutRightBrackets) fprintf(outStream, "(");
    err = MathExpressionPrintEquationFormatTex(token->right, varsArr, outStream);
    if (!isDivideOperation && haveToPutRightBrackets) fprintf(outStream, ")");
    if (isDivideOperation)                            fprintf(outStream, "}");

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
    MathExpressionTokenType* right = MathExpressionReadPrefixFormat(stringPtr, &stringPtr, varsArr);

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

    int operation = GetOperation(inputString);
    if (operation != -1)
    {
        value->operation = (MathExpressionsOperations) operation;
        *valueType       = MathExpressionTokenValueTypeof::OPERATION;
        
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

static int GetOperation(const char* string)
{
    assert(string);

    if (     strcasecmp(string, "/") == 0 || strcasecmp(string, DIV_STR) == 0)
        return (int)MathExpressionsOperations::DIV;
    else if (strcasecmp(string, "*") == 0 || strcasecmp(string, MUL_STR) == 0)
        return (int)MathExpressionsOperations::MUL;
    else if (strcasecmp(string, "-") == 0 || strcasecmp(string, SUB_STR) == 0)
        return (int)MathExpressionsOperations::SUB;
    else if (strcasecmp(string, "+") == 0 || strcasecmp(string, ADD_STR) == 0)
        return (int)MathExpressionsOperations::ADD;

    else if (strcasecmp(string, POW_STR) == 0)
        return (int)MathExpressionsOperations::POW;

    else if (strcasecmp(string, LOG_STR) == 0)
        return (int)MathExpressionsOperations::LOG;

    else if (strcasecmp(string, SIN_STR) == 0)
        return (int)MathExpressionsOperations::SIN;
    else if (strcasecmp(string, COS_STR) == 0)
        return (int)MathExpressionsOperations::COS;
    else if (strcasecmp(string, TAN_STR) == 0)
        return (int)MathExpressionsOperations::TAN;
    else if (strcasecmp(string, COT_STR) == 0)
        return (int)MathExpressionsOperations::COT;
    
    else if (strcasecmp(string, ARCSIN_STR) == 0)
        return (int)MathExpressionsOperations::ARCSIN;
    else if (strcasecmp(string, ARCCOS_STR) == 0)
        return (int)MathExpressionsOperations::ARCCOS;
    else if (strcasecmp(string, ARCTAN_STR) == 0)
        return (int)MathExpressionsOperations::ARCTAN;
    else if (strcasecmp(string, ARCCOT_STR) == 0)
        return (int)MathExpressionsOperations::ARCCOT;
    
    return -1;
}

static const char* GetOperationLongName(const MathExpressionsOperations operation)
{
    switch (operation)
    {
        case MathExpressionsOperations::MUL:
            return MUL_STR;
        case MathExpressionsOperations::DIV:
            return DIV_STR;
        case MathExpressionsOperations::SUB:
            return SUB_STR;
        case MathExpressionsOperations::ADD:
            return ADD_STR;
        
        case MathExpressionsOperations::POW:
            return POW_STR;
        case MathExpressionsOperations::LOG:
            return LOG_STR;
        
        case MathExpressionsOperations::SIN:
            return SIN_STR;
        case MathExpressionsOperations::COS:
            return COS_STR;
        case MathExpressionsOperations::TAN:
            return TAN_STR;
        case MathExpressionsOperations::COT:
            return COT_STR;
        
        case MathExpressionsOperations::ARCSIN:
            return ARCSIN_STR;
        case MathExpressionsOperations::ARCCOS:
            return ARCCOS_STR;
        case MathExpressionsOperations::ARCTAN:
            return ARCTAN_STR;
        case MathExpressionsOperations::ARCCOT:
            return ARCCOT_STR;
        
        default:
            return nullptr;
    }

    return nullptr;
}

static const char* GetOperationShortName(const MathExpressionsOperations operation)
{

#pragma GCC diagnostic ignored "-Wswitch-enum"

    switch (operation)
    {
        case MathExpressionsOperations::MUL:
            return "*";
        case MathExpressionsOperations::DIV:
            return "/";
        case MathExpressionsOperations::SUB:
            return "-";
        case MathExpressionsOperations::ADD:
            return "+";
        
        default:
            break;
    }
#pragma GCC diagnostic warning "-Wswitch-enum"

    //все остальные случаи уходят сюда
    return GetOperationLongName(operation);
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
    if (token == nullptr)
        return;
    
    fprintf(outDotFile, "token%p"
                        "[shape=Mrecord, style=filled, ", token);
    
    if (token->valueType == MathExpressionTokenValueTypeof::OPERATION)
        fprintf(outDotFile, "fillcolor=\"#89AC76\", label = \"%s\", ", 
                            GetOperationLongName(token->value.operation));
    else if (token->valueType == MathExpressionTokenValueTypeof::VALUE)
        fprintf(outDotFile, "fillcolor=\"#7293ba\", label = \"%lf\", ", token->value.value);
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
    assert(token);

    if (token->valueType == MathExpressionTokenValueTypeof::VALUE)
        return token->value.value;

    if (token->valueType == MathExpressionTokenValueTypeof::VARIABLE)
        return varsArr->data[token->value.varId].variableValue;

    double firstVal  = MathExpressionCalculate(token->left,  varsArr);
    double secondVal = MathExpressionCalculate(token->right, varsArr);
    
    return MathExpressionCalculateUsingTokenOperation(token->value.operation, firstVal, secondVal);
}

static double CalculateLog(const double base, const double value)
{
    assert(DoubleLess(0, base));
    assert(DoubleLess(0, value));

    double log_Base = log(base);
    
    assert(!DoubleEqual(log_Base, 0));

    return log(value) / log_Base;
}

static double CalculateCot(const double val)
{
    assert(isfinite(val));

    double tan_val = tan(val);

    assert(!DoubleEqual(tan_val, 0));

    return 1 / tan_val;
}

static double MathExpressionCalculateUsingTokenOperation(const MathExpressionsOperations operation, 
                                                         double firstVal, double secondVal = NAN)
{
    assert(isfinite(firstVal));

    switch(operation)
    {
        case MathExpressionsOperations::ADD:
            assert(isfinite(secondVal));
            return firstVal + secondVal;
        case MathExpressionsOperations::SUB:
            assert(isfinite(secondVal));
            return firstVal - secondVal;
        case MathExpressionsOperations::MUL:
            assert(isfinite(secondVal));
            return firstVal * secondVal;
        case MathExpressionsOperations::DIV:
            assert(isfinite(secondVal));
            assert(!DoubleEqual(secondVal, 0));
            return firstVal / secondVal;

        case MathExpressionsOperations::POW:
            assert(isfinite(secondVal));
            return pow(firstVal, secondVal);
        case MathExpressionsOperations::LOG:
            assert(isfinite(secondVal));
            return CalculateLog(firstVal, secondVal);
        
        case MathExpressionsOperations::SIN:
            return sin(firstVal);
        case MathExpressionsOperations::COS:
            return cos(firstVal);
        case MathExpressionsOperations::TAN:
            return tan(firstVal);
        case MathExpressionsOperations::COT:
            return CalculateCot(firstVal);

        case MathExpressionsOperations::ARCSIN:
            return asin(firstVal);
        case MathExpressionsOperations::ARCCOS:
            return acos(firstVal);
        case MathExpressionsOperations::ARCTAN:
            return atan(firstVal);
        case MathExpressionsOperations::ARCCOT:
            return PI / 2 - atan(firstVal);
        
        default:
            return NAN;
    }

    return NAN;
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

static inline MathExpressionTokenValue MathExpressionCreateValue(
                                                        MathExpressionsOperations operation)
{
    MathExpressionTokenValue value =
    {
        .operation = operation,
    };

    return value;
}

#define D(TOKEN) MathExpressionDifferentiate(TOKEN)
#define C(TOKEN) MathExpressionCopy(TOKEN)

#define TOKEN(OPERATION_NAME, LEFT_TOKEN, RIGHT_TOKEN)                                             \
    MathExpressionTokenCtor(MathExpressionCreateValue(MathExpressionsOperations::OPERATION_NAME),  \
                            MathExpressionTokenValueTypeof::OPERATION,                             \
                            LEFT_TOKEN, RIGHT_TOKEN)                                               

static inline MathExpressionTokenType* MathExpressionDifferentiateAdd(
                                                            const MathExpressionTokenType* token)
{
    assert(token);
    assert(token->valueType == MathExpressionTokenValueTypeof::OPERATION);
    assert(token->value.operation == MathExpressionsOperations::ADD);

    return TOKEN(ADD, D(token->left), D(token->right));
}

static inline MathExpressionTokenType* MathExpressionDifferentiateSub(
                                                            const MathExpressionTokenType* token)
{
    assert(token);
    assert(token->valueType == MathExpressionTokenValueTypeof::OPERATION);
    assert(token->value.operation == MathExpressionsOperations::SUB);

    return TOKEN(SUB, D(token->left), D(token->right));
}

static inline MathExpressionTokenType* MathExpressionDifferentiateMul(
                                                            const MathExpressionTokenType* token)
{
    assert(token);
    assert(token->valueType == MathExpressionTokenValueTypeof::OPERATION);
    assert(token->value.operation == MathExpressionsOperations::MUL);


    return TOKEN(ADD, TOKEN(MUL, D(token->left), C(token->right)), 
                      TOKEN(MUL, C(token->left), D(token->right)));
}

static inline MathExpressionTokenType* MathExpressionDifferentiateDiv(
                                                            const MathExpressionTokenType* token)
{
    assert(token);
    assert(token->valueType == MathExpressionTokenValueTypeof::OPERATION);
    assert(token->value.operation == MathExpressionsOperations::DIV);

    return TOKEN(DIV, TOKEN(SUB, TOKEN(MUL, D(token->left), C(token->right)), 
                                 TOKEN(MUL, C(token->left), D(token->right))),
                      TOKEN(MUL, C(token->right), C(token->right)));
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
    
    switch(token->value.operation)
    {
        case MathExpressionsOperations::ADD:
            return MathExpressionDifferentiateAdd(token);
        case MathExpressionsOperations::SUB:
            return MathExpressionDifferentiateSub(token);
        case MathExpressionsOperations::MUL:
            return MathExpressionDifferentiateMul(token);
        case MathExpressionsOperations::DIV:
            return MathExpressionDifferentiateDiv(token);
    }

    return nullptr;
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
