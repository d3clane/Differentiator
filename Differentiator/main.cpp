#include "MathExpressionsHandler.h"
#include "../Common/Log.h"

int main(const int argc, const char* argv[])
{
    setbuf(stdout, nullptr);
    LogOpen(argv[0]);

    MathExpressionType mathExpressionPrefixInput;
    MathExpressionType mathExpressionInfixInput;
    MathExpressionCtor(&mathExpressionPrefixInput);
    //MathExpressionCtor(&mathExpressionPrefixInput);
    //MathExpressionCtor(&mathExpressionInfixInput);

    FILE* inStreamPrefix = fopen("input.txt",  "r");
    //FILE* inStreamInfix  = fopen("input2.txt", "r");
    MathExpressionReadPrefixFormat(&mathExpressionPrefixInput, inStreamPrefix);
    //MathExpressionReadInfixFormat(&mathExpressionInfixInput,  inStreamInfix);

    MathExpressionReadVariables(&mathExpressionPrefixInput);

    MathExpressionPrintPrefixFormat     (&mathExpressionPrefixInput);
    MathExpressionPrintEquationFormat   (&mathExpressionPrefixInput);
    MathExpressionPrintEquationFormatTex(&mathExpressionPrefixInput);
    MathExpressionGraphicDump(&mathExpressionPrefixInput);
    
    //MathExpressionGraphicDump(&mathExpressionInfixInput);
    //printf("\n\n");
    //MathExpressionPrintEquationFormat(&mathExpressionInfixInput);

    printf("Calculation result: %lf\n\n\n", MathExpressionCalculate(&mathExpressionPrefixInput));

    MathExpressionType mathExpressionDiff =  MathExpressionDifferentiate(&mathExpressionPrefixInput);

    MathExpressionPrintEquationFormatTex(&mathExpressionDiff);
    MathExpressionGraphicDump(&mathExpressionDiff, true);
    printf("Diff result in x: %lf\n\n\n", MathExpressionCalculate(&mathExpressionDiff));
    //MATH_EXPRESSION_DUMP(&mathExpressionDiff);
}