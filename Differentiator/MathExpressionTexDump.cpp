#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "MathExpressionTexDump.h"
#include "FastInput/InputOutput.h"

static bool        ExpressionOperationIsPrefix          (const ExpressionOperationId operation);
static bool        ExpressionOperationNeedTexRightBraces(const ExpressionOperationId operation);
static bool        ExpressionOperationNeedTexLeftBraces (const ExpressionOperationId operation);
static const char* ExpressionOperationGetTexName        (const ExpressionOperationId operation);

static LatexReplacementType* ExpressionLatexFindReplacement(const LatexReplacementArrType* arr,
                                                            const ExpressionTokenType* token);
static LatexReplacementType* ExpressionLatexAddReplacement(LatexReplacementArrType* arr,
                                                           const ExpressionTokenType* token);
static char* ExpressionLatexReplacementCreateName();
static size_t ExpressionLatexGetLen(ExpressionOperationId operation, const size_t leftSz, 
                                                                     const size_t rightSz);

static bool HaveToPutBrackets(const ExpressionTokenType* parent, 
                              const ExpressionTokenType* son,
                              const LatexReplacementArrType* arr);

static void ExpressionTokenPrintValue   (const ExpressionTokenType* token, 
                                         FILE* outStream);

ExpressionErrors ExpressionPrintTex(const ExpressionType* expression,
                                    FILE* outStream,
                                    const char* string,
                                    LatexReplacementArrType* replacementArr)
{
    assert(expression);
    assert(outStream);

    return ExpressionTokenPrintTexWithTrollString(expression->root, outStream, string, 
                                                  replacementArr);
}

ExpressionErrors ExpressionTokenPrintTexWithTrollString(
                                                const ExpressionTokenType* rootToken,
                                                FILE* outStream,
                                                const char* string,
                                                LatexReplacementArrType* replacementArr)
{
    assert(rootToken);
    assert(outStream);

    static const char* roflStrings[] = 
    {
        "Kind of obvious expression transformation. ",
        "Easy to see that it's equal. ", 
        "Lubopitniy chitatel can show this perehod by himself. ",
        "I have a proof of this transformation, but there is not enough space in this margin \\cite{Arithmetica}. ",
        "Don't ask me to prove this. ",
        "Perun \\cite{Ruses} sent me the solution and I have no right to believe or not to believe.",
        "C'mon guys, it's not rocket science. ",
        "Bez kommentariev. ",
        "No one is reading, so I'm gonna say that I hate calculus. ",
        "If you're reading this - why?",
        "Even a monkey can learn how to do it, why won't you do it by yourself?",
        "Nikto ne zametit, chto ya ne smog perevesti eto dlya svoe' stat'i. ",

        "Explanation is available only for premium subscribers."
        "You can become one of them - it costs only 5 bucks a week.",
    };

    static const size_t numberOfRoflStrings = sizeof(roflStrings) / sizeof(*roflStrings);

    if (string == nullptr)
        fprintf(outStream, "%s It is:\n", roflStrings[rand() % numberOfRoflStrings]);
    else
        fprintf(outStream, "%s\n", string);
    

    size_t prevArrSize = 0;
    if (replacementArr)
    {
        prevArrSize = replacementArr->size;
        ExpressionLatexReplacementArrayInit(rootToken, replacementArr);
    }

    fprintf(outStream, "\\begin{gather}\n");

    ExpressionErrors err = ExpressionTokenPrintTex(rootToken, outStream, replacementArr);

    fprintf(outStream, "\n\\end{gather}\n");

    if (replacementArr && prevArrSize < replacementArr->size)
    {
        fprintf(outStream, "Using these replacements: \n");
        for (size_t i = prevArrSize; i < replacementArr->size; ++i)
        {
            ExpressionLatexReplacementPrint(replacementArr, replacementArr->data[i].token, 
                                                                                outStream);
        }                                                                                
    }

    return err;
}

ExpressionErrors ExpressionTokenPrintTex(const ExpressionTokenType* token, 
                                         FILE* outStream,
                                         const LatexReplacementArrType* replacementArr)
{
    assert(token);
    assert(outStream);

    if (token->left == nullptr && token->right == nullptr)
    {
        ExpressionTokenPrintValue(token, outStream);

        return ExpressionErrors::NO_ERR;
    }

    LatexReplacementType* replacement = ExpressionLatexFindReplacement(replacementArr, token);
    if (replacement != nullptr)
    {
        fprintf(outStream, "%s", replacement->replacementStr);
        return ExpressionErrors::NO_ERR;
    }

    ExpressionErrors err = ExpressionErrors::NO_ERR;
    assert((token->valueType == ExpressionTokenValueTypeof::OPERATION));

    bool isPrefixOperation = ExpressionOperationIsPrefix(token->value.operation);

    if (isPrefixOperation) fprintf(outStream, "%s ", 
                                    ExpressionOperationGetTexName(token->value.operation));

    bool needLeftBrackets  = HaveToPutBrackets(token, token->left, replacementArr);
    bool needTexLeftBraces = ExpressionOperationNeedTexLeftBraces(token->value.operation);

    if (needTexLeftBraces)                      fprintf(outStream, "{");
    if (!needTexLeftBraces && needLeftBrackets) fprintf(outStream, "(");

    err = ExpressionTokenPrintTex(token->left, outStream, replacementArr);

    if (!needTexLeftBraces && needLeftBrackets) fprintf(outStream, ")");
    if (needTexLeftBraces)                      fprintf(outStream, "}");

    if (!isPrefixOperation) fprintf(outStream, "%s ", 
                                    ExpressionOperationGetTexName(token->value.operation));

    if (ExpressionOperationIsUnary(token->value.operation))
        return err;

    bool needTexRightBraces = ExpressionOperationNeedTexRightBraces(token->value.operation);
    bool needRightBrackets  = HaveToPutBrackets(token, token->right, replacementArr);
    
    if (needTexRightBraces)                       fprintf(outStream, "{");
    if (!needTexRightBraces && needRightBrackets) fprintf(outStream, "(");

    err = ExpressionTokenPrintTex(token->right, outStream, replacementArr);

    if (!needTexRightBraces && needRightBrackets) fprintf(outStream, ")");
    if (needTexRightBraces)                       fprintf(outStream, "}");

    return err;   
}


//---------------------------------------------------------------------------------------

static const char* ExpressionOperationGetTexName(const ExpressionOperationId operation)
{
    #define GENERATE_OPERATION_CMD(NAME, v1, v2, v3, v4, TEX_NAME, ...) \
        case ExpressionOperationId::NAME:                               \
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

static void ExpressionTokenPrintValue(const ExpressionTokenType* token, FILE* outStream)
{
    assert(token->valueType != ExpressionTokenValueTypeof::OPERATION);

    switch (token->valueType)
    {
        case ExpressionTokenValueTypeof::VALUE:
            fprintf(outStream, "%lg ", token->value.value);
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
                              const ExpressionTokenType* son,
                              const LatexReplacementArrType* arr)
{
    assert(parent);

    assert(parent->valueType == ExpressionTokenValueTypeof::OPERATION);

    if (son->valueType != ExpressionTokenValueTypeof::OPERATION)
        return false;

    if (ExpressionLatexFindReplacement(arr, son) != nullptr)
        return false;

    ExpressionOperationId parentOperation = parent->value.operation;
    ExpressionOperationId sonOperation    = son->value.operation;

    if (parentOperation == ExpressionOperationId::POW && 
        sonOperation    == ExpressionOperationId::DIV)
        return true;

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

void LaTexInsertImg(const char* imgName, FILE* outStream, const char* string)
{
    assert(imgName);
    assert(outStream);

    fprintf(outStream, "%s\n", string);
    fprintf(outStream, "\\begin{figure}[H]\n"
                       "\\centering\n"
                       "\\includegraphics[scale=0.6]{%s}\n"
                       "\\end{figure}\n", imgName);
}

void LatexFileTrollingStart(FILE* outTex)
{
    assert(outTex);

    static const char* inAssetFileName = "latexStartAsset.txt";
    FILE* inAssetStream = fopen(inAssetFileName, "r");

    if (inAssetStream == nullptr)
        return;
    
    char* text = ReadText(inAssetStream);

    PrintText(text, strlen(text), outTex);

    free(text);
}

void LatexFileTrollingEnd  (FILE* outTex)
{
    assert(outTex);

    static const char* inAssetFileName = "latexEndAsset.txt";
    FILE* inAssetStream = fopen(inAssetFileName, "r");

    if (inAssetStream == nullptr)
        return;
    
    char* text = ReadText(inAssetStream);

    PrintText(text, strlen(text), outTex);

    free(text);
}

void LatexCreatePdf(const char* fileName)
{
    assert(fileName);

    static const size_t maxCommandLength  = 512;
    static char command[maxCommandLength]  = "";

    snprintf(command, maxCommandLength, "lualatex %s", fileName);

    system(command);
}

void LaTexStartNewSection(const char* sectionName, FILE* outStream)
{
    assert(sectionName);
    assert(outStream);

    fprintf(outStream, "\\section{%s}\n", sectionName);
}

static LatexReplacementType* ExpressionLatexFindReplacement(const LatexReplacementArrType* arr,
                                                            const ExpressionTokenType* token)
{
    assert(token);

    if (arr == nullptr)
        return nullptr;

    for (size_t i = 0; i < arr->size; ++i)
    {
        if (arr->data[i].token == token)
            return arr->data + i;
    }
    
    return nullptr;
}

static LatexReplacementType* ExpressionLatexAddReplacement(LatexReplacementArrType* arr,
                                                           const ExpressionTokenType* token)
{
    assert(token);

    if (arr == nullptr)
        return nullptr;

    if (arr->size >= arr->capacity)
        return nullptr;

    LatexReplacementType* replacement = ExpressionLatexFindReplacement(arr, token);

    if (replacement != nullptr)
        return replacement;

    char* replaceName = ExpressionLatexReplacementCreateName();

    arr->data[arr->size].token       = token;
    arr->data[arr->size].replacementStr = replaceName;
    arr->size++;
    
    return arr->data + arr->size - 1;
}

static char* ExpressionLatexReplacementCreateName()
{
    static int charId  = -1;
    static int numId   = 0;
    
    int oldCharId = charId;
    charId = (charId + 1) % ('z' - 'a' + 1);
    if (charId < oldCharId)
        numId++;

    static const size_t      maxReplacementSz  = 32;
    static char  replacement[maxReplacementSz] = "";

    snprintf(replacement, maxReplacementSz, "%c_%d", charId + 'a', numId);

    return strdup(replacement);
}

void ExpressionLatexReplacementArrayCtor(LatexReplacementArrType* arr, size_t capacity)
{
    assert(arr);

    arr->data     = (LatexReplacementType*) calloc(capacity, sizeof(*(arr->data)));
    arr->capacity = capacity;
    arr->size     = 0;
}

void ExpressionLatexReplacementArrayDtor(LatexReplacementArrType* arr)
{
    assert(arr);

    for (size_t i = 0; i < arr->size; ++i)
    {
        if (arr->data[i].replacementStr)
        {
            arr->data[i].token = nullptr;
            free(arr->data[i].replacementStr);
        }
    }

    arr->capacity = 0;
    arr->size     = 0;
    free(arr->data);
}

size_t ExpressionLatexReplacementArrayInit(const ExpressionTokenType* token, 
                                           LatexReplacementArrType* arr)
{
    assert(arr);

    if (token == nullptr)
        return 0;

    static const size_t maxTokenSize = 16;
    if (token->valueType == ExpressionTokenValueTypeof::VALUE)
        return 1;
    
    if (token->valueType == ExpressionTokenValueTypeof::VARIABLE)
        return strlen(token->value.varPtr->variableName);
    
    size_t leftSz  = ExpressionLatexReplacementArrayInit(token->left,  arr);
    size_t rightSz = ExpressionLatexReplacementArrayInit(token->right, arr);

    assert(token->valueType == ExpressionTokenValueTypeof::OPERATION);
    size_t mySz = ExpressionLatexGetLen(token->value.operation, leftSz, rightSz);
    
    if (mySz >= maxTokenSize)
    {
        ExpressionLatexAddReplacement(arr, token);
        return 1;
    }

    return mySz;
}

static inline size_t max(const size_t leftSz, const size_t rightSz)
{
    return leftSz > rightSz ? leftSz : rightSz;
}

static size_t ExpressionLatexGetLen(ExpressionOperationId operation, const size_t leftSz, 
                                                                     const size_t rightSz)
{
    #define GENERATE_OPERATION_CMD(NAME, v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11,  \
                                   SUM_LENS_CODE)                                       \
            case ExpressionOperationId::NAME:                                           \
            {                                                                           \
                SUM_LENS_CODE;                                                          \
                break;                                                                  \
            }

    switch (operation)
    {
        #include "Operations.h"
        
        default:
            break;
    }

    #undef GENERATE_OPERATION_CMD

    return 0;
}

ExpressionErrors ExpressionLatexReplacementPrint(LatexReplacementArrType* arr,
                                                 const ExpressionTokenType* token,
                                                 FILE* outStream)
{
    assert(arr);

    if (arr->size == 0)
        return ExpressionErrors::NO_ERR;

    fprintf(outStream, "\\begin{gather*}\n");

    if (token != nullptr)
    {
        LatexReplacementType* replacement = ExpressionLatexFindReplacement(arr, token);

        if (replacement == nullptr)
            return ExpressionErrors::NO_REPLACEMENT;

        replacement->token = nullptr;

        fprintf(outStream, "%s = ", replacement->replacementStr);
        ExpressionTokenPrintTex(token, outStream, arr);
        fprintf(outStream, "\n\\end{gather*}\n");

        replacement->token = token;

        return ExpressionErrors::NO_ERR;
    }

    for (size_t i = 0; i < arr->size; ++i)
    {
        fprintf(outStream, "%s = ", arr->data[i].replacementStr);

        const ExpressionTokenType* tmpToken = arr->data[i].token;
        arr->data[i].token = nullptr;

        ExpressionTokenPrintTex(arr->data[i].token, outStream, arr);
        fprintf(outStream, "\\\\\n");

        arr->data[i].token = tmpToken;
    }

    fprintf(outStream, "\\end{gather}\n");
    return ExpressionErrors::NO_ERR;
}
