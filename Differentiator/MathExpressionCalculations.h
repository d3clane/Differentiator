#ifndef MATH_EXPRESSION_CALCULATIONS_H
#define MATH_EXPRESSION_CALCULATINS_h

#include "MathExpressionsHandler.h"

double ExpressionCalculate(const ExpressionType* expression);

ExpressionType ExpressionDifferentiate(const ExpressionType* expression,
                                               FILE* outTex = nullptr);

void ExpressionSimplify(ExpressionType* expression, FILE* outTex = nullptr);

#endif