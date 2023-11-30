#ifndef MATH_EXPRESSION_GNU_PLOT_H
#define MATH_EXPRESSION_GNU_PLOT_H

#include "MathExpressionsMain.h"

ExpressionErrors ExpressionPrintGnuPlotFormat (ExpressionTokenType* token, FILE* outStream);
ExpressionErrors ExpressionPrintGnuPlotFormat (ExpressionType* expression, FILE* outStream);

ExpressionErrors ExpressionGnuPlotAddFunc(const char* plotFileName,  ExpressionType* expression, 
                                                                     const char* funcTitle, 
                                                                     const char* funcColor);
ExpressionErrors ExpressionPlotTwoFuncs(ExpressionType* func1, 
                                        const char* title1,  const char* color1, 
                                        ExpressionType* func2,
                                        const char* title2, const char* color2,
                                        char** outImgName = nullptr);

ExpressionErrors ExpressionPlotFunc(ExpressionType* func, const char* funcTitle, 
                                                          const char* funcColor, 
                                                          char** outImgName = nullptr);

void        GnuPlotImgCreate (const char* plotFileName);
const char* GnuPlotFileCreate(char** outImgName = nullptr);

#endif