#ifndef MATH_EXPRESSION_TEX_DUMP
#define MATH_EXPRESSION_TEX_DUMP

#include "MathExpressionsMain.h"

struct ExpressionLatexReplacementType
{
    const ExpressionTokenType* token;
    char* replacementStr;
};

typedef ExpressionLatexReplacementType LatexReplacementType;

struct ExpressionLatexReplacementArrayType
{
    LatexReplacementType* data;

    size_t size;
    size_t capacity;
};

typedef ExpressionLatexReplacementArrayType LatexReplacementArrType;

void LatexFileTrollingStart(FILE* outTex);
void LatexFileTrollingEnd  (FILE* outTex);
void LatexCreatePdf(const char* fileName);
void LaTexStartNewSection(const char* sectionName, FILE* outStream);
void LaTexInsertImg(const char* imgName, FILE* outStream, const char* stringToPrint = nullptr);

ExpressionErrors ExpressionPrintTex(const ExpressionType* expression,
                                    FILE* outStream,
                                    const char* funnyString,
                                    LatexReplacementArrType* replacementArr = nullptr);

ExpressionErrors ExpressionTokenPrintTexWithTrollString(
                                                const ExpressionTokenType* rootToken,
                                                FILE* outStream,
                                                const char* string,
                                                LatexReplacementArrType* replacementArr);

ExpressionErrors ExpressionTokenPrintTex(const ExpressionTokenType* token, 
                                         FILE* outStream,
                                         const LatexReplacementArrType* replacementArr);

ExpressionErrors ExpressionLatexReplacementPrint(LatexReplacementArrType* arr,
                                                 const ExpressionTokenType* token,
                                                 FILE* outStream);

size_t ExpressionLatexReplacementArrayInit(const ExpressionTokenType* token, 
                                            LatexReplacementArrType* arr);
                                            
void ExpressionLatexReplacementArrayDtor(LatexReplacementArrType* arr);
void ExpressionLatexReplacementArrayCtor(LatexReplacementArrType* arr, size_t capacity = 20);
#endif