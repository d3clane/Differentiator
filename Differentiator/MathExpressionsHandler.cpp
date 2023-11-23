#include <assert.h>
#include <ctype.h>
#include <string.h>
#include <math.h>

#include "MathExpressionsHandler.h"
#include "../Common/StringFuncs.h"
#include "../Common/Log.h"
#include "../FastInput/InputOutput.h"
#include "../Common/DoubleFuncs.h"

const int POISON = 0xDEAD;

static void MathExpressionDtor    (MathExpressionTokenType* node);
static void MathExpressionNodeDtor(MathExpressionTokenType* node);

static MathExpressionTokenType* MathExpressionTokenCtor(MathExpressionTokenValue value, 
                                                        MathExpressionTokenValueTypeof valueType);

static void MathExpressionNodeSetEdges(MathExpressionTokenType* node, 
                                       MathExpressionTokenType* left, 
                                       MathExpressionTokenType* right);
 
static MathExpressionErrors MathExpressionPrintPrefixFormat     (
                                                const MathExpressionTokenType* node, 
                                                const MathExpressionVariablesArrayType* varsArr, 
                                                FILE* outStream);

static MathExpressionErrors MathExpressionPrintEquationFormat   (
                                                const MathExpressionTokenType* node, 
                                                const MathExpressionVariablesArrayType* varsArr,
                                                FILE* outStream);

static MathExpressionErrors MathExpressionPrintEquationFormatTex(
                                                const MathExpressionTokenType* node, 
                                                const MathExpressionVariablesArrayType* varsArr, 
                                                FILE* outStream);

