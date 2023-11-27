#include <assert.h>
#include <math.h>

#include "MathExpressionCalculations.h"
#include "MathExpressionInOut.h"
#include "Common/DoubleFuncs.h"

//---------------------------------------------------------------------------------------

static double ExpressionCalculate(const ExpressionTokenType* token, 
                                      const ExpressionVariablesArrayType* varsArr);

//---------------------------------------------------------------------------------------

static ExpressionTokenType* ExpressionDifferentiate(
                                        const ExpressionTokenType* token,
                                        const ExpressionVariablesArrayType* varsArr = nullptr,
                                        FILE* outTex = nullptr);

static inline ExpressionTokenType* ExpressionDifferentiateADD(
                                        const ExpressionTokenType* token,
                                        const ExpressionVariablesArrayType* varsArr = nullptr,
                                        FILE* outTex = nullptr);
static inline ExpressionTokenType* ExpressionDifferentiateSUB(
                                        const ExpressionTokenType* token,
                                        const ExpressionVariablesArrayType* varsArr = nullptr,
                                        FILE* outTex = nullptr);
static inline ExpressionTokenType* ExpressionDifferentiateMUL(
                                        const ExpressionTokenType* token,
                                        const ExpressionVariablesArrayType* varsArr = nullptr,
                                        FILE* outTex = nullptr);
static inline ExpressionTokenType* ExpressionDifferentiateDIV(
                                        const ExpressionTokenType* token,
                                        const ExpressionVariablesArrayType* varsArr = nullptr,
                                        FILE* outTex = nullptr);
static inline ExpressionTokenType* ExpressionDifferentiateSIN(
                                        const ExpressionTokenType* token,
                                        const ExpressionVariablesArrayType* varsArr = nullptr,
                                        FILE* outTex = nullptr);
static inline ExpressionTokenType* ExpressionDifferentiateCOS(
                                        const ExpressionTokenType* token,
                                        const ExpressionVariablesArrayType* varsArr = nullptr,
                                        FILE* outTex = nullptr);
static inline ExpressionTokenType* ExpressionDifferentiateTAN(
                                        const ExpressionTokenType* token,
                                        const ExpressionVariablesArrayType* varsArr = nullptr,
                                        FILE* outTex = nullptr);
static inline ExpressionTokenType* ExpressionDifferentiateCOT(
                                        const ExpressionTokenType* token,
                                        const ExpressionVariablesArrayType* varsArr = nullptr,
                                        FILE* outTex = nullptr);
static inline ExpressionTokenType* ExpressionDifferentiateARCSIN(
                                        const ExpressionTokenType* token,
                                        const ExpressionVariablesArrayType* varsArr = nullptr,
                                        FILE* outTex = nullptr);
static inline ExpressionTokenType* ExpressionDifferentiateARCCOS(
                                        const ExpressionTokenType* token,
                                        const ExpressionVariablesArrayType* varsArr = nullptr,
                                        FILE* outTex = nullptr);
static inline ExpressionTokenType* ExpressionDifferentiateARCTAN(
                                        const ExpressionTokenType* token,
                                        const ExpressionVariablesArrayType* varsArr = nullptr,
                                        FILE* outTex = nullptr);
static inline ExpressionTokenType* ExpressionDifferentiateARCCOT(
                                        const ExpressionTokenType* token,
                                        const ExpressionVariablesArrayType* varsArr = nullptr,
                                        FILE* outTex = nullptr);

static inline ExpressionTokenType* ExpressionDifferentiateLN(
                                        const ExpressionTokenType* token,
                                        const ExpressionVariablesArrayType* varsArr = nullptr,
                                        FILE* outTex = nullptr);
static inline ExpressionTokenType* ExpressionDifferentiateLOG(
                                        const ExpressionTokenType* token,
                                        const ExpressionVariablesArrayType* varsArr = nullptr,
                                        FILE* outTex = nullptr);
static inline ExpressionTokenType* ExpressionDifferentiatePOW(
                                        const ExpressionTokenType* token,
                                        const ExpressionVariablesArrayType* varsArr = nullptr,
                                        FILE* outTex = nullptr);

#define GENERATE_OPERATION_CMD(NAME, ...) ExpressionDifferentiate##NAME,

