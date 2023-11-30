#ifndef MATH_EXPRESSION_TEX_DUMP
#define MATH_EXPRESSION_TEX_DUMP

#include "MathExpressionsMain.h"

void LatexFileTrollingStart(FILE* outTex);
void LatexFileTrollingEnd  (FILE* outTex);
void LatexCreatePdf(const char* fileName);

ExpressionErrors ExpressionPrintTex(const ExpressionType* expression,
                                                          FILE* outStream = stdout, 
                                                          const char* funnyString = nullptr);

ExpressionErrors ExpressionTokenPrintTexWithTrollString(
                                                const ExpressionTokenType* rootToken,
                                                FILE* outStream,
                                                const char* string = nullptr);

ExpressionErrors ExpressionTokenPrintTex(const ExpressionTokenType* token, 
                                         FILE* outStream);

void TexInsertImg(const char* imgName, FILE* outStream, const char* stringToPrint = nullptr);

#endif