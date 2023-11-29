#include <assert.h>

#include "MathExpressionTexDump.h"

static bool        ExpressionOperationIsPrefix          (const ExpressionOperationId operation);
static bool        ExpressionOperationNeedTexRightBraces(const ExpressionOperationId operation);
static bool        ExpressionOperationNeedTexLeftBraces (const ExpressionOperationId operation);
static const char* ExpressionOperationGetTexName        (const ExpressionOperationId operation);

static bool HaveToPutBrackets(const ExpressionTokenType* parent, 
                              const ExpressionTokenType* son);
                              
static void ExpressionTokenPrintValue   (const ExpressionTokenType* token, 
                                         FILE* outStream);
ExpressionErrors ExpressionPrintTex     (const ExpressionType* expression, 
                                         FILE* outStream,
                                         const char* string)
{
    assert(expression);
    assert(outStream);

    return ExpressionTokenPrintTexWithTrollString(expression->root, outStream, string);
}

ExpressionErrors ExpressionTokenPrintTexWithTrollString(const ExpressionTokenType* rootToken,
                                                    FILE* outStream,
                                                    const char* string)
{
    assert(rootToken);
    assert(outStream);

    static const size_t            numberOfRoflStrings  = 8;
    static const char* roflStrings[numberOfRoflStrings] = 
    {
        "Очевидно, что это преобразование верно",
        "Несложно заметить это преобразование", 
        "Любопытный читатель может показать этот переход самостоятельно, ",
        "Не буду утруждать себя доказательством, что",
        "Я нашел удивительное решение, но здесь маловато места, чтобы его поместить, ",
        "Оставим переход без комментариев, ",
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
    
    fprintf(outStream, "\\begin{gather}\n\\end{gather}\n\\begin{math}\n");

    ExpressionErrors err = ExpressionTokenPrintTex(rootToken, outStream);

    fprintf(outStream, "\\\\\n\\end{math}\n");

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

    bool isPrefixOperation    = ExpressionOperationIsPrefix(token->value.operation);

    if (isPrefixOperation) fprintf(outStream, "%s ", 
                                    ExpressionOperationGetTexName(token->value.operation));

    bool needLeftBrackets  = HaveToPutBrackets(token, token->left);
    bool needTexLeftBraces = ExpressionOperationNeedTexLeftBraces(token->value.operation);

    if (needTexLeftBraces)                      fprintf(outStream, "{");
    if (!needTexLeftBraces && needLeftBrackets) fprintf(outStream, "(");

    err = ExpressionTokenPrintTex(token->left, outStream);

    if (!needTexLeftBraces && needLeftBrackets) fprintf(outStream, ")");
    if (needTexLeftBraces)                      fprintf(outStream, "}");

    if (!isPrefixOperation) fprintf(outStream, "%s ", 
                                    ExpressionOperationGetTexName(token->value.operation));

    if (ExpressionOperationIsUnary(token->value.operation))
        return err;

    bool needTexRightBraces   = ExpressionOperationNeedTexRightBraces(token->value.operation);
    bool needRightBrackets    = HaveToPutBrackets(token, token->right);
    
    if (needTexRightBraces)                       fprintf(outStream, "{");
    if (!needTexRightBraces && needRightBrackets) fprintf(outStream, "(");
    err = ExpressionTokenPrintTex(token->right, outStream);
    if (!needTexRightBraces && needRightBrackets) fprintf(outStream, ")");
    if (needTexRightBraces)                       fprintf(outStream, "}");

    return err;   
}


//---------------------------------------------------------------------------------------

static const char* ExpressionOperationGetTexName(const ExpressionOperationId operation)
{
    #define GENERATE_OPERATION_CMD(NAME, v1, v2, v3, v4, TEX_NAME, ...) \
        case ExpressionOperationId::NAME:                            \
            return TEX_NAME;

    switch(operation)
    {
        #include "Operations.h"

        default:
            break;
    }

    #undef GENERATE_OPERATION_CMD

    return nullptr;
}

static bool ExpressionOperationNeedTexRightBraces(const ExpressionOperationId operation)
{
    #define GENERATE_OPERATION_CMD(NAME, v1, v2, v3, v4, v5, v6, NEED_RIGHT_BRACES, ...)    \
        case ExpressionOperationId::NAME:                                                   \
            return NEED_RIGHT_BRACES;
    
    switch(operation)
    {
        #include "Operations.h"

        default:
            break;
    }

    #undef GENERATE_OPERATION_CMD

    return false;
}

static bool ExpressionOperationNeedTexLeftBraces(const ExpressionOperationId operation)
{
    #define GENERATE_OPERATION_CMD(NAME, v1, v2, v3, v4, v5, NEED_LEFT_BRACES, ...) \
        case ExpressionOperationId::NAME:                                           \
            return NEED_LEFT_BRACES;

    switch(operation)
    {
        #include "Operations.h"

        default:
            break;
    }

    #undef GENERATE_OPERATION_CMD

    return false;
}

static void ExpressionTokenPrintValue(const ExpressionTokenType* token, 
                                      FILE* outStream)
{
    assert(token->valueType != ExpressionTokenValueTypeof::OPERATION);

    switch (token->valueType)
    {
        case ExpressionTokenValueTypeof::VALUE:
            fprintf(outStream, "%.2lf ", token->value.value);
            break;
        
        case ExpressionTokenValueTypeof::VARIABLE:
            fprintf(outStream, "%s ", token->value.varPtr->variableName);
            break;
        
        case ExpressionTokenValueTypeof::OPERATION:
            fprintf(outStream, "%s ", ExpressionOperationGetTexName(token->value.operation));
            break;
        
        default:
            break;
    }
}

static bool HaveToPutBrackets(const ExpressionTokenType* parent, 
                              const ExpressionTokenType* son)
{
    assert(parent);

    assert(parent->valueType == ExpressionTokenValueTypeof::OPERATION);

    if (son->valueType != ExpressionTokenValueTypeof::OPERATION)
        return false;

    ExpressionOperationId parentOperation = parent->value.operation;
    ExpressionOperationId sonOperation    = son->value.operation;

    if (sonOperation == ExpressionOperationId::POW && sonOperation == parentOperation)
        return true;

    if (ExpressionOperationIsPrefix(sonOperation))
        return false;

    if (sonOperation == ExpressionOperationId::POW)
        return false;

    if ((sonOperation    == ExpressionOperationId::MUL  || 
         sonOperation    == ExpressionOperationId::DIV) &&
        (parentOperation == ExpressionOperationId::SUB  || 
         parentOperation == ExpressionOperationId::ADD))
        return false;

    if (sonOperation    == ExpressionOperationId::ADD && 
        parentOperation == ExpressionOperationId::ADD)
        return false;
    
    if (sonOperation    == ExpressionOperationId::MUL && 
        parentOperation == ExpressionOperationId::MUL)
        return false;

    return true;
}

static bool ExpressionOperationIsPrefix(const ExpressionOperationId operation)
{
    #define GENERATE_OPERATION_CMD(NAME, FORMAT, TEX_FORMAT, ...)                               \
        case ExpressionOperationId::NAME:                                                       \
            return ExpressionOperationFormat::TEX_FORMAT == ExpressionOperationFormat::PREFIX;

    switch (operation)
    {
        #include "Operations.h"

        default:
            break;
    }

    #undef  GENERATE_OPERATION_CMD

    return false;
}
