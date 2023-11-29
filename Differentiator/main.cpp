#include "MathExpressionsMain.h"
#include "MathExpressionInOut.h"
#include "MathExpressionCalculations.h"
#include "MathExpressionGnuPlot.h"
#include "MathExpressionTexDump.h"

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

    ExpressionType expressionDiff =  ExpressionDifferentiate(&expression, outputTex);

    ExpressionPrintTex   (&expressionDiff, outputTex, "Итоговый ответ: ");
    ExpressionGraphicDump(&expressionDiff);

    ExpressionType maclorenSeries = ExpressionMacloren(&expression, 10);

    //ExpressionGraphicDump(&maclorenSeries);
    ExpressionPrintTex   (&maclorenSeries, outputTex, "Разложение по маклорену: ");
    ExpressionPrintEquationFormat(&maclorenSeries, output);
    printf("Diff result in x: %lf\n\n\n", ExpressionCalculate(&expressionDiff));

    ExpressionPlotFuncAndMacloren(&expression, &maclorenSeries);
}