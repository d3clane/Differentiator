#ifndef MATH_EXPRESSION_GNU_PLOT_H
#define MATH_EXPRESSION_GNU_PLOT_H

#include "MathExpressionsMain.h"

ExpressionErrors ExpressionPrintGnuPlot     (ExpressionTokenType* token, FILE* outStream);
ExpressionErrors ExpressionPrintGnuPlot     (ExpressionType* expression, FILE* outStream);
void ExpressionCreatePlotImg    (ExpressionType* expression);

#endif