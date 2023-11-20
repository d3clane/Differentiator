#include <assert.h>
#include <ctype.h>
#include <string.h>

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
 
static DiffErrors DiffPrintPrefixFormat(const DiffTreeNodeType* node, FILE* outStream);

static DiffTreeNodeType* DiffReadPrefixFormat(const char* const string, const char** stringEndPtr);

static const char* DiffReadNodeValuePrefixFormat(DiffValue* value, DiffValueType* valueType, 
                                                 const char* stringPtr);

static int GetOperator(const char* string);
static const char* GetOperatorName(const char operation);


static        void DiffGraphicDump   (const DiffTreeNodeType* node, FILE* outDotFile);
static        void DotFileCreateNodes(const DiffTreeNodeType* node, FILE* outDotFile);
static inline void CreateImgInLogFile(const size_t imgIndex, bool openImg);
static inline void DotFileBegin(FILE* outDotFile);
static inline void DotFileEnd(FILE* outDotFile);
                            
DiffErrors DiffCtor(DiffTreeType* diff, DiffTreeNodeType* root)
{
    assert(diff);

    if (root != nullptr)
    {
        diff->root = root;
        return DiffErrors::NO_ERR;
    }

    diff->root = nullptr;

    return DiffErrors::NO_ERR;
}

DiffErrors DiffDtor(DiffTreeType* diff)
{
    assert(diff);

    DiffDtor(diff->root);
    diff->root = nullptr;

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

    DiffErrors err = DiffPrintPrefixFormat(diff->root, outStream);

    PRINT(outStream, "\n");

    LOG_END();

    return err;
}

static DiffErrors DiffPrintPrefixFormat(const DiffTreeNodeType* node, FILE* outStream)
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
        PRINT(outStream, "%c ", 'x'); //TODO: поменять на функцию, которая вернет тип переменной по его id, а не просто 'x'
    else
        PRINT(outStream, "%s ", GetOperatorName(node->value.operation)); //TODO: вместо вывода +, -, /, ... вывести add sub div mul

    DiffErrors err = DiffErrors::NO_ERR;

    err = DiffPrintPrefixFormat(node->left, outStream);
    err = DiffPrintPrefixFormat(node->right, outStream);

    PRINT(outStream, ")");
    
    return err;
}

#undef PRINT

DiffErrors DiffReadPrefixFormat(DiffTreeType* diff, FILE* inStream)
{
    assert(diff);
    assert(inStream);

    char* inputTree = ReadText(inStream);

    if (inputTree == nullptr)
        return DiffErrors::MEM_ERR;

    const char* inputTreeEndPtr = inputTree;
    diff->root = DiffReadPrefixFormat(inputTree, &inputTreeEndPtr);

    free(inputTree);

    return DiffErrors::NO_ERR;
}

static DiffTreeNodeType* DiffReadPrefixFormat(const char* const string, const char** stringEndPtr)
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

    stringPtr = DiffReadNodeValuePrefixFormat(&value, &valueType, stringPtr);
    DiffTreeNodeType* node = DiffTreeNodeCtor(value, valueType);

    DiffTreeNodeType* left  = DiffReadPrefixFormat(stringPtr, &stringPtr);
    DiffTreeNodeType* right = DiffReadPrefixFormat(stringPtr, &stringPtr);

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

    value->varId = 0; //TODO: заполнить таблицу labels, предоставить номер оттуда
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

void DiffGraphicDump(const DiffTreeType* tree, bool openImg)
{
    assert(tree);

    static const char* dotFileName = "differentiator.dot";
    FILE* outDotFile = fopen(dotFileName, "w");

    if (outDotFile == nullptr)
        return;

    DotFileBegin(outDotFile);

    DotFileCreateNodes(tree->root, outDotFile);

    DiffGraphicDump(tree->root, outDotFile);

    DotFileEnd(outDotFile);

    fclose(outDotFile);

    static size_t imgIndex = 0;
    CreateImgInLogFile(imgIndex, openImg);
    imgIndex++;
}

static void DotFileCreateNodes(const DiffTreeNodeType* node, FILE* outDotFile)
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
        fprintf(outDotFile, "fillcolor=\"#78DBE2\", label = \"%c\", ", 'x'); //TODO: вывод нужной переменной
    else 
        fprintf(outDotFile, "fillcolor=\"#FF0000\", label = \"ERROR\", ");

    fprintf(outDotFile, "color = \"#D0D000\"];\n");

    DotFileCreateNodes(node->left,  outDotFile);
    DotFileCreateNodes(node->right, outDotFile);
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

void DiffTextDump(const DiffTreeType* tree, const char* fileName, 
                                        const char* funcName,
                                        const int   line)
{
    assert(tree);
    assert(fileName);
    assert(funcName);

    LogBegin(fileName, funcName, line);

    Log("Tree root: %p, value: %s\n", tree->root, tree->root->value);
    Log("Tree: ");
    DiffPrintPrefixFormat(tree, nullptr);

    LOG_END();
}

void DiffDump(const DiffTreeType* tree, const char* fileName,
                                    const char* funcName,
                                    const int   line)
{
    assert(tree);
    assert(fileName);
    assert(funcName);

    DiffTextDump(tree, fileName, funcName, line);

    DiffGraphicDump(tree);
}
