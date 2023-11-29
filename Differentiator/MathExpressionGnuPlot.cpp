#include <assert.h>

#include "MathExpressionGnuPlot.h"

static void ExpressionTokenPrintValue(const ExpressionTokenType* token, FILE* outStream);

static bool ExpressionOperationIsPrefix             (const ExpressionOperationId operation);
static const char* ExpressionOperationGetGnuPlotName(const ExpressionOperationId operation);

ExpressionErrors ExpressionPrintGnuPlot(ExpressionTokenType* token, FILE* outStream)
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
    err = ExpressionPrintGnuPlot(token->left, outStream);
    fprintf(outStream, ")");

    if (!isPrefixOperation) fprintf(outStream, "%s ", 
                                    ExpressionOperationGetGnuPlotName(token->value.operation));

    if (ExpressionOperationIsUnary(token->value.operation))
        return err;

    fprintf(outStream, "(");
    err = ExpressionPrintGnuPlot(token->right, outStream);
    fprintf(outStream, ")");

    return err;   
}

ExpressionErrors ExpressionPrintGnuPlot (ExpressionType* expression, FILE* outStream)
{
    assert(expression);
    assert(outStream);

    ExpressionErrors err =  ExpressionPrintGnuPlot(expression->root, outStream);

    fprintf(outStream, "\n");

    return err;
}

void ExpressionCreatePlotImg(ExpressionType* expression)
{
    assert(expression);


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