static void       MathExpressionNodePrintValue                  (
                                                const MathExpressionTokenType* node, 
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

static const char* MathExpressionReadNodeValue(MathExpressionTokenValue* value, 
                                               MathExpressionTokenValueTypeof* valueType, 
                                               MathExpressionVariablesArrayType* varsArr,
                                               const char* stringPtr);

static bool HaveToPutBrackets(const MathExpressionTokenType* parent, 
                              const MathExpressionTokenType* son);

static int         GetOperation(const char* operation);
static const char* GetOperationLongName (const MathExpressionsOperations operation);
static const char* GetOperationShortName(const MathExpressionsOperations operation);

static void MathExpressionGraphicDump(const MathExpressionTokenType* node, FILE* outDotFile);
static void DotFileCreateNodes(const MathExpressionTokenType* node, 
                               const MathExpressionVariablesArrayType* varsArr, FILE* outDotFile);

static inline void CreateImgInLogFile(const size_t imgIndex, bool openImg);
static inline void DotFileBegin(FILE* outDotFile);
static inline void DotFileEnd(FILE* outDotFile);

static double MathExpressionCalculate(const MathExpressionTokenType* node, 
                                      const MathExpressionVariablesArrayType* varsArr);
static double MathExpressionCalculateUsingNodeOperation(const MathExpressionsOperations operation,
                                                        double firstVal, double secondVal);

static inline int AddVariable(MathExpressionVariablesArrayType* varsArr,  
                              const char*  variableName, 
                              const double variableValue = 0);

static int GetVariableIdByName(const MathExpressionVariablesArrayType* varsArr, 
                               const char* variableName);

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

static void MathExpressionDtor(MathExpressionTokenType* node)
{
    if (node == nullptr)
        return;
    
    assert(node);

    MathExpressionDtor(node->left);
    MathExpressionDtor(node->right);

    MathExpressionNodeDtor(node);
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
                                                    const MathExpressionTokenType* node, 
                                                    const MathExpressionVariablesArrayType* varsArr, 
                                                    FILE* outStream)
{
    if (node == nullptr)
    {
        PRINT(outStream, "nil ");
        return MathExpressionErrors::NO_ERR;
    }

    PRINT(outStream, "(");
    
    if (node->valueType == MathExpressionTokenValueTypeof::VALUE)
        PRINT(outStream, "%lf ", node->value.value);
    else if (node->valueType == MathExpressionTokenValueTypeof::VARIABLE)
        PRINT(outStream, "%s ", varsArr->data[node->value.varId].variableName);
    else
        PRINT(outStream, "%s ", GetOperationLongName(node->value.operation));

    MathExpressionErrors err = MathExpressionErrors::NO_ERR;

    err = MathExpressionPrintPrefixFormat(node->left,  varsArr, outStream);
    err = MathExpressionPrintPrefixFormat(node->right, varsArr, outStream);

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
                                          const MathExpressionTokenType* node, 
                                          const MathExpressionVariablesArrayType* varsArr,
                                          FILE* outStream)
{
    if (node->left == nullptr && node->right == nullptr)
    {
        MathExpressionNodePrintValue(node, varsArr, outStream);

        return MathExpressionErrors::NO_ERR;
    }

    MathExpressionErrors err = MathExpressionErrors::NO_ERR;

    bool haveToPutLeftBrackets = (node->left->valueType == MathExpressionTokenValueTypeof::OPERATION) &&
                                  HaveToPutBrackets(node, node->left);
    
    if (haveToPutLeftBrackets) PRINT(outStream, "(");
    err = MathExpressionPrintEquationFormat(node->left, varsArr, outStream);
    if (haveToPutLeftBrackets) PRINT(outStream, ")");

    MathExpressionNodePrintValue(node, varsArr, outStream);
    
    bool haveToPutRightBrackets = (node->right->valueType == MathExpressionTokenValueTypeof::OPERATION) &&
                                   HaveToPutBrackets(node, node->right);
    if (haveToPutRightBrackets) PRINT(outStream, "(");
    err = MathExpressionPrintEquationFormat(node->right, varsArr, outStream);
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

static void MathExpressionNodePrintValue(const MathExpressionTokenType* node, 
                                         const MathExpressionVariablesArrayType* varsArr, 
                                         FILE* outStream)
{
    if (node->valueType == MathExpressionTokenValueTypeof::VALUE)
        PRINT(outStream, "%lf ", node->value.value);
    else if (node->valueType == MathExpressionTokenValueTypeof::VARIABLE)
        PRINT(outStream, "%s ", varsArr->data[node->value.varId].variableName);
    else
        PRINT(outStream, "%s ", GetOperationShortName(node->value.operation));
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
                                             const MathExpressionTokenType* node, 
                                             const MathExpressionVariablesArrayType* varsArr, 
                                             FILE* outStream)
{
    assert(node);
    assert(outStream);

    if (node->left == nullptr && node->right == nullptr)
    {
        MathExpressionNodePrintValue(node, varsArr, outStream);

        return MathExpressionErrors::NO_ERR;
    }

    MathExpressionErrors err = MathExpressionErrors::NO_ERR;

    bool isDivideOperation     = (node->valueType       == MathExpressionTokenValueTypeof::OPERATION) &&
                                 (node->value.operation == MathExpressionsOperations::DIV);
    bool haveToPutLeftBrackets = (node->left->valueType == MathExpressionTokenValueTypeof::OPERATION) &&
                                 (HaveToPutBrackets(node, node->left));

    if (isDivideOperation)                           fprintf(outStream, "\\frac{");
    if (!isDivideOperation && haveToPutLeftBrackets) fprintf(outStream, "(");
    err = MathExpressionPrintEquationFormatTex(node->left, varsArr, outStream);
    if (!isDivideOperation && haveToPutLeftBrackets) fprintf(outStream, ")");
    if (isDivideOperation)                           fprintf(outStream, "}");

    if (!isDivideOperation) MathExpressionNodePrintValue(node, varsArr, outStream);
    
    bool haveToPutRightBrackets = (node->right->valueType == MathExpressionTokenValueTypeof::OPERATION) &&
                                   (HaveToPutBrackets(node, node->right));
    
    if (isDivideOperation)                            fprintf(outStream, "{");
    if (!isDivideOperation && haveToPutRightBrackets) fprintf(outStream, "(");
    err = MathExpressionPrintEquationFormatTex(node->right, varsArr, outStream);
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

    stringPtr = MathExpressionReadNodeValue(&value, &valueType, varsArr, stringPtr);
    MathExpressionTokenType* node = MathExpressionTokenCtor(value, valueType);

    MathExpressionTokenType* left  = MathExpressionReadPrefixFormat(stringPtr, &stringPtr, varsArr);
    MathExpressionTokenType* right = MathExpressionReadPrefixFormat(stringPtr, &stringPtr, varsArr);

    stringPtr = SkipSymbolsUntilStopChar(stringPtr, ')');
    ++stringPtr;

    MathExpressionNodeSetEdges(node, left, right);

    *stringEndPtr = stringPtr;
    return node;
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

        stringPtr = MathExpressionReadNodeValue(&value, &valueType, varsArr, stringPtr);
        MathExpressionTokenType* node = MathExpressionTokenCtor(value, valueType);

        *stringEndPtr = stringPtr;
        return node;
    }

    MathExpressionTokenType* left  = MathExpressionReadInfixFormat(stringPtr, &stringPtr, varsArr);

    stringPtr = MathExpressionReadNodeValue(&value, &valueType, varsArr, stringPtr);
    MathExpressionTokenType* node = MathExpressionTokenCtor(value, valueType);
    MathExpressionTokenType* right = MathExpressionReadInfixFormat(stringPtr, &stringPtr, varsArr);

    MathExpressionNodeSetEdges(node, left, right);

    stringPtr = SkipSymbolsWhileChar(stringPtr, ')');

    *stringEndPtr = stringPtr;
    return node;
}

static const char* MathExpressionReadNodeValue(MathExpressionTokenValue* value, 
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

    if (strcasecmp(string, "/") == 0      || strcasecmp(string, "div") == 0)
        return (int)MathExpressionsOperations::DIV;
    else if (strcasecmp(string, "*") == 0 || strcasecmp(string, "mul") == 0)
        return (int)MathExpressionsOperations::MUL;
    else if (strcasecmp(string, "-") == 0 || strcasecmp(string, "sub") == 0)
        return (int)MathExpressionsOperations::SUB;
    else if (strcasecmp(string, "+") == 0 || strcasecmp(string, "add") == 0)
        return (int)MathExpressionsOperations::ADD;
    
    return -1;
}

static const char* GetOperationLongName(const MathExpressionsOperations operation)
{
    switch (operation)
    {
        case MathExpressionsOperations::MUL:
            return "mul";
        case MathExpressionsOperations::DIV:
            return "div";
        case MathExpressionsOperations::SUB:
            return "sub";
        case MathExpressionsOperations::ADD:
            return "add";
        
        default:
            return nullptr;
    }

    return nullptr;;
}

static const char* GetOperationShortName(const MathExpressionsOperations operation)
{
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
            return nullptr;
    }

    return nullptr;
}

