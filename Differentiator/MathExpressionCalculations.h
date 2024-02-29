#ifndef MATH_EXPRESSION_CALCULATIONS_H
#define MATH_EXPRESSION_CALCULATIONS_H

#include "MathExpressionsMain.h"

double ExpressionCalculate(const ExpressionType* expression);

ExpressionType ExpressionSubTwoExpressions(const ExpressionType* expr1, 
                                           const ExpressionType* expr2);

void ExpressionSimplify(ExpressionType* expression, FILE* outTex = nullptr);

ExpressionType ExpressionDifferentiate(const ExpressionType* expression,
                                               FILE* outTex = nullptr);

ExpressionType ExpressionTangent(ExpressionType* expression, const double x);
ExpressionType ExpressionTaylor (const ExpressionType* expression, const int n, const double x);

#endif