typedef ExpressionTokenType* (DiffFuncType)(const ExpressionTokenType* token,
                                                const ExpressionVariablesArrayType* varsArr,
                                                FILE* outTex);

static const DiffFuncType* const OperationsDiff[] =
{
    #include "Operations.h"
};

static const size_t NumberOfOperations = sizeof(OperationsDiff) / sizeof(*OperationsDiff);

#undef GENERATE_OPERATION_CMD

static inline DiffFuncType* GetOperationDiffFunc(const ExpressionOperationsIds operationId);

//---------------------------------------------------------------------------------------

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


double ExpressionCalculate(const ExpressionType* expression)
{
    assert(expression);

    return ExpressionCalculate(expression->root, &expression->variables);
}

//---------------------------------------------------------------------------------------

static double ExpressionCalculate(const ExpressionTokenType* token, 
                                      const ExpressionVariablesArrayType* varsArr)
{
    if (token == nullptr)
        return NAN;
    
    if (token->valueType == ExpressionTokenValueTypeof::VALUE)
        return token->value.value;

    if (token->valueType == ExpressionTokenValueTypeof::VARIABLE)
        return token->value.varPtr->variableValue;

    double firstVal  = ExpressionCalculate(token->left,  varsArr);
    double secondVal = ExpressionCalculate(token->right, varsArr);
    
    return token->value.operation.CalculationFunc(firstVal, secondVal);
}

//---------------------------------------------------------------------------------------

ExpressionType ExpressionDifferentiate(const ExpressionType* expression,
                                       FILE* outTex)
{
    assert(expression);
    
    if (outTex)
        ExpressionPrintTex(expression, outTex, 
                                "Ящеры нападают, нужно срочно взять эту производную ради победы");

    ExpressionTokenType* diffRootToken = ExpressionDifferentiate(expression->root, 
                                                                 &expression->variables,
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
                                                    const ExpressionVariablesArrayType* varsArr,
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
            diffToken = GetOperationDiffFunc(token->value.operation.operationId)(token,
                                                                                 varsArr,
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

#define D(TOKEN) ExpressionDifferentiate(TOKEN, varsArr, outTex)
#define C(TOKEN) ExpressionTokenCopy(TOKEN)

#define CONST_TOKEN(VALUE) ExpressionCreateNumericToken(VALUE)

#define TOKEN(OPERATION_NAME, LEFT_TOKEN, RIGHT_TOKEN)                                        \
    ExpressionTokenCtor(ExpressionCreateTokenValue(                                   \
                                            ExpressionOperationsIds::OPERATION_NAME),   \
                            ExpressionTokenValueTypeof::OPERATION,                        \
                            LEFT_TOKEN, RIGHT_TOKEN)                                               

#define UNARY_TOKEN(OPERATION_NAME, LEFT_TOKEN) TOKEN(OPERATION_NAME, LEFT_TOKEN, nullptr)

static inline ExpressionTokenType* ExpressionDifferentiateADD(
                                        const ExpressionTokenType* token,
                                        const ExpressionVariablesArrayType* varsArr,
                                        FILE* outTex)
{
    assert(token);
    assert(token->valueType == ExpressionTokenValueTypeof::OPERATION);
    assert(token->value.operation.operationId == ExpressionOperationsIds::ADD);

    return TOKEN(ADD, D(token->left), D(token->right));
}

static inline ExpressionTokenType* ExpressionDifferentiateSUB(
                                        const ExpressionTokenType* token,
                                        const ExpressionVariablesArrayType* varsArr,
                                        FILE* outTex)
{
    assert(token);
    assert(token->valueType == ExpressionTokenValueTypeof::OPERATION);
    assert(token->value.operation.operationId == ExpressionOperationsIds::SUB);

    return TOKEN(SUB, D(token->left), D(token->right));
}

static inline ExpressionTokenType* ExpressionDifferentiateMUL(
                                        const ExpressionTokenType* token,
                                        const ExpressionVariablesArrayType* varsArr,
                                        FILE* outTex)
{
    assert(token);
    assert(token->valueType == ExpressionTokenValueTypeof::OPERATION);
    assert(token->value.operation.operationId == ExpressionOperationsIds::MUL);

    return TOKEN(ADD, TOKEN(MUL, D(token->left), C(token->right)), 
                      TOKEN(MUL, C(token->left), D(token->right)));
}

static inline ExpressionTokenType* ExpressionDifferentiateDIV(
                                        const ExpressionTokenType* token,
                                        const ExpressionVariablesArrayType* varsArr,
                                        FILE* outTex)
{
    assert(token);
    assert(token->valueType == ExpressionTokenValueTypeof::OPERATION);
    assert(token->value.operation.operationId == ExpressionOperationsIds::DIV);

    return TOKEN(DIV, TOKEN(SUB, TOKEN(MUL, D(token->left), C(token->right)), 
                                 TOKEN(MUL, C(token->left), D(token->right))),
                      TOKEN(POW, C(token->right), CONST_TOKEN(2)));
}

static inline ExpressionTokenType* ExpressionDifferentiatePOW(
                                        const ExpressionTokenType* token,
                                        const ExpressionVariablesArrayType* varsArr,
                                        FILE* outTex)
{
    assert(token);
    assert(token->valueType == ExpressionTokenValueTypeof::OPERATION);
    assert(token->value.operation.operationId == ExpressionOperationsIds::POW);

    bool baseContainVar  = ExpressionTokenContainVariable(token->left);
    bool powerContainVar = ExpressionTokenContainVariable(token->right);

    if (!baseContainVar && !powerContainVar)
        return CONST_TOKEN(0);

    if (baseContainVar && !powerContainVar)
        return TOKEN(MUL, TOKEN(MUL, C(token->right), D(token->left)), 
                          TOKEN(POW, C(token->left), 
                                     TOKEN(SUB, C(token->right), CONST_TOKEN(1))));
                        
    if (!baseContainVar && powerContainVar)
        return TOKEN(MUL, TOKEN(POW, C(token->left), C(token->right)),
                          TOKEN(MUL, UNARY_TOKEN(LN, C(token->left)),
                                     D(token->right)));

    return TOKEN(MUL, TOKEN(POW, C(token->left), C(token->right)),
                      TOKEN(ADD, TOKEN(MUL, C(token->right),
                                            TOKEN(DIV, D(token->left), C(token->left))),
                                 TOKEN(MUL, UNARY_TOKEN(LN, C(token->left)), D(token->right))));
}

static inline ExpressionTokenType* ExpressionDifferentiateLN(
                                        const ExpressionTokenType* token,
                                        const ExpressionVariablesArrayType* varsArr,
                                        FILE* outTex)
{
    assert(token);
    assert(token->valueType == ExpressionTokenValueTypeof::OPERATION);
    assert(token->value.operation.operationId == ExpressionOperationsIds::LN);

    return TOKEN(DIV, D(token->left), C(token->left));
}

static inline ExpressionTokenType* ExpressionDifferentiateLOG(
                                        const ExpressionTokenType* token,
                                        const ExpressionVariablesArrayType* varsArr,
                                        FILE* outTex)
{
    assert(token);
    assert(token->valueType == ExpressionTokenValueTypeof::OPERATION);
    assert(token->value.operation.operationId == ExpressionOperationsIds::LN);


    return TOKEN(DIV, D(token->left), C(token->left));
}

static inline ExpressionTokenType* ExpressionDifferentiateSIN(
                                        const ExpressionTokenType* token,
                                        const ExpressionVariablesArrayType* varsArr,
                                        FILE* outTex)
{
    assert(token);
    assert(token->valueType == ExpressionTokenValueTypeof::OPERATION);
    assert(token->value.operation.operationId == ExpressionOperationsIds::SIN);

    return TOKEN(MUL, UNARY_TOKEN(COS, C(token->left)), D(token->left));
}

static inline ExpressionTokenType* ExpressionDifferentiateCOS(
                                        const ExpressionTokenType* token,
                                        const ExpressionVariablesArrayType* varsArr,
                                        FILE* outTex)
{
    assert(token);
    assert(token->valueType == ExpressionTokenValueTypeof::OPERATION);
    assert(token->value.operation.operationId == ExpressionOperationsIds::COS);

    return TOKEN(MUL, CONST_TOKEN(-1), 
                      TOKEN(MUL, UNARY_TOKEN(SIN, C(token->left)), D(token->left)));
}

static inline ExpressionTokenType* ExpressionDifferentiateTAN(
                                        const ExpressionTokenType* token,
                                        const ExpressionVariablesArrayType* varsArr,
                                        FILE* outTex)
{
    assert(token);
    assert(token->valueType == ExpressionTokenValueTypeof::OPERATION);
    assert(token->value.operation.operationId == ExpressionOperationsIds::TAN);

    return TOKEN(DIV, D(token->left), 
                      TOKEN(POW, UNARY_TOKEN(COS, C(token->left)), CONST_TOKEN(2)));
}

static inline ExpressionTokenType* ExpressionDifferentiateCOT(
                                        const ExpressionTokenType* token,
                                        const ExpressionVariablesArrayType* varsArr,
                                        FILE* outTex)
{
    assert(token);
    assert(token->valueType == ExpressionTokenValueTypeof::OPERATION);
    assert(token->value.operation.operationId == ExpressionOperationsIds::COT);

    return TOKEN(MUL, CONST_TOKEN(-1), 
                      TOKEN(DIV, D(token->left), 
                                 TOKEN(POW, UNARY_TOKEN(SIN, C(token->left)), CONST_TOKEN(2))));
} 

static inline ExpressionTokenType* ExpressionDifferentiateARCSIN(
                                        const ExpressionTokenType* token,
                                        const ExpressionVariablesArrayType* varsArr,
                                        FILE* outTex)
{
    assert(token);
    assert(token->valueType == ExpressionTokenValueTypeof::OPERATION);
    assert(token->value.operation.operationId == ExpressionOperationsIds::ARCSIN);

    return TOKEN(DIV, D(token->left),
                      TOKEN(POW, TOKEN(SUB, CONST_TOKEN(1), 
                                            TOKEN(POW, C(token->left), CONST_TOKEN(2))),
                                 CONST_TOKEN(0.5)));
} 

static inline ExpressionTokenType* ExpressionDifferentiateARCCOS(
                                        const ExpressionTokenType* token,
                                        const ExpressionVariablesArrayType* varsArr,
                                        FILE* outTex)
{
    assert(token);
    assert(token->valueType == ExpressionTokenValueTypeof::OPERATION);
    assert(token->value.operation.operationId == ExpressionOperationsIds::ARCCOS);

    return TOKEN(DIV, CONST_TOKEN(-1),
                      TOKEN(MUL, D(token->left),
                                 TOKEN(POW, TOKEN(SUB, CONST_TOKEN(1), 
                                                       TOKEN(POW, C(token->left), CONST_TOKEN(2))),
                                            CONST_TOKEN(0.5))));
}

static inline ExpressionTokenType* ExpressionDifferentiateARCTAN(
                                        const ExpressionTokenType* token,
                                        const ExpressionVariablesArrayType* varsArr,
                                        FILE* outTex)
{
    assert(token);
    assert(token->valueType == ExpressionTokenValueTypeof::OPERATION);
    assert(token->value.operation.operationId == ExpressionOperationsIds::ARCTAN);

    return TOKEN(DIV, D(token->left), 
                      TOKEN(ADD, CONST_TOKEN(1),
                                 TOKEN(POW, C(token->left), CONST_TOKEN(2))));
}

static inline ExpressionTokenType* ExpressionDifferentiateARCCOT(
                                        const ExpressionTokenType* token,
                                        const ExpressionVariablesArrayType* varsArr,
                                        FILE* outTex)
{
    assert(token);
    assert(token->valueType == ExpressionTokenValueTypeof::OPERATION);
    assert(token->value.operation.operationId == ExpressionOperationsIds::ARCTAN);

    return TOKEN(MUL, CONST_TOKEN(-1),
                      TOKEN(DIV, D(token->left),
                                 TOKEN(ADD, CONST_TOKEN(1),
                                            TOKEN(POW, C(token->left), CONST_TOKEN(2)))));
}

static inline DiffFuncType* GetOperationDiffFunc(const ExpressionOperationsIds operationId)
{
    assert((int)operationId >= 0);
    assert((size_t)operationId < NumberOfOperations);

    return OperationsDiff[(size_t)operationId];
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