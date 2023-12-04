#ifndef DSL_H
#define DSL_H

#include "MathExpressionsMain.h"

#define C(TOKEN) ExpressionTokenCopy(TOKEN)    

#define  OP_TYPE_CNST ExpressionTokenValueTypeof::OPERATION
#define VAR_TYPE_CNST ExpressionTokenValueTypeof::VARIABLE
#define VAL_TYPE_CNST ExpressionTokenValueTypeof::VALUE
#define TOKEN_OP(token) token->value.operation

#define VAL_TYPE(token) token->valueType
#define VAL(token)      token->value.value
#define VAR(token)      token->value.varPtr
#define  OP(token)      token->value.operation
#define   L(token)      token->left
#define   R(token)      token->right

#define L_VAL(token) token->left->value.value
#define R_VAL(token) token->right->value.value
#define L_VAR(token) token->left->value.varPtr
#define R_VAR(token) token->right->value.varPtr
#define  L_OP(token) token->left->value.operation
#define  R_OP(token) token->right->value.operation

#define   IS_VAL(token) (token->valueType        == VAL_TYPE_CNST)
#define L_IS_VAL(token) (token->left->valueType  == VAL_TYPE_CNST)
#define R_IS_VAL(token) (token->right->valueType == VAL_TYPE_CNST)
#define   IS_VAR(token) (token->valueType        == VAR_TYPE_CNST)
#define L_IS_VAR(token) (token->left->valueType  == VAR_TYPE_CNST)
#define R_IS_VAR(token) (token->right->valueType == VAR_TYPE_CNST)
#define    IS_OP(token) (token->valueType        ==  OP_TYPE_CNST)
#define  L_IS_OP(token) (token->left->valueType  ==  OP_TYPE_CNST)
#define  R_IS_OP(token) (token->right->valueType ==  OP_TYPE_CNST)

#define CRT_NUM(VALUE)    ExpressionNumericTokenCreate(VALUE)
#define CRT_VAR(VARS_ARR, VAR_NAME) ExpressionVariableTokenCreate(VARS_ARR, VAR_NAME)

#define GENERATE_OPERATION_CMD(NAME, ...)                                                       \
    ExpressionTokenType* _##NAME(ExpressionTokenType* left,                                     \
                                               ExpressionTokenType* right = nullptr);

#include "Operations.h"

#undef GENERATE_OPERATION_CMD

#endif