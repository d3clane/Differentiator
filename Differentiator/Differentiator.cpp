#include <assert.h>
#include <ctype.h>
#include <string.h>
#include <math.h>

#include "Differentiator.h"
#include "../Common/StringFuncs.h"
#include "../Common/Log.h"
#include "../FastInput/InputOutput.h"

const int POISON = 0xDEAD;

static void DiffDtor    (DiffTreeNodeType* node);
static void DiffNodeDtor(DiffTreeNodeType* node);

static DiffTreeNodeType* DiffTreeNodeCtor(DiffValue value, DiffValueType valueType);
static void DiffNodeSetEdges(DiffTreeNodeType* node, DiffTreeNodeType* left, 
                                                     DiffTreeNodeType* right);
 
static DiffErrors DiffPrintPrefixFormat     (const DiffTreeNodeType* node, 
                                             const DiffVariablesArrayType* varsArr, FILE* outStream);
static DiffErrors DiffPrintEquationFormat   (const DiffTreeNodeType* node, 
                                             const DiffVariablesArrayType* varsArr, FILE* outStream);
static DiffErrors DiffPrintEquationFormatTex(const DiffTreeNodeType* node, 
                                             const DiffVariablesArrayType* varsArr, FILE* outStream);
static void       DiffNodePrintValue        (const DiffTreeNodeType* node, 
                                             const DiffVariablesArrayType* varsArr, FILE* outStream);

static DiffTreeNodeType* DiffReadPrefixFormat(const char* const string, const char** stringEndPtr,
                                              DiffVariablesArrayType* varsArr);

static const char* DiffReadNodeValuePrefixFormat(DiffValue* value, DiffValueType* valueType, 
                                                 DiffVariablesArrayType* varsArr,
                                                 const char* stringPtr);

static bool HaveToPutBrackets(const DiffTreeNodeType* parent, const DiffTreeNodeType* son);

static int  GetOperator(const char* string);
static const char* GetOperatorName(const char operation);

static        void DiffGraphicDump   (const DiffTreeNodeType* node, FILE* outDotFile);
static        void DotFileCreateNodes(const DiffTreeNodeType* node, 
                                      const DiffVariablesArrayType* varsArr, FILE* outDotFile);
static inline void CreateImgInLogFile(const size_t imgIndex, bool openImg);
static inline void DotFileBegin(FILE* outDotFile);
static inline void DotFileEnd(FILE* outDotFile);

static double DiffCalculate(const DiffTreeNodeType* node, const DiffVariablesArrayType* varsArr);
static double DiffCalculateUsingNodeOperation(const char operation, double firstVal, double secondVal);

static inline int AddVariable(DiffVariablesArrayType* varsArr,  
                              const char*  variableName, 
                              const double variableValue = 0);

static int GetVariableIdByName(const DiffVariablesArrayType* varsArr, 
                               const char* variableName);

DiffErrors DiffCtor(DiffTreeType* diff, DiffTreeNodeType* root)
{
    assert(diff);

    if (root != nullptr)
    {
        diff->root = root;
        return DiffErrors::NO_ERR;
    }

    diff->root = nullptr;

    diff->variables.capacity = 100;
    diff->variables.size     =   0;
    diff->variables.data     = (DiffVariableType*)calloc(diff->variables.capacity, 
                                                         sizeof(*(diff->variables.data)));
    
    return DiffErrors::NO_ERR;
}

DiffErrors DiffDtor(DiffTreeType* diff)
{
    assert(diff);

    DiffDtor(diff->root);
    diff->root = nullptr;

    for (size_t i = 0; i < diff->variables.capacity; ++i)
    {
        if      (diff->variables.data->variableName)
            free(diff->variables.data->variableName);

        diff->variables.data->variableName = nullptr;
        diff->variables.data->variableValue = POISON;
    }

    free(diff->variables.data);
    diff->variables.data     = nullptr;
    diff->variables.size     = 0;
    diff->variables.capacity = 0;


    return DiffErrors::NO_ERR;
}

static void DiffDtor(DiffTreeNodeType* node)
{
    if (node == nullptr)
        return;
    
    assert(node);

    DiffDtor(node->left);
    DiffDtor(node->right);

    DiffNodeDtor(node);
}

#define PRINT(outStream, ...)                          \
do                                                     \
{                                                      \
    if (outStream) fprintf(outStream, __VA_ARGS__);    \
    Log(__VA_ARGS__);                                  \
} while (0)

DiffErrors DiffPrintPrefixFormat(const DiffTreeType* diff, FILE* outStream)
{
    assert(diff);
    assert(outStream);

    LOG_BEGIN();

    DiffErrors err = DiffPrintPrefixFormat(diff->root, &diff->variables, outStream);

    PRINT(outStream, "\n");

    LOG_END();

    return err;
}

static DiffErrors DiffPrintPrefixFormat(const DiffTreeNodeType* node, 
                                        const DiffVariablesArrayType* varsArr, FILE* outStream)
{
    if (node == nullptr)
    {
        PRINT(outStream, "nil ");
        return DiffErrors::NO_ERR;
    }

    PRINT(outStream, "(");
    
    if (node->valueType == DiffValueType::VALUE)
        PRINT(outStream, "%lf ", node->value.value);
    else if (node->valueType == DiffValueType::VARIABLE)
        PRINT(outStream, "%s ", varsArr->data[node->value.varId].variableName);
    else
        PRINT(outStream, "%s ", GetOperatorName(node->value.operation));

    DiffErrors err = DiffErrors::NO_ERR;

    err = DiffPrintPrefixFormat(node->left,  varsArr, outStream);
    err = DiffPrintPrefixFormat(node->right, varsArr, outStream);

    PRINT(outStream, ")");
    
    return err;
}

DiffErrors DiffPrintEquationFormat(const DiffTreeType* diff, FILE* outStream)
{
    assert(diff);
    assert(outStream);

    LOG_BEGIN();

    DiffErrors err = DiffPrintEquationFormat(diff->root, &diff->variables, outStream);

    PRINT(outStream, "\n");

    LOG_END();

    return err; 
}

static DiffErrors DiffPrintEquationFormat(const DiffTreeNodeType* node, 
                                          const DiffVariablesArrayType* varsArr, FILE* outStream)
{
    if (node->left == nullptr /* || node->right == nullptr */)
    {
        DiffNodePrintValue(node, varsArr, outStream);

        return DiffErrors::NO_ERR;
    }

    DiffErrors err = DiffErrors::NO_ERR;

    bool haveToPutLeftBrackets = (node->left->valueType == DiffValueType::OPERATION) &&
                                  HaveToPutBrackets(node, node->left);
    
    if (haveToPutLeftBrackets) PRINT(outStream, "(");
    err = DiffPrintEquationFormat(node->left, varsArr, outStream);
    if (haveToPutLeftBrackets) PRINT(outStream, ")");

    DiffNodePrintValue(node, varsArr, outStream);
    
    bool haveToPutRightBrackets = (node->right->valueType == DiffValueType::OPERATION) &&
                                   HaveToPutBrackets(node, node->right);
    if (haveToPutRightBrackets) PRINT(outStream, "(");
    err = DiffPrintEquationFormat(node->right, varsArr, outStream);
    if (haveToPutRightBrackets) PRINT(outStream, ")");

    return err;
}

static bool HaveToPutBrackets(const DiffTreeNodeType* parent, const DiffTreeNodeType* son)
{
    assert(parent);
    assert(parent->valueType == DiffValueType::OPERATION);
    assert(son->valueType    == DiffValueType::OPERATION);

    char parentOperation = parent->value.operation;
    char sonOperation    = son->value.operation;

    if ((sonOperation    == '*' || sonOperation    == '/') &&
        (parentOperation == '-' || parentOperation == '*'))
        return false;

    if (sonOperation == '+' && parentOperation == '+')
        return false;
    
    if (sonOperation == '*' && parentOperation == '*')
        return false;

    return true;
}

static void DiffNodePrintValue(const DiffTreeNodeType* node, 
                               const DiffVariablesArrayType* varsArr, FILE* outStream)
{
    if (node->valueType == DiffValueType::VALUE)
        PRINT(outStream, "%lf ", node->value.value);
    else if (node->valueType == DiffValueType::VARIABLE)
        PRINT(outStream, "%s ", varsArr->data[node->value.varId].variableName);
    else
        PRINT(outStream, "%c ", node->value.operation);
}

#undef PRINT

DiffErrors DiffPrintEquationFormatTex(const DiffTreeType* diff, FILE* outStream,
                                                                const char* string)
{
    assert(diff);
    assert(outStream);

    static const size_t            numberOfRoflStrings  = 7;
    static const char* roflStrings[numberOfRoflStrings] = 
    {
        "Очевидно, что",
        "Несложно заметить, что", 
        "Любопытный читатель может показать, что",
        "Не буду утруждать себя доказательством, что",
        "Я нашел удивительное решение, но здесь маловато места, чтобы его поместить, ",
        "Без комментариев, ",
        "Это же не рокет саенс, поэтому легко видеть, что",
    };

    srand(time(NULL));

    if (string == nullptr)
        fprintf(outStream, "%s\n", roflStrings[rand() % numberOfRoflStrings]);
    else
        fprintf(outStream, "%s\n", string);
    
    fprintf(outStream, "$");

    DiffErrors err = DiffPrintEquationFormatTex(diff->root, &diff->variables, outStream);

    fprintf(outStream, "$\n");

    return err;
}

static DiffErrors DiffPrintEquationFormatTex(const DiffTreeNodeType* node, 
                                             const DiffVariablesArrayType* varsArr, FILE* outStream)
{
    assert(node);
    assert(outStream);

    if (node->left == nullptr /* || node->right == nullptr */)
    {
        DiffNodePrintValue(node, varsArr, outStream);

        return DiffErrors::NO_ERR;
    }

    DiffErrors err = DiffErrors::NO_ERR;

    bool isDivideOperation     = (node->valueType       == DiffValueType::OPERATION) &&
                                 (node->value.operation == '/');
    bool haveToPutLeftBrackets = (node->left->valueType == DiffValueType::OPERATION) &&
                                 (HaveToPutBrackets(node, node->left));

    if (isDivideOperation)                           fprintf(outStream, "\\frac{");
    if (!isDivideOperation && haveToPutLeftBrackets) fprintf(outStream, "(");
    err = DiffPrintEquationFormatTex(node->left, varsArr, outStream);
    if (!isDivideOperation && haveToPutLeftBrackets) fprintf(outStream, ")");
    if (isDivideOperation)                           fprintf(outStream, "}");

    if (!isDivideOperation) DiffNodePrintValue(node, varsArr, outStream);
    
    bool haveToPutRightBrackets = (node->right->valueType == DiffValueType::OPERATION) &&
                                   (HaveToPutBrackets(node, node->right));
    
    if (isDivideOperation)                            fprintf(outStream, "{");
    if (!isDivideOperation && haveToPutRightBrackets) fprintf(outStream, "(");
    err = DiffPrintEquationFormatTex(node->right, varsArr, outStream);
    if (!isDivideOperation && haveToPutRightBrackets) fprintf(outStream, ")");
    if (isDivideOperation)                            fprintf(outStream, "}");

    return err;   
}

DiffErrors DiffReadPrefixFormat(DiffTreeType* diff, FILE* inStream)
{
    assert(diff);
    assert(inStream);

    char* inputTree = ReadText(inStream);

    if (inputTree == nullptr)
        return DiffErrors::MEM_ERR;

    const char* inputTreeEndPtr = inputTree;
    diff->root = DiffReadPrefixFormat(inputTree, &inputTreeEndPtr, &diff->variables);

    free(inputTree);

    return DiffErrors::NO_ERR;
}

static DiffTreeNodeType* DiffReadPrefixFormat(const char* const string, const char** stringEndPtr,
                                              DiffVariablesArrayType* varsArr)
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

    DiffValue value;
    DiffValueType valueType;

    stringPtr = DiffReadNodeValuePrefixFormat(&value, &valueType, varsArr, stringPtr);
    DiffTreeNodeType* node = DiffTreeNodeCtor(value, valueType);

    DiffTreeNodeType* left  = DiffReadPrefixFormat(stringPtr, &stringPtr, varsArr);
    DiffTreeNodeType* right = DiffReadPrefixFormat(stringPtr, &stringPtr, varsArr);

    stringPtr = SkipSymbolsUntilStopChar(stringPtr, ')');
    ++stringPtr;

    DiffNodeSetEdges(node, left, right);

    *stringEndPtr = stringPtr;
    return node;
}

static void DiffNodeSetEdges(DiffTreeNodeType* node, DiffTreeNodeType* left, 
                                                     DiffTreeNodeType* right)
{
    assert(node);

    node->left  = left;
    node->right = right;
}

static const char* DiffReadNodeValuePrefixFormat(DiffValue* value, DiffValueType* valueType, 
                                                 DiffVariablesArrayType* varsArr,
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
        *valueType   = DiffValueType::VALUE;
        return string + shift;
    }

    shift = 0;

    static const size_t      maxInputStringSize  = 128;
    static char  inputString[maxInputStringSize] =  "";

    const char* stringPtr = string;
    sscanf(string, "%s%n", inputString, &shift);

    stringPtr = string + shift;
    assert(isspace(*stringPtr));

    int ch = GetOperator(inputString);
    if (ch != -1)
    {
        value->operation = (char)ch;
        *valueType       = DiffValueType::OPERATION;
        
        return stringPtr;
    }

    int varId = GetVariableIdByName(varsArr, inputString);
    if (varId == -1)
        varId = AddVariable(varsArr, inputString);
    
    assert(varId != -1);

    value->varId = varId;
    *valueType   = DiffValueType::VARIABLE;

    return stringPtr;
}

static int GetOperator(const char* string)
{
    assert(string);

    if (strcasecmp(string, "div") == 0)
        return '/';
    else if (strcasecmp(string, "mul") == 0)
        return '*';
    else if (strcasecmp(string, "sub") == 0)
        return '-';
    else if (strcasecmp(string, "add") == 0)
        return '+';
    else
        return -1;
}

static const char* GetOperatorName(const char operation)
{
    switch (operation)
    {
        case '*':
            return "mul";
        case '/':
            return "div";
        case '-':
            return "sub";
        case '+':
            return "add";
        
        default:
            return "ERROR";
    }

    return "ERROR";
}

static DiffTreeNodeType* DiffTreeNodeCtor(DiffValue value, DiffValueType valueType)
{   
    DiffTreeNodeType* node = (DiffTreeNodeType*)calloc(1, sizeof(*node));
    node->left      = nullptr;
    node->right     = nullptr;
    node->value     = value;
    node->valueType = valueType;

    return node;
}

static void DiffNodeDtor(DiffTreeNodeType* node)
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
    snprintf(commandName, maxCommandLength, "dot differentiator.dot -T png -o %s", imgName);
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

void DiffGraphicDump(const DiffTreeType* diff, bool openImg)
{
    assert(diff);

    static const char* dotFileName = "differentiator.dot";
    FILE* outDotFile = fopen(dotFileName, "w");

    if (outDotFile == nullptr)
        return;

    DotFileBegin(outDotFile);

    DotFileCreateNodes(diff->root, &diff->variables, outDotFile);

    DiffGraphicDump(diff->root, outDotFile);

    DotFileEnd(outDotFile);

    fclose(outDotFile);

    static size_t imgIndex = 0;
    CreateImgInLogFile(imgIndex, openImg);
    imgIndex++;
}

static void DotFileCreateNodes(const DiffTreeNodeType* node,    
                               const DiffVariablesArrayType* varsArr, FILE* outDotFile)
{
    if (node == nullptr)
        return;
    
    fprintf(outDotFile, "node%p"
                        "[shape=Mrecord, style=filled, ", node);
    
    if (node->valueType == DiffValueType::OPERATION)
        fprintf(outDotFile, "fillcolor=\"#89AC76\", label = \"%s\", ", 
                            GetOperatorName(node->value.operation));
    else if (node->valueType == DiffValueType::VALUE)
        fprintf(outDotFile, "fillcolor=\"#7293ba\", label = \"%lf\", ", node->value.value);
    else if (node->valueType == DiffValueType::VARIABLE)
        fprintf(outDotFile, "fillcolor=\"#78DBE2\", label = \"%s\", ", 
                            varsArr->data[node->value.varId].variableName);
    else 
        fprintf(outDotFile, "fillcolor=\"#FF0000\", label = \"ERROR\", ");

    fprintf(outDotFile, "color = \"#D0D000\"];\n");

    DotFileCreateNodes(node->left,  varsArr, outDotFile);
    DotFileCreateNodes(node->right, varsArr, outDotFile);
}

static void DiffGraphicDump(const DiffTreeNodeType* node, FILE* outDotFile)
{
    if (node == nullptr)
    {
        fprintf(outDotFile, "\n");
        return;
    }
    
    fprintf(outDotFile, "node%p;\n", node);

    if (node->left != nullptr) fprintf(outDotFile, "node%p->", node);
    DiffGraphicDump(node->left, outDotFile);

    if (node->right != nullptr) fprintf(outDotFile, "node%p->", node);
    DiffGraphicDump(node->right, outDotFile);
}

void DiffTextDump(const DiffTreeType* diff, const char* fileName, 
                                            const char* funcName,
                                            const int   line)
{
    assert(diff);
    assert(fileName);
    assert(funcName);

    LogBegin(fileName, funcName, line);

    Log("Tree root: %p, value: %s\n", diff->root, diff->root->value);
    Log("Tree: ");
    DiffPrintPrefixFormat(diff, nullptr);

    LOG_END();
}

void DiffDump(const DiffTreeType* diff, const char* fileName,
                                        const char* funcName,
                                        const int   line)
{
    assert(diff);
    assert(fileName);
    assert(funcName);

    DiffTextDump(diff, fileName, funcName, line);

    DiffGraphicDump(diff);
}

double DiffCalculate(const DiffTreeType* diff)
{
    assert(diff);

    return DiffCalculate(diff->root, &diff->variables);
}

static double DiffCalculate(const DiffTreeNodeType* node, const DiffVariablesArrayType* varsArr)
{
    assert(node);

    if (node->valueType == DiffValueType::VALUE)
        return node->value.value;

    if (node->valueType == DiffValueType::VARIABLE)
        return varsArr->data[node->value.varId].variableValue;

    double firstVal  = DiffCalculate(node->left,  varsArr);
    double secondVal = DiffCalculate(node->right, varsArr);
    
    return DiffCalculateUsingNodeOperation(node->value.operation, firstVal, secondVal);
}

static double DiffCalculateUsingNodeOperation(const char operation, double firstVal, double secondVal)
{
    switch(operation)
    {
        case '+':
            return firstVal + secondVal;
        case '-':
            return firstVal - secondVal;
        case '*':
            return firstVal * secondVal;
        case '/':
            return firstVal / secondVal;
        
        default:
            return NAN;
    }

    return NAN;
}

static inline int AddVariable(DiffVariablesArrayType* varsArr,  
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

static int GetVariableIdByName(const DiffVariablesArrayType* varsArr, 
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
