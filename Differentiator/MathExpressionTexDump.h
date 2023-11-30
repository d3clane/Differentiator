#ifndef MATH_EXPRESSION_TEX_DUMP
#define MATH_EXPRESSION_TEX_DUMP

#include "MathExpressionsMain.h"

void LatexFileTrollingStart(FILE* outTex);
void LatexFileTrollingEnd  (FILE* outTex);
void LatexCreatePdf(const char* fileName);
void LaTexStartNewSection(const char* sectionName, FILE* outStream);
void LaTexInsertImg(const char* imgName, FILE* outStream, const char* stringToPrint = nullptr);

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