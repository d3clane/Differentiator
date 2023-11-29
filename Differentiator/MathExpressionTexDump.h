#ifndef MATH_EXPRESSION_TEX_DUMP
#define MATH_EXPRESSION_TEX_DUMP

#include "MathExpressionsMain.h"

ExpressionErrors ExpressionPrintTex(const ExpressionType* expression,
                                                          FILE* outStream = stdout, 
                                                          const char* funnyString = nullptr);

ExpressionErrors ExpressionTokenPrintTexWithTrollString(
                                                const ExpressionTokenType* rootToken,
                                                FILE* outStream,
                                                const char* string = nullptr);

ExpressionErrors ExpressionTokenPrintTex(const ExpressionTokenType* token, 
                                         FILE* outStream);

#endif