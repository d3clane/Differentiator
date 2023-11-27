#include <assert.h>
#include <string.h>
#include <ctype.h>

#include "MathExpressionInOut.h"
#include "Common/Log.h"
#include "FastInput/InputOutput.h"
#include "Common/StringFuncs.h"

static ExpressionErrors ExpressionPrintPrefixFormat     (
                                                const ExpressionTokenType* token, 
                                                FILE* outStream);

static ExpressionErrors ExpressionPrintEquationFormat   (
                                                const ExpressionTokenType* token, 
                                                FILE* outStream);

static void ExpressionTokenPrintValue                       (
                                                const ExpressionTokenType* token, 
                                                FILE* outStream);

static ExpressionTokenType* ExpressionReadPrefixFormat(
                                                const char* const string, 
                                                const char** stringEndPtr,
                                                ExpressionVariablesArrayType* varsArr);

static ExpressionTokenType* ExpressionReadInfixFormat(
                                                const char* const string, 
                                                const char** stringEndPtr,
                                                ExpressionVariablesArrayType* varsArr);

static const char* ExpressionReadTokenValue(ExpressionTokenValue* value, 
                                               ExpressionTokenValueTypeof* valueType, 
                                               ExpressionVariablesArrayType* varsArr,
                                               const char* stringPtr);

static bool HaveToPutBrackets(const ExpressionTokenType* parent, 
                              const ExpressionTokenType* son,
                              bool inTex = false);

static ExpressionVariableType* AddVariable(ExpressionVariablesArrayType* varsArr, 
                                           const char*  variableName, 
                                           const double variableValue = 0);

static ExpressionVariableType* GetVariablePtrByName(const ExpressionVariablesArrayType* varsArr, 
                                                    const char* variableName);

#define PRINT(outStream, ...)                          \
do                                                     \
{                                                      \
    if (outStream) fprintf(outStream, __VA_ARGS__);    \
    Log(__VA_ARGS__);                                  \
} while (0)

ExpressionErrors ExpressionPrintPrefixFormat(const ExpressionType* expression, 
                                                     FILE* outStream)
{
    assert(expression);
    assert(outStream);

    LOG_BEGIN();

    ExpressionErrors err = ExpressionPrintPrefixFormat(expression->root, outStream);

    PRINT(outStream, "\n");

    LOG_END();

    return err;
}

//---------------------------------------------------------------------------------------

static ExpressionErrors ExpressionPrintPrefixFormat(
                                                    const ExpressionTokenType* token, 
                                                    FILE* outStream)
{
    if (token == nullptr)
    {
        PRINT(outStream, "nil ");
        return ExpressionErrors::NO_ERR;
    }

    PRINT(outStream, "(");
    
    if (token->valueType == ExpressionTokenValueTypeof::VALUE)
        PRINT(outStream, "%.2lf ", token->value.value);
    else if (token->valueType == ExpressionTokenValueTypeof::VARIABLE)
        PRINT(outStream, "%s ", token->value.varPtr->variableName);
    else
        PRINT(outStream, "%s ", token->value.operation.longName);

    ExpressionErrors err = ExpressionErrors::NO_ERR;

    err = ExpressionPrintPrefixFormat(token->left, outStream);
    
    err = ExpressionPrintPrefixFormat(token->right, outStream);

    PRINT(outStream, ")");
    
    return err;
}

//---------------------------------------------------------------------------------------

ExpressionErrors ExpressionPrintEquationFormat(const ExpressionType* expression, 
                                                       FILE* outStream)
{
    assert(expression);
    assert(outStream);

    LOG_BEGIN();

    ExpressionErrors err = ExpressionPrintEquationFormat(expression->root, outStream);
    PRINT(outStream, "\n");

    LOG_END();

    return err; 
}

//---------------------------------------------------------------------------------------

static ExpressionErrors ExpressionPrintEquationFormat(
                                          const ExpressionTokenType* token, 
                                          FILE* outStream)
{
    if (token->left == nullptr && token->right == nullptr)
    {
        ExpressionTokenPrintValue(token, outStream);

        return ExpressionErrors::NO_ERR;
    }

    assert(token->valueType == ExpressionTokenValueTypeof::OPERATION);

    bool isPrefixOperation = ExpressionOperationIsPrefix(&token->value.operation);
    if (isPrefixOperation) fprintf(outStream, "%s ", token->value.operation.shortName);

    bool needLeftBrackets = HaveToPutBrackets(token, token->left);
    if (needLeftBrackets) PRINT(outStream, "(");

    ExpressionErrors err = ExpressionErrors::NO_ERR;
    err = ExpressionPrintEquationFormat(token->left, outStream);

    if (needLeftBrackets) PRINT(outStream, ")");

    if (!isPrefixOperation) fprintf(outStream, "%s ", token->value.operation.shortName);

    if (ExpressionOperationIsUnary(&token->value.operation))  
        return err;

    bool needRightBrackets = HaveToPutBrackets(token, token->right);
    if (needRightBrackets) PRINT(outStream, "(");

    err = ExpressionPrintEquationFormat(token->right, outStream);

    if (needRightBrackets) PRINT(outStream, ")");

    return err;
}

//---------------------------------------------------------------------------------------

static bool HaveToPutBrackets(const ExpressionTokenType* parent, 
                              const ExpressionTokenType* son,
                              bool inTex)
{
    assert(parent);

    assert(parent->valueType == ExpressionTokenValueTypeof::OPERATION);

    if (son->valueType != ExpressionTokenValueTypeof::OPERATION)
        return false;

    ExpressionOperationsIds parentOperation = parent->value.operation.operationId;
    ExpressionOperationsIds sonOperation    = son->value.operation.operationId;

    if (ExpressionOperationIsPrefix(&son->value.operation, inTex))
        return false;

    if (sonOperation == ExpressionOperationsIds::POW)
        return false;

    if ((sonOperation    == ExpressionOperationsIds::MUL  || 
         sonOperation    == ExpressionOperationsIds::DIV) &&
        (parentOperation == ExpressionOperationsIds::SUB  || 
         parentOperation == ExpressionOperationsIds::ADD))
        return false;

    if (sonOperation    == ExpressionOperationsIds::ADD && 
        parentOperation == ExpressionOperationsIds::ADD)
        return false;
    
    if (sonOperation    == ExpressionOperationsIds::MUL && 
        parentOperation == ExpressionOperationsIds::MUL)
        return false;

    return true;
}

//---------------------------------------------------------------------------------------

//---------------------------------------------------------------------------------------

ExpressionErrors ExpressionReadPrefixFormat(ExpressionType* expression, FILE* inStream)
{
    assert(expression);
    assert(inStream);

    char* inputExpression = ReadText(inStream);

    if (inputExpression == nullptr)
        return ExpressionErrors::MEM_ERR;

    const char* inputExpressionEndPtr = inputExpression;

    expression->root = ExpressionReadPrefixFormat(inputExpression, 
                                                      &inputExpressionEndPtr, 
                                                      &expression->variables);

    free(inputExpression);

    return ExpressionErrors::NO_ERR;
}

//---------------------------------------------------------------------------------------

static ExpressionTokenType* ExpressionReadPrefixFormat(
                                                        const char* const string, 
                                                        const char** stringEndPtr,
                                                        ExpressionVariablesArrayType* varsArr)
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

    ExpressionTokenValue value;
    ExpressionTokenValueTypeof valueType;

    stringPtr = ExpressionReadTokenValue(&value, &valueType, varsArr, stringPtr);
    ExpressionTokenType* token = ExpressionTokenCtor(value, valueType);
    
    ExpressionTokenType* left  = ExpressionReadPrefixFormat(stringPtr, &stringPtr, varsArr);

    ExpressionTokenType* right = nullptr;
    if (!ExpressionOperationIsUnary(&token->value.operation))
        right = ExpressionReadPrefixFormat(stringPtr, &stringPtr, varsArr);

    stringPtr = SkipSymbolsUntilStopChar(stringPtr, ')');
    ++stringPtr;

    ExpressionTokenSetEdges(token, left, right);

    *stringEndPtr = stringPtr;
    return token;
}

//---------------------------------------------------------------------------------------

ExpressionErrors ExpressionReadInfixFormat (ExpressionType* expression, FILE* inStream)
{
    assert(expression);
    assert(inStream);

    char* inputExpression = ReadText(inStream);

    if (inputExpression == nullptr)
        return ExpressionErrors::MEM_ERR;
    
    const char* inputExpressionEndPtr = inputExpression;

    expression->root = ExpressionReadInfixFormat(inputExpression, 
                                                     &inputExpressionEndPtr, 
                                                     &expression->variables);

    free(inputExpression);

    return ExpressionErrors::NO_ERR;
}

//---------------------------------------------------------------------------------------

static ExpressionTokenType* ExpressionReadInfixFormat(
                                            const char* const string, 
                                            const char** stringEndPtr,
                                            ExpressionVariablesArrayType* varsArr)
{
    assert(string);

    const char* stringPtr = string;

    stringPtr = SkipSymbolsWhileStatement(stringPtr, isspace);

    ExpressionTokenValue     value;
    ExpressionTokenValueTypeof valueType;

    int symbol = *stringPtr;
    stringPtr++;

    if (symbol != '(')
    {
        --stringPtr;

        stringPtr = ExpressionReadTokenValue(&value, &valueType, varsArr, stringPtr);
        ExpressionTokenType* token = ExpressionTokenCtor(value, valueType);

        *stringEndPtr = stringPtr;
        return token;
    }

    ExpressionTokenType* left  = ExpressionReadInfixFormat(stringPtr, &stringPtr, varsArr);

    stringPtr = ExpressionReadTokenValue(&value, &valueType, varsArr, stringPtr);
    ExpressionTokenType* token = ExpressionTokenCtor(value, valueType);
    ExpressionTokenType* right = ExpressionReadInfixFormat(stringPtr, &stringPtr, varsArr);

    ExpressionTokenSetEdges(token, left, right);

    stringPtr = SkipSymbolsWhileChar(stringPtr, ')');

    *stringEndPtr = stringPtr;
    return token;
}

//---------------------------------------------------------------------------------------

static const char* ExpressionReadTokenValue(ExpressionTokenValue* value, 
                                               ExpressionTokenValueTypeof* valueType, 
                                               ExpressionVariablesArrayType* varsArr,
                                               const char* string)
{
    assert(value);
    assert(string);
    assert(valueType);

    double readenValue = NAN;
    int shift = 0;
    int scanResult = sscanf(string, "%lf%n\n", &readenValue, &shift);

    if (scanResult != 0)
    {
        value->value = readenValue;
        *valueType   = ExpressionTokenValueTypeof::VALUE;
        return string + shift;
    }

    shift = 0;

    static const size_t      maxInputStringSize  = 128;
    static char  inputString[maxInputStringSize] =  "";

    const char* stringPtr = string;
    sscanf(string, "%s%n", inputString, &shift);

    stringPtr = string + shift;
    assert(isspace(*stringPtr));

    int operationId = ExpressionOperationGetId(inputString);
    if (operationId != -1)
    {
        *value     = ExpressionCreateTokenValue((ExpressionOperationsIds) operationId);
        *valueType = ExpressionTokenValueTypeof::OPERATION;
        return stringPtr;
    }

    ExpressionVariableType* varPtr = AddVariable(varsArr, inputString);

    assert(varPtr != nullptr);

    value->varPtr = varPtr;
    *valueType   = ExpressionTokenValueTypeof::VARIABLE;

    return stringPtr;
}

//---------------------------------------------------------------------------------------

static void ExpressionTokenPrintValue(const ExpressionTokenType* token, 
                                      FILE* outStream)
{
    assert(token->valueType != ExpressionTokenValueTypeof::OPERATION);

    if (token->valueType == ExpressionTokenValueTypeof::VALUE)
        PRINT(outStream, "%.2lf ", token->value.value);
    else if (token->valueType == ExpressionTokenValueTypeof::VARIABLE)
        PRINT(outStream, "%s ", token->value.varPtr->variableName);
    else if (token->valueType == ExpressionTokenValueTypeof::OPERATION) 
        PRINT(outStream, "%s ", token->value.operation.longName);
}

#undef PRINT

//---------------------------------------------------------------------------------------

ExpressionErrors ExpressionPrintTex(const ExpressionType* expression, 
                                                          FILE* outStream,
                                                          const char* string)
{
    assert(expression);
    assert(outStream);

    return ExpressionTokenPrintTexTrollString(expression->root, outStream, string);
}

ExpressionErrors ExpressionTokenPrintTexTrollString(const ExpressionTokenType* rootToken,
                                                    FILE* outStream,
                                                    const char* string)
{
    assert(rootToken);
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
    
    fprintf(outStream, "\\begin{gather}\n\\end{gather}\n\\begin{}\n");

    ExpressionErrors err = ExpressionTokenPrintTex(rootToken, outStream);

    fprintf(outStream, "\\\\\n\\end{}\n");

    return err;
}

ExpressionErrors ExpressionTokenPrintTex(const ExpressionTokenType* token, 
                                         FILE* outStream)
{
    assert(token);
    assert(outStream);

    if (token->left == nullptr && token->right == nullptr)
    {
        ExpressionTokenPrintValue(token, outStream);

        return ExpressionErrors::NO_ERR;
    }

    ExpressionErrors err = ExpressionErrors::NO_ERR;
    assert((token->valueType == ExpressionTokenValueTypeof::OPERATION));

    bool isPrefixOperation    = ExpressionOperationIsPrefix(&token->value.operation, true);

    if (isPrefixOperation) fprintf(outStream, "%s ", token->value.operation.texName);

    bool needLeftBrackets  = HaveToPutBrackets(token, token->left);
    bool needTexLeftBraces = token->value.operation.needTexLeftBraces;

    if (needTexLeftBraces)                      fprintf(outStream, "{");
    if (!needTexLeftBraces && needLeftBrackets) fprintf(outStream, "(");

    err = ExpressionTokenPrintTex(token->left, outStream);

    if (!needTexLeftBraces && needLeftBrackets) fprintf(outStream, ")");
    if (needTexLeftBraces)                      fprintf(outStream, "}");

    if (!isPrefixOperation) fprintf(outStream, "%s ", token->value.operation.texName);

    if (ExpressionOperationIsUnary(&token->value.operation))
        return err;

    bool needTexRightBraces   = token->value.operation.needTexRightBraces;
    bool needRightBrackets    = HaveToPutBrackets(token, token->right);
    
    if (needTexRightBraces)                       fprintf(outStream, "{");
    if (!needTexRightBraces && needRightBrackets) fprintf(outStream, "(");
    err = ExpressionTokenPrintTex(token->right, outStream);
    if (!needTexRightBraces && needRightBrackets) fprintf(outStream, ")");
    if (needTexRightBraces)                       fprintf(outStream, "}");

    return err;   
}

//---------------------------------------------------------------------------------------

static ExpressionVariableType* AddVariable(ExpressionVariablesArrayType* varsArr, 
                                           const char*  variableName, 
                                           const double variableValue)
{
    assert(varsArr);
    assert(variableName);

    ExpressionVariableType* varPtr = GetVariablePtrByName(varsArr, variableName);

    if (varPtr != nullptr)
        return varPtr;
        
    assert(varsArr->size < varsArr->capacity);

    varsArr->data[varsArr->size].variableName  = strdup(variableName);

    assert(varsArr->data[varsArr->size].variableName);
    if (varsArr->data[varsArr->size].variableName == nullptr)
        return nullptr;
    
    varsArr->data[varsArr->size].variableValue = variableValue;
    varsArr->size++;

    return varsArr->data + (int)varsArr->size - 1;
}

//---------------------------------------------------------------------------------------

static ExpressionVariableType* GetVariablePtrByName(const ExpressionVariablesArrayType* varsArr, 
                                                    const char* variableName)
{
    assert(varsArr);
    assert(variableName);

    for (size_t i = 0; i < varsArr->size; ++i)
    {
        if (strcmp(varsArr->data[i].variableName, variableName) == 0)
            return varsArr->data + i;
    }
    
    return nullptr;
}
