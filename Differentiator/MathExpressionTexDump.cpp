#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "MathExpressionTexDump.h"
#include "FastInput/InputOutput.h"

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

    static const char* roflStrings[] = 
    {
        "Kind of obvious expression transformation. ",
        "Easy to see that it's equal to. ", 
        "Lubopitniy chitatel can show this perehod by himself. ",
        "I have a proof of this transformation, but there is not enough space in this margin. ",
        "Don't ask me to prove this. ",
        "Perun sent me the solution and I have no right to believe or not to believe. ",
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
    
    fprintf(outStream, "\\begin{gather}\n");

    ExpressionErrors err = ExpressionTokenPrintTex(rootToken, outStream);

    fprintf(outStream, "\n\\end{gather}\n");

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

static void ExpressionTokenPrintValue(const ExpressionTokenType* token, 
                                      FILE* outStream)
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
                              const ExpressionTokenType* son)
{
    assert(parent);

    assert(parent->valueType == ExpressionTokenValueTypeof::OPERATION);

    if (son->valueType != ExpressionTokenValueTypeof::OPERATION)
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
