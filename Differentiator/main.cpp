#include "MathExpressionsMain.h"
#include "MathExpressionInOut.h"
#include "MathExpressionCalculations.h"

#include "Common/Log.h"

int main(const int argc, const char* argv[])
{
    setbuf(stdout, nullptr);
    LogOpen(argv[0]);

    ExpressionType ExpressionPrefixInput;
    ExpressionType ExpressionInfixInput;
    ExpressionCtor(&ExpressionPrefixInput);
    //ExpressionCtor(&ExpressionPrefixInput);
    //ExpressionCtor(&ExpressionInfixInput);

    FILE* inStreamPrefix = fopen("input.txt",  "r");
    FILE* output         = fopen("output.txt", "w");
    FILE* outputTex      = fopen("output.tex", "w");
    setbuf(outputTex, nullptr);
    //FILE* inStreamInfix  = fopen("input2.txt", "r");
    ExpressionReadPrefixFormat(&ExpressionPrefixInput, inStreamPrefix);
    //ExpressionReadInfixFormat(&ExpressionInfixInput,  inStreamInfix);

    ExpressionReadVariables(&ExpressionPrefixInput);

    ExpressionPrintPrefixFormat     (&ExpressionPrefixInput, output);
    ExpressionGraphicDump(&ExpressionPrefixInput);
    ExpressionPrintEquationFormat   (&ExpressionPrefixInput);
    ExpressionPrintTex(&ExpressionPrefixInput, outputTex);
    

    printf("Calculation result: %lf\n\n\n", ExpressionCalculate(&ExpressionPrefixInput));

    ExpressionType ExpressionDiff =  ExpressionDifferentiate(&ExpressionPrefixInput, outputTex);

    ExpressionPrintTex(&ExpressionDiff, outputTex);
    ExpressionGraphicDump(&ExpressionDiff);


    printf("Diff result in x: %lf\n\n\n", ExpressionCalculate(&ExpressionDiff));
    
}