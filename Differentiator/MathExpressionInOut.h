#ifndef MATH_EXPRESSION_IN_OUT_H
#define MATH_EXRPESSION_IN_OUT_H

#include <stdio.h>

#include "MathExpressionsHandler.h"

ExpressionErrors ExpressionPrintPrefixFormat     (const ExpressionType* expression, 
                                                          FILE* outStream = stdout);
ExpressionErrors ExpressionPrintEquationFormat   (const ExpressionType* expression, 
                                                          FILE* outStream = stdout);
ExpressionErrors ExpressionPrintTex(const ExpressionType* expression,
                                                          FILE* outStream = stdout, 
                                                          const char* funnyString = nullptr);

ExpressionErrors ExpressionPrintTexTrollString(
                                                const ExpressionTokenType* rootToken,
                                                const ExpressionVariablesArrayType* varsArr,
                                                FILE* outStream,
                                                const char* string = nullptr);
                                                
ExpressionErrors ExpressionReadPrefixFormat(ExpressionType* expression, 
                                                    FILE* inStream = stdin);
ExpressionErrors ExpressionReadInfixFormat (ExpressionType* expression, 
                                                    FILE* inStream = stdin);
ExpressionErrors ExpressionReadVariables(ExpressionType* expression);

#endif 