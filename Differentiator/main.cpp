#include "MathExpressionsHandler.h"
#include "../Common/Log.h"

int main(const int argc, const char* argv[])
{
    setbuf(stdout, nullptr);
    LogOpen(argv[0]);

    MathExpressionType MathExpressionTreePrefixInput;
    MathExpressionType MathExpressionTreeInfixInput;
    MathExpressionCtor(&MathExpressionTreePrefixInput);
    MathExpressionCtor(&MathExpressionTreeInfixInput);

    FILE* inStreamPrefix = fopen("input.txt",  "r");
    //FILE* inStreamInfix  = fopen("input2.txt", "r");
    MathExpressionReadPrefixFormat(&MathExpressionTreePrefixInput, inStreamPrefix);
    //MathExpressionReadInfixFormat(&MathExpressionTreeInfixInput,  inStreamInfix);

    MathExpressionReadVariables(&MathExpressionTreePrefixInput);

    MathExpressionPrintPrefixFormat     (&MathExpressionTreePrefixInput);
    MathExpressionPrintEquationFormat   (&MathExpressionTreePrefixInput);
    MathExpressionPrintEquationFormatTex(&MathExpressionTreePrefixInput);
    MathExpressionGraphicDump(&MathExpressionTreePrefixInput);
    
    //MathExpressionGraphicDump(&MathExpressionTreeInfixInput);
    //printf("\n\n");
    //MathExpressionPrintEquationFormat(&MathExpressionTreeInfixInput);

    printf("Calculation result: %lf", MathExpressionCalculate(&MathExpressionTreePrefixInput));
}