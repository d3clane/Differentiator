#include "DSL.h"

#define GENERATE_OPERATION_CMD(NAME, ...)                                                       \
    ExpressionTokenType* _##NAME(ExpressionTokenType* left,                                     \
                                    ExpressionTokenType* right)                                 \
    {                                                                                           \
        return ExpressionTokenCreate(ExpressionTokenValueСreate(ExpressionOperationId::NAME),   \
                                   ExpressionTokenValueTypeof::OPERATION,                       \
                                   left, right);                                                \
    }

#include "Operations.h"

#undef GENERATE_OPERATION_CMD 