static MathExpressionTokenType* MathExpressionTokenCtor(MathExpressionTokenValue value, 
                                                        MathExpressionTokenValueTypeof valueType)
{   
    MathExpressionTokenType* node = (MathExpressionTokenType*)calloc(1, sizeof(*node));
    node->left      = nullptr;
    node->right     = nullptr;
    node->value     = value;
    node->valueType = valueType;

    return node;
}

static void MathExpressionNodeDtor(MathExpressionTokenType* node)
{
    node->left        = nullptr;
    node->right       = nullptr;
    node->value.varId =  POISON;

    free(node);
}

static inline void CreateImgInLogFile(const size_t imgIndex, bool openImg)
{
    static const size_t maxImgNameLength  = 64;
    static char imgName[maxImgNameLength] = "";
    snprintf(imgName, maxImgNameLength, "../imgs/img_%zu_time_%s.png", imgIndex, __TIME__);

    static const size_t     maxCommandLength  = 128;
    static char commandName[maxCommandLength] =  "";
    snprintf(commandName, maxCommandLength, "dot MathExpressionerentiator.dot -T png -o %s", imgName);
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

    static const char* dotFileName = "MathExpressionerentiator.dot";
    FILE* outDotFile = fopen(dotFileName, "w");

    if (outDotFile == nullptr)
        return;

    DotFileBegin(outDotFile);

    DotFileCreateNodes(expression->root, &expression->variables, outDotFile);

    MathExpressionGraphicDump(expression->root, outDotFile);

    DotFileEnd(outDotFile);

    fclose(outDotFile);

    static size_t imgIndex = 0;
    CreateImgInLogFile(imgIndex, openImg);
    imgIndex++;
}

static void DotFileCreateNodes(const MathExpressionTokenType* node,    
                               const MathExpressionVariablesArrayType* varsArr, FILE* outDotFile)
{
    if (node == nullptr)
        return;
    
    fprintf(outDotFile, "node%p"
                        "[shape=Mrecord, style=filled, ", node);
    
    if (node->valueType == MathExpressionTokenValueTypeof::OPERATION)
        fprintf(outDotFile, "fillcolor=\"#89AC76\", label = \"%s\", ", 
                            GetOperationLongName(node->value.operation));
    else if (node->valueType == MathExpressionTokenValueTypeof::VALUE)
        fprintf(outDotFile, "fillcolor=\"#7293ba\", label = \"%lf\", ", node->value.value);
    else if (node->valueType == MathExpressionTokenValueTypeof::VARIABLE)
        fprintf(outDotFile, "fillcolor=\"#78DBE2\", label = \"%s\", ", 
                            varsArr->data[node->value.varId].variableName);
    else 
        fprintf(outDotFile, "fillcolor=\"#FF0000\", label = \"ERROR\", ");

    fprintf(outDotFile, "color = \"#D0D000\"];\n");

    DotFileCreateNodes(node->left,  varsArr, outDotFile);
    DotFileCreateNodes(node->right, varsArr, outDotFile);
}

static void MathExpressionGraphicDump(const MathExpressionTokenType* node, FILE* outDotFile)
{
    if (node == nullptr)
    {
        fprintf(outDotFile, "\n");
        return;
    }
    
    fprintf(outDotFile, "node%p;\n", node);

    if (node->left != nullptr) fprintf(outDotFile, "node%p->", node);
    MathExpressionGraphicDump(node->left, outDotFile);

    if (node->right != nullptr) fprintf(outDotFile, "node%p->", node);
    MathExpressionGraphicDump(node->right, outDotFile);
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

static double MathExpressionCalculate(const MathExpressionTokenType* node, 
                                      const MathExpressionVariablesArrayType* varsArr)
{
    assert(node);

    if (node->valueType == MathExpressionTokenValueTypeof::VALUE)
        return node->value.value;

    if (node->valueType == MathExpressionTokenValueTypeof::VARIABLE)
        return varsArr->data[node->value.varId].variableValue;

    double firstVal  = MathExpressionCalculate(node->left,  varsArr);
    double secondVal = MathExpressionCalculate(node->right, varsArr);
    
    return MathExpressionCalculateUsingNodeOperation(node->value.operation, firstVal, secondVal);
}

static double MathExpressionCalculateUsingNodeOperation(const MathExpressionsOperations operation, 
                                              double firstVal, double secondVal)
{
    switch(operation)
    {
        case MathExpressionsOperations::ADD:
            return firstVal + secondVal;
        case MathExpressionsOperations::SUB:
            return firstVal - secondVal;
        case MathExpressionsOperations::MUL:
            return firstVal * secondVal;
        case MathExpressionsOperations::DIV:
        {
            assert(!DoubleEqual(secondVal, 0));
            return firstVal / secondVal;
        }
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

static void MathExpressionNodeSetEdges(MathExpressionTokenType* node, MathExpressionTokenType* left, 
                                                     MathExpressionTokenType* right)
{
    assert(node);

    node->left  = left;
    node->right = right;
}
