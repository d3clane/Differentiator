#ifndef MATH_EXPRESSION_GNU_PLOT_H
#define MATH_EXPRESSION_GNU_PLOT_H

#include "MathExpressionsMain.h"

ExpressionErrors ExpressionPrintGnuPlotFormat (ExpressionTokenType* token, FILE* outStream);
ExpressionErrors ExpressionPrintGnuPlotFormat (ExpressionType* expression, FILE* outStream);

ExpressionErrors ExpressionGnuPlotAddFunc(const char* plotFileName,  ExpressionType* expression, 
                                                                     const char* funcTitle, 
                                                                     const char* funcColor);
ExpressionErrors ExpressionPlotFuncAndMacloren(ExpressionType* func, ExpressionType* macloren);

void        GnuPlotImgCreate (const char* plotFileName);
const char* GnuPlotFileCreate(char** outImgName = nullptr);

#endif