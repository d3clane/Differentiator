#ifndef MATH_EXPRESSION_IN_OUT_H
#define MATH_EXRPESSION_IN_OUT_H

#include <stdio.h>

#include "MathExpressionsMain.h"

ExpressionErrors ExpressionPrintPrefixFormat     (const ExpressionType* expression, 
                                                          FILE* outStream = stdout);
ExpressionErrors ExpressionPrintEquationFormat   (const ExpressionType* expression, 
                                                          FILE* outStream = stdout);

ExpressionErrors ExpressionReadPrefixFormat(ExpressionType* expression, 
                                            FILE* inStream = stdin);
ExpressionErrors ExpressionReadInfixFormat (ExpressionType* expression, 
                                            FILE* inStream = stdin);
ExpressionErrors ExpressionReadVariables(ExpressionType* expression);

#endif 