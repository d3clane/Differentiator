#ifndef MATH_EXPRESSION_EQUATION_READ
#define MATH_EXPRESSION_EQUATION_READ

#include "MathExpressionsMain.h"

union TokenValue
{
    char* word;
    double val;
};

enum class TokenValueType
{
    OPERATION,
    VARIABLE,
    VALUE,
};

struct TokenType
{
    TokenValue value;
    TokenValueType valueType;

    size_t line;
    size_t pos;
};

TokenType TokenCopy(const TokenType* token);
TokenType TokenCreate(TokenValue value, TokenValueType valueType,   const size_t line, 
                                                                    const size_t pos);

TokenValue TokenValueCreate(const char* word);
TokenValue TokenValueCreate(double value);

ExpressionType ExpressionParse(const char* str);

#endif