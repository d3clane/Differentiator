#ifndef MATH_EXPRESSION_CALCULATIONS_H
#define MATH_EXPRESSION_CALCULATINS_h

#include "MathExpressionsMain.h"

ExpressionType ExpressionDifferentiate(const ExpressionType* expression,
                                               FILE* outTex = nullptr);

void ExpressionSimplify(ExpressionType* expression, FILE* outTex = nullptr);

#endif