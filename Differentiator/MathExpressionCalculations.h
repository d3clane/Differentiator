#ifndef MATH_EXPRESSION_CALCULATIONS_H
#define MATH_EXPRESSION_CALCULATINS_h

#include "MathExpressionsMain.h"

double ExpressionCalculate(const ExpressionType* expression);

ExpressionType ExpressionSubTwoExpressions(const ExpressionType* expr1, 
                                           const ExpressionType* expr2);

void ExpressionSimplify(ExpressionType* expression, FILE* outTex = nullptr);

ExpressionType ExpressionDifferentiate(const ExpressionType* expression,
                                               FILE* outTex = nullptr);

ExpressionType ExpressionMacloren(const ExpressionType* expression, const int n);

#endif