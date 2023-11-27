#include <assert.h>
#include <math.h>

#include "MathExpressionCalculations.h"
#include "MathExpressionInOut.h"
#include "Common/DoubleFuncs.h"

//---------------------------------------------------------------------------------------

#define GENERATE_OPERATION_CMD(NAME, v1, v2, v3, v4, v5, v6, v7, CALCULATION_CODE, ...)
    
//--------------------DSL-----------------------------

#define D(TOKEN) ExpressionDifferentiate(TOKEN, outTex)
#define C(TOKEN) ExpressionTokenCopy(TOKEN)

#define CONST_TOKEN(VALUE) ExpressionNumericTokenCreate(VALUE)

#define TOKEN(OPERATION_NAME, LEFT_TOKEN, RIGHT_TOKEN)                                        \
    ExpressionTokenCtor(ExpressionTokenValueСreate(                                           \
                                            ExpressionOperationsIds::OPERATION_NAME),         \
                            ExpressionTokenValueTypeof::OPERATION,                            \
                            LEFT_TOKEN, RIGHT_TOKEN)                                               

#define UNARY_TOKEN(OPERATION_NAME, LEFT_TOKEN) TOKEN(OPERATION_NAME, LEFT_TOKEN, nullptr)

//-------------------Differentiate---------------
static ExpressionTokenType* ExpressionDifferentiate(
                                        const ExpressionTokenType* token,
                                        FILE* outTex = nullptr);

#define GENERATE_OPERATION_CMD(NAME, v1, v2, v3, v4, v5, v6, v7, v8, DIFF_CODE, ...)      \
    static inline ExpressionTokenType* ExpressionDifferentiate##NAME(                    \
                                                const ExpressionTokenType* token,       \
                                                FILE* outTex = nullptr)                 \
    {                                                                                   \
        DIFF_CODE                                                                       \
    }

//Creating funcs ExpressionDifferentiateADD, ...
#include "Operations.h"

#undef  GENERATE_OPERATION_CMD
#define GENERATE_OPERATION_CMD(NAME, ...) ExpressionDifferentiate##NAME,

typedef ExpressionTokenType* (DiffFuncType)(const ExpressionTokenType* token,
                                            FILE* outTex);

static const DiffFuncType* const OperationsDiffFuncs[] =
{
    #include "Operations.h"
};

static const size_t NumberOfOperations = sizeof(OperationsDiffFuncs) / sizeof(*OperationsDiffFuncs);

#undef GENERATE_OPERATION_CMD

static inline DiffFuncType* ExpressionOperationGetDiffFunc(const ExpressionOperationsIds operationId);

//--------------------------------Simplify-------------------------------------------

static ExpressionTokenType* ExpressionSimplifyConstants (ExpressionTokenType* token,
                                                                 int* simplifiesCount,
                                                                 bool* haveVariables = nullptr);

static ExpressionTokenType* ExpressionSimplifyNeutralTokens(ExpressionTokenType* token, 
                                                                    int* simplifiesCount);

static inline ExpressionTokenType* ExpressionSimplifyAdd(ExpressionTokenType* token,   
                                                                 ExpressionTokenType* left,
                                                                 ExpressionTokenType* right,
                                                                 int* simplifiesCount);
static inline ExpressionTokenType* ExpressionSimplifySub(ExpressionTokenType* token,   
                                                          ExpressionTokenType* left,
                                                          ExpressionTokenType* right,
                                                          int* simplifiesCount);
static inline ExpressionTokenType* ExpressionSimplifyMul(ExpressionTokenType* token,   
                                                          ExpressionTokenType* left,
                                                          ExpressionTokenType* right,
                                                          int* simplifiesCount);
static inline ExpressionTokenType* ExpressionSimplifyDiv(ExpressionTokenType* token,   
                                                          ExpressionTokenType* left,
                                                          ExpressionTokenType* right,
                                                          int* simplifiesCount);
static inline ExpressionTokenType* ExpressionSimplifyPow(ExpressionTokenType* token,   
                                                          ExpressionTokenType* left,
                                                          ExpressionTokenType* right,
                                                          int* simplifiesCount);
static inline ExpressionTokenType* ExpressionSimplifyLog(ExpressionTokenType* token,   
                                                          ExpressionTokenType* left,
                                                          ExpressionTokenType* right,
                                                          int* simplifiesCount);

static inline ExpressionTokenType* ExpressionSimplifyReturnLeftToken(
                                                                ExpressionTokenType* token,
                                                                ExpressionTokenType* left);
static inline ExpressionTokenType* ExpressionSimplifyReturnRightToken(
                                                                ExpressionTokenType* token,
                                                                ExpressionTokenType* right);
static inline ExpressionTokenType* ExpressionSimplifyReturnConstToken(
                                                                ExpressionTokenType* token,
                                                                double value);

//---------------------------------------------------------------------------------------

static bool ExpressionTokenContainVariable(const ExpressionTokenType* token);

//---------------------------------------------------------------------------------------

ExpressionType ExpressionDifferentiate(const ExpressionType* expression,
                                       FILE* outTex)
{
    assert(expression);
    
    if (outTex)
        ExpressionPrintTex(expression, outTex, 
                                "Ящеры нападают, нужно срочно взять эту производную ради победы");

    ExpressionTokenType* diffRootToken = ExpressionDifferentiate(expression->root, 
                                                                 outTex);
    ExpressionType diffExpression = {};
    ExpressionCtor(&diffExpression);

    diffExpression.root = diffRootToken;

    ExpressionsCopyVariables(&diffExpression, expression);

    fprintf(outTex, "***** не понятно, но очень интересно. Сделаем выражение более понятным\n");

    ExpressionSimplify(&diffExpression, outTex);

    return diffExpression;
}

static ExpressionTokenType* ExpressionDifferentiate(
                                                    const ExpressionTokenType* token,
                                                    FILE* outTex)
{
    assert(token);

    ExpressionTokenValue val = {};
    ExpressionTokenType* diffToken = nullptr;

    switch(token->valueType)
    {
        case ExpressionTokenValueTypeof::VALUE:
            val.value = 0;
            diffToken =  ExpressionTokenCtor(val, ExpressionTokenValueTypeof::VALUE);
            break;
        case ExpressionTokenValueTypeof::VARIABLE:
            val.value = 1;
            diffToken =  ExpressionTokenCtor(val, ExpressionTokenValueTypeof::VALUE);
            break;

        case ExpressionTokenValueTypeof::OPERATION:
            //TODO: отдельно создать функцию будет гораздо читабельнее
            diffToken = ExpressionOperationGetDiffFunc(token->value.operation.operationId)(token,
                                                                                           outTex);
            break;

        default:
            break;
    }

    if (outTex) 
    {
        ExpressionTokenPrintTexTrollString(token, outTex, "Возьмем производную от:");
        ExpressionTokenPrintTexTrollString(diffToken, outTex);
    }

    return diffToken;  
}

//---------------------------------------------------------------------------------------

static inline DiffFuncType* ExpressionOperationGetDiffFunc(const ExpressionOperationsIds operationId)
{
    assert((int)operationId >= 0);
    assert((size_t)operationId < NumberOfOperations);

    return OperationsDiffFuncs[(size_t)operationId];
}

//---------------------------------------------------------------------------------------

void ExpressionSimplify(ExpressionType* expression,
                            FILE* outTex)
{
    assert(expression);

    int simplifiesCount = 0;
    do
    {
        simplifiesCount = 0;
        expression->root = ExpressionSimplifyConstants(expression->root, &simplifiesCount);
        expression->root = ExpressionSimplifyNeutralTokens(expression->root, 
                                                               &simplifiesCount);
    } while (simplifiesCount != 0);

}

static ExpressionTokenType* ExpressionSimplifyConstants (ExpressionTokenType* token,
                                                                 int* simplifiesCount,
                                                                 bool* haveVariables)
{
    assert(simplifiesCount);
    if (token == nullptr)
    {
        assert(haveVariables != nullptr);
        
        *haveVariables = false;
        
        return nullptr;
    }

    if (token->valueType == ExpressionTokenValueTypeof::VALUE)
    {
        assert(haveVariables != nullptr);

        *haveVariables = false;

        return token;
    }

    if (token->valueType == ExpressionTokenValueTypeof::VARIABLE)
    {
        assert(haveVariables != nullptr);

        *haveVariables = true;

        return token;
    }

    bool haveLeftTokenVariables  = false;
    bool haveRightTokenVariables = false;
    ExpressionTokenType* left  = ExpressionSimplifyConstants(token->left,  
                                                                     simplifiesCount,
                                                                     &haveLeftTokenVariables);
    ExpressionTokenType* right = ExpressionSimplifyConstants(token->right, 
                                                                     simplifiesCount,
                                                                     &haveRightTokenVariables);

    if (token->left != left)
    {
        ExpressionTokenDtor(token->left);
        token->left = left;
    }

    if (token->right != right)
    {
        ExpressionTokenDtor(token->right);
        token->right = right;
    }

    bool haveTokenVariables = haveLeftTokenVariables | haveRightTokenVariables;

    if (haveVariables != nullptr) *haveVariables = haveTokenVariables;

    if (!haveTokenVariables)
    {
        (*simplifiesCount)++;
        
        double leftVal = NAN, rightVal = NAN;
        assert(token->left);

        leftVal = token->left->value.value;
        if (token->right) rightVal = token->right->value.value;

        return CONST_TOKEN(token->value.operation.CalculationFunc(leftVal, rightVal));
    }
    return token;
}

//---------------------------------------------------------------------------------------

static ExpressionTokenType* ExpressionSimplifyNeutralTokens(ExpressionTokenType* token, 
                                                                    int* simplifiesCount)
{
    if (token == nullptr || token->valueType != ExpressionTokenValueTypeof::OPERATION)
        return token;
    
    ExpressionTokenType* left  = ExpressionSimplifyNeutralTokens(token->left,  
                                                                        simplifiesCount);
    ExpressionTokenType* right = ExpressionSimplifyNeutralTokens(token->right, 
                                                                        simplifiesCount);
    
    if (token->left != left)
    {
        ExpressionTokenDtor(token->left);
        token->left = left;
    }

    if (token->right != right)
    {
        ExpressionTokenDtor(token->right);
        token->right = right;
    }

    if (left == nullptr || right == nullptr)
        return token;
    
    assert(token->valueType == ExpressionTokenValueTypeof::OPERATION);

    if (((right->valueType == ExpressionTokenValueTypeof::VARIABLE)  &&
         (left->valueType  == ExpressionTokenValueTypeof::VARIABLE)) &&
        (right->value.varPtr == left->value.varPtr))
        return ExpressionSimplifyReturnConstToken(token, 0);

    bool rightIsValue = (right->valueType == ExpressionTokenValueTypeof::VALUE);
    bool leftIsValue  = (left->valueType  == ExpressionTokenValueTypeof::VALUE);

    if (!leftIsValue && !rightIsValue)
        return token;

    switch (token->value.operation.operationId)
    {
        case ExpressionOperationsIds::ADD:
            return ExpressionSimplifyAdd(token, left, right, simplifiesCount);
        case ExpressionOperationsIds::SUB:
            return ExpressionSimplifySub(token, left, right, simplifiesCount);
        case ExpressionOperationsIds::MUL:
            return ExpressionSimplifyMul(token, left, right, simplifiesCount);
        case ExpressionOperationsIds::DIV:
            return ExpressionSimplifyDiv(token, left, right, simplifiesCount);
        
        case ExpressionOperationsIds::POW:
            return ExpressionSimplifyPow(token, left, right, simplifiesCount);
        case ExpressionOperationsIds::LOG:
            return ExpressionSimplifyLog(token, left, right, simplifiesCount);
        
        default:
            break;
    }

    return token;
}

//---------------------------------------------------------------------------------------

static inline ExpressionTokenType* ExpressionSimplifyAdd(ExpressionTokenType* token,   
                                                                 ExpressionTokenType* left,
                                                                 ExpressionTokenType* right,
                                                                 int* simplifiesCount)
{
    assert(simplifiesCount);

    bool rightIsValue = (right->valueType == ExpressionTokenValueTypeof::VALUE);
    bool leftIsValue  = (left->valueType  == ExpressionTokenValueTypeof::VALUE);

    if (rightIsValue && DoubleEqual(right->value.value, 0))
    {
        (*simplifiesCount)++;
        return ExpressionSimplifyReturnLeftToken(token, left);
    }

    if (leftIsValue && DoubleEqual(left->value.value, 0))
    {
        (*simplifiesCount)++;
        return ExpressionSimplifyReturnRightToken(token, right);
    }

    return token;
}

static inline ExpressionTokenType* ExpressionSimplifySub(ExpressionTokenType* token,   
                                                          ExpressionTokenType* left,
                                                          ExpressionTokenType* right,
                                                          int* simplifiesCount)
{
    bool rightIsValue = (right->valueType == ExpressionTokenValueTypeof::VALUE);
    bool leftIsValue  = (left->valueType  == ExpressionTokenValueTypeof::VALUE);

    if (rightIsValue && DoubleEqual(right->value.value, 0))
    {    
        (*simplifiesCount)++;
        return ExpressionSimplifyReturnLeftToken(token, left);
    }

    if (leftIsValue && DoubleEqual(left->value.value, 0))
    {
        (*simplifiesCount)++;

        ExpressionTokenDtor(token->left);
        token->left = nullptr;

        ExpressionTokenType* simpledToken = TOKEN(MUL, CONST_TOKEN(-1), token->right);
        token->right = nullptr;

        return simpledToken;
    }

    return token;
}

static inline ExpressionTokenType* ExpressionSimplifyMul(ExpressionTokenType* token,   
                                                          ExpressionTokenType* left,
                                                          ExpressionTokenType* right,
                                                          int* simplifiesCount)
{
    bool rightIsValue = (right->valueType == ExpressionTokenValueTypeof::VALUE);
    bool leftIsValue  = (left->valueType  == ExpressionTokenValueTypeof::VALUE);

    if (rightIsValue && DoubleEqual(right->value.value, 0))
    {
        (*simplifiesCount)++;
        return ExpressionSimplifyReturnConstToken(token, 0);
    }

    if (leftIsValue && DoubleEqual(left->value.value, 0))
    {
        (*simplifiesCount)++;
        return ExpressionSimplifyReturnConstToken(token, 0);
    }

    if (rightIsValue && DoubleEqual(right->value.value, 1))
    {
        (*simplifiesCount)++;
        return ExpressionSimplifyReturnLeftToken(token, left);
    }

    if (leftIsValue && DoubleEqual(left->value.value, 1))
    {
        (*simplifiesCount)++;
        return ExpressionSimplifyReturnRightToken(token, right);
    }

    return token;
}

static inline ExpressionTokenType* ExpressionSimplifyDiv(ExpressionTokenType* token,   
                                                          ExpressionTokenType* left,
                                                          ExpressionTokenType* right,
                                                          int* simplifiesCount)
{
    bool rightIsValue = (right->valueType == ExpressionTokenValueTypeof::VALUE);
    bool leftIsValue  = (left->valueType  == ExpressionTokenValueTypeof::VALUE);

    if (leftIsValue && DoubleEqual(left->value.value, 0))
    {
        (*simplifiesCount)++;
        return ExpressionSimplifyReturnConstToken(token, 0);
    }

    if (rightIsValue && DoubleEqual(right->value.value, 1))
    {
        (*simplifiesCount)++;
        return ExpressionSimplifyReturnLeftToken(token, left);
    }

    return token;
}

static inline ExpressionTokenType* ExpressionSimplifyPow(ExpressionTokenType* token,   
                                                          ExpressionTokenType* left,
                                                          ExpressionTokenType* right,
                                                          int* simplifiesCount)
{
    bool rightIsValue = (right->valueType == ExpressionTokenValueTypeof::VALUE);
    bool leftIsValue  = (left->valueType  == ExpressionTokenValueTypeof::VALUE);

    if (rightIsValue && DoubleEqual(right->value.value, 0))
    {
        (*simplifiesCount)++;
        return ExpressionSimplifyReturnConstToken(token, 1);
    }

    if (leftIsValue && DoubleEqual(left->value.value, 0))
    {
        (*simplifiesCount)++;
        return ExpressionSimplifyReturnConstToken(token, 0);
    }

    if (rightIsValue && DoubleEqual(right->value.value, 1))
    {
        (*simplifiesCount)++;
        return ExpressionSimplifyReturnLeftToken(token, left);
    }

    if (leftIsValue && DoubleEqual(left->value.value, 1))
    {
        (*simplifiesCount)++;
        return ExpressionSimplifyReturnConstToken(token, 1);
    }

    return token;
}

static inline ExpressionTokenType* ExpressionSimplifyLog(ExpressionTokenType* token,   
                                                          ExpressionTokenType* left,
                                                          ExpressionTokenType* right,
                                                          int* simplifiesCount)
{
    bool rightIsValue = (right->valueType == ExpressionTokenValueTypeof::VALUE);

    if (rightIsValue && DoubleEqual(right->value.value, 1))
    {
        (*simplifiesCount)++;
        return ExpressionSimplifyReturnConstToken(token, 0);
    }

    return token;
}

//---------------------------------------------------------------------------------------

static inline ExpressionTokenType* ExpressionSimplifyReturnLeftToken(
                                                                ExpressionTokenType* token,
                                                                ExpressionTokenType* left)
{
    ExpressionTokenDtor(token->right);

    assert(token->left == left);

    token->left  = nullptr;
    token->right = nullptr;

    return left;
}

static inline ExpressionTokenType* ExpressionSimplifyReturnRightToken(
                                                                ExpressionTokenType* token,
                                                                ExpressionTokenType* right)
{
    ExpressionTokenDtor(token->left);

    assert(token->right == right);
    
    token->left  = nullptr;
    token->right = nullptr;

    return right;
}

static inline ExpressionTokenType* ExpressionSimplifyReturnConstToken(
                                                                ExpressionTokenType* token,
                                                                double value)
{
    ExpressionTokenDtor(token->right);
    ExpressionTokenDtor(token->left);

    token->left  = nullptr;
    token->right = nullptr;

    return CONST_TOKEN(value);
}

//---------------------------------------------------------------------------------------

#undef C
#undef D
#undef TOKEN
#undef CONST_TOKEN
#undef UNARY_TOKEN

//---------------------------------------------------------------------------------------

static bool ExpressionTokenContainVariable(const ExpressionTokenType* token)
{
    if (token == nullptr)
        return false;
    
    if (token->valueType == ExpressionTokenValueTypeof::VARIABLE)
        return true;

    bool containVariable  = ExpressionTokenContainVariable(token->left);

    if (containVariable)
        return containVariable;
    
    containVariable = ExpressionTokenContainVariable(token->right);

    return containVariable;
}