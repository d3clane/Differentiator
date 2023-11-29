#include "MathExpressionsMain.h"
#include "MathExpressionInOut.h"
#include "MathExpressionCalculations.h"

#include "Common/Log.h"

int main(const int argc, const char* argv[])
{
    setbuf(stdout, nullptr);
    LogOpen(argv[0]);

    ExpressionType  expression = {};
    ExpressionCtor(&expression);

    FILE* inStreamPrefix = fopen("input.txt",  "r");
    FILE* output         = fopen("output.txt", "w");
    FILE* outputTex      = fopen("output.tex", "w");
    setbuf(outputTex, nullptr);

    ExpressionReadPrefixFormat(&expression, inStreamPrefix);
    ExpressionReadVariables(&expression);

    ExpressionPrintPrefixFormat     (&expression, output);
    ExpressionGraphicDump           (&expression);
    ExpressionPrintEquationFormat   (&expression);
    ExpressionPrintTex              (&expression, outputTex);

    printf("Calculation result: %lf\n\n\n", ExpressionCalculate(&expression));

    ExpressionType ExpressionDiff =  ExpressionDifferentiate(&expression, outputTex);

    ExpressionPrintTex   (&ExpressionDiff, outputTex, "Итоговый ответ: ");
    ExpressionGraphicDump(&ExpressionDiff);

    ExpressionType taylorSeries = ExpressionTaylorize(&expression, 3);

    //ExpressionGraphicDump(&taylorSeries);
    ExpressionPrintTex   (&taylorSeries, outputTex, "Разложение по тейлору: ");

    printf("Diff result in x: %lf\n\n\n", ExpressionCalculate(&ExpressionDiff));
}