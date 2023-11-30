#include <assert.h>
#include <string.h>

#include "MathExpressionGnuPlot.h"

static void ExpressionTokenPrintValue(const ExpressionTokenType* token, FILE* outStream);

static bool        ExpressionOperationIsPrefix      (const ExpressionOperationId operation);
static const char* ExpressionOperationGetGnuPlotName(const ExpressionOperationId operation);

static char* CreateImgName(const size_t imgIndex);

ExpressionErrors ExpressionPrintGnuPlotFormat(ExpressionTokenType* token, FILE* outStream)
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

    bool isPrefixOperation = ExpressionOperationIsPrefix(token->value.operation);

    if (isPrefixOperation) fprintf(outStream, "%s ", 
                                    ExpressionOperationGetGnuPlotName(token->value.operation));

    fprintf(outStream, "(");
    err = ExpressionPrintGnuPlotFormat(token->left, outStream);
    fprintf(outStream, ")");

    if (!isPrefixOperation) fprintf(outStream, "%s ", 
                                    ExpressionOperationGetGnuPlotName(token->value.operation));

    if (ExpressionOperationIsUnary(token->value.operation))
        return err;

    fprintf(outStream, "(");
    err = ExpressionPrintGnuPlotFormat(token->right, outStream);
    fprintf(outStream, ")");

    return err;   
}

ExpressionErrors ExpressionPrintGnuPlotFormat (ExpressionType* expression, FILE* outStream)
{
    assert(expression);
    assert(outStream);

    return ExpressionPrintGnuPlotFormat(expression->root, outStream);
}

static char* CreateImgName(const size_t imgIndex)
{
    static const size_t maxImgNameLength  = 256;
    static char imgName[maxImgNameLength] = "";
    snprintf(imgName, maxImgNameLength, "Graphs/graph_%zu_time_%s.png", imgIndex, __TIME__);

    return strdup(imgName);
}

void GnuPlotImgCreate(const char* plotFileName)
{
    assert(plotFileName);

    static const size_t maxCommandLen = 256;
    static char  commandName[]        = "";

    snprintf(commandName, maxCommandLen, "chmod +x %s", plotFileName);
    system(commandName);

    snprintf(commandName, maxCommandLen, "./%s", plotFileName);
    system(commandName);
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
            fprintf(outStream, "%s ", ExpressionOperationGetGnuPlotName(token->value.operation));
            break;
        
        default:
            break;
    }
}

static const char* ExpressionOperationGetGnuPlotName(const  ExpressionOperationId operation)
{
    #define GENERATE_OPERATION_CMD(NAME, v1, v2, v3, v4, v5, v6, v7, v8, v9, GNU_PLOT_NAME, ...) \
        case ExpressionOperationId::NAME:                                                        \
            return GNU_PLOT_NAME;

    switch (operation)
    {
        #include "Operations.h"

        default:
            break;
    }

    #undef GENERATE_OPERATION_CMD

    return nullptr;
}

static bool ExpressionOperationIsPrefix(const ExpressionOperationId operation)
{

    #define GENERATE_OPERATION_CMD(NAME, v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, PRINT_FORMAT, ...) \
        case ExpressionOperationId::NAME:                                                            \
            return ExpressionOperationFormat::PRINT_FORMAT == ExpressionOperationFormat::PREFIX;
        
    switch (operation)
    {
        #include "Operations.h"

        default:
            break;
    }

    return false;
}

const char* GnuPlotFileCreate(char** outImgName)
{
    static const char* gnuPlotFileName = "expressionsTmpPlot.gpi";

    FILE* outStream = fopen(gnuPlotFileName, "w");

    static const char* gnuPlotFilePrefix = "#! /opt/homebrew/bin/gnuplot -persist\n"
                                           "set xlabel \"X\"\n" 
                                           "set ylabel \"Y\"\n"
                                           "set xrange[-0.6:0.6]\n"
                                           "set terminal png size 800, 600\n";

    fprintf(outStream, "%s\n", gnuPlotFilePrefix);

    static size_t imgIndex = 1337;
    char* imgName = CreateImgName(imgIndex);
    imgIndex++;

    fprintf(outStream, "set output \"%s\"\n", imgName);

    fprintf(outStream, "plot ");
    fclose(outStream);

    if (outImgName)
        *outImgName = imgName;
    else
        free(imgName);  
    
    return gnuPlotFileName;
}

ExpressionErrors ExpressionGnuPlotAddFunc(const char* plotFileName,  ExpressionType* expression, 
                                                        const char* funcTitle, 
                                                        const char* funcColor)
{
    FILE* outStream = fopen(plotFileName, "a");

    ExpressionErrors err = ExpressionPrintGnuPlotFormat(expression, outStream);
    fprintf(outStream, " title \"%s\" lc rgb \"%s\", ", funcTitle, funcColor);

    fclose(outStream);

    return err;
}

ExpressionErrors ExpressionPlotTwoFuncs(ExpressionType* func1, 
                                        const char* title1,  const char* color1, 
                                        ExpressionType* func2,
                                        const char* title2, const char* color2,
                                        char** outImgName)
{
    assert(func1);
    assert(func2);
    assert(title1);
    assert(title2);
    assert(color1);
    assert(color2);

    char* imgName = nullptr;

    const char* gnuPlotFileName = GnuPlotFileCreate(&imgName);
    ExpressionErrors err = ExpressionGnuPlotAddFunc(gnuPlotFileName, func1, title1, color1);

    if (err != ExpressionErrors::NO_ERR)
    {
        assert(imgName);
        free  (imgName);

        return err;
    }

    err = ExpressionGnuPlotAddFunc(gnuPlotFileName, func2, title2, color2);

    GnuPlotImgCreate(gnuPlotFileName);

    if (outImgName == nullptr)
        free(imgName);
    else
        *outImgName = imgName;

    return err;
}

ExpressionErrors ExpressionPlotFunc(ExpressionType* func, const char* funcTitle, 
                                                          const char* funcColor, 
                                                          char** outImgName)
{
    assert(func);
    assert(funcTitle);
    assert(funcColor);
    
    char* imgName = nullptr;

    const char* gnuPlotFileName = GnuPlotFileCreate(&imgName);
    ExpressionErrors err = ExpressionGnuPlotAddFunc(gnuPlotFileName, func, funcTitle, funcColor);

    if (err != ExpressionErrors::NO_ERR)
    {
        assert(imgName);
        free  (imgName);

        return err;
    }

    GnuPlotImgCreate(gnuPlotFileName);

    if (outImgName == nullptr)
        free(imgName);
    else
        *outImgName = imgName;

    return err;
}
