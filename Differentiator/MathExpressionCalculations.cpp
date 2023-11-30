#include <assert.h>
#include <math.h>

#include "MathExpressionCalculations.h"
#include "MathExpressionInOut.h"
#include "Common/DoubleFuncs.h"
#include "MathExpressionTexDump.h"

//---------------Calculation-------------------

static double ExpressionCalculate(const ExpressionTokenType* token);

static double CalculateUsingOperation(const ExpressionOperationId operation, 
                                      const double val1, const double val2 = NAN);


//--------------------DSL-----------------------------

#define D(TOKEN) ExpressionDifferentiate(TOKEN, outTex)
#define C(TOKEN) ExpressionTokenCopy(TOKEN)                                     

#define NUM_TOKEN(VALUE)    ExpressionNumericTokenCreate(VALUE)
#define VAR_TOKEN(VARS_ARR, VAR_NAME) ExpressionVariableTokenCreate(VARS_ARR, VAR_NAME)

#define GENERATE_OPERATION_CMD(NAME, ...)                                                   \
    static inline ExpressionTokenType* _##NAME(ExpressionTokenType* left,                   \
                                               ExpressionTokenType* right = nullptr)        \
    {                                                                                       \
        return ExpressionTokenCreate(ExpressionTokenValueСreate(ExpressionOperationId::NAME), \
                                   ExpressionTokenValueTypeof::OPERATION,                   \
                                   left, right);                                            \
    }

//GENERATING DSL _ADD, _MUL, _SUB, ...
#include "Operations.h"

#undef GENERATE_OPERATION_CMD

//-------------------Differentiate---------------

static ExpressionTokenType* ExpressionDifferentiate(
                                        const ExpressionTokenType* token,
                                        FILE* outTex = nullptr);

static ExpressionTokenType* ExpressionDiffOperation(const ExpressionTokenType* token,
                                                    FILE* outTex = nullptr);

//--------------------------------Simplify-------------------------------------------

static ExpressionTokenType* ExpressionSimplifyConstants (ExpressionTokenType* token,
                                                         int* simplifiesCount,
                                                         bool* haveVariables = nullptr,
                                                         FILE* outTex = nullptr);

static ExpressionTokenType* ExpressionSimplifyNeutralTokens(ExpressionTokenType* token, 
                                                            int* simplifiesCount,
                                                            FILE* outTex = nullptr);

static inline ExpressionTokenType* ExpressionSimplifyAdd(ExpressionTokenType* token,   
                                                         int* simplifiesCount,
                                                         FILE* outTex = nullptr);
static inline ExpressionTokenType* ExpressionSimplifySub(ExpressionTokenType* token,   
                                                          int* simplifiesCount,
                                                          FILE* outTex = nullptr);
static inline ExpressionTokenType* ExpressionSimplifyMul(ExpressionTokenType* token,   
                                                          int* simplifiesCount,
                                                          FILE* outTex = nullptr);
static inline ExpressionTokenType* ExpressionSimplifyDiv(ExpressionTokenType* token,   
                                                          int* simplifiesCount,
                                                          FILE* outTex = nullptr);
static inline ExpressionTokenType* ExpressionSimplifyPow(ExpressionTokenType* token,   
                                                          int* simplifiesCount,
                                                          FILE* outTex = nullptr);
static inline ExpressionTokenType* ExpressionSimplifyLog(ExpressionTokenType* token,   
                                                          int* simplifiesCount,
                                                          FILE* outTex = nullptr);

static inline ExpressionTokenType* ExpressionSimplifyReturnLeftToken(
                                                                ExpressionTokenType* token,
                                                                FILE* outTex = nullptr);
static inline ExpressionTokenType* ExpressionSimplifyReturnRightToken(
                                                                ExpressionTokenType* token,
                                                                FILE* outTex = nullptr);
static inline ExpressionTokenType* ExpressionSimplifyReturnConstToken(
                                                                ExpressionTokenType* token,
                                                                double value,
                                                                FILE* outTex = nullptr);

//---------------------------------------------------------------------------------------

static bool ExpressionTokenContainVariable(const ExpressionTokenType* token);

//---------------------------------------------------------------------------------------

static inline void TokenPrintDifferenceToTex(const ExpressionTokenType* prevToken, 
                                             const ExpressionTokenType* newToken, FILE* outTex, 
                                             const char* stringToPrint);

//---------------------------------------------------------------------------------------

double ExpressionCalculate(const ExpressionType* expression)
{
    assert(expression);

    return ExpressionCalculate(expression->root);
}

static double ExpressionCalculate(const ExpressionTokenType* token)
{
    if (token == nullptr)
        return NAN;
    
    if (token->valueType == ExpressionTokenValueTypeof::VALUE)
    {
        return token->value.value;
    }

    if (token->valueType == ExpressionTokenValueTypeof::VARIABLE)
    {
        return token->value.varPtr->variableValue;
    }

    double firstVal  = ExpressionCalculate(token->left);
    double secondVal = ExpressionCalculate(token->right);
    
    return CalculateUsingOperation(token->value.operation, firstVal, secondVal);
}

static double CalculateUsingOperation(const ExpressionOperationId operation, 
                                      const double val1, const double val2)
{
    #define GENERATE_OPERATION_CMD(NAME, v1, v2, v3, v4, v5, v6, v7, CALCULATE_CODE, ...)   \
        case ExpressionOperationId::NAME:                                                   \
        {                                                                                   \
            CALCULATE_CODE;                                                                 \
            break;                                                                          \
        }                                                                               

    switch(operation)
    {
        #include "Operations.h"

        default:
            break;
    }

    #undef GENERATE_OPERATION_CMD

    return NAN;
}

//---------------------------------------------------------------------------------------

ExpressionType ExpressionDifferentiate(const ExpressionType* expression,
                                       FILE* outTex)
{
    assert(expression);
    
    if (outTex)
        ExpressionPrintTex(expression, outTex, 
            "According to legend, the ancient Ruses were able to defeat the Raptors "
            "by taking this derivative:");

    ExpressionTokenType* diffRootToken = ExpressionDifferentiate(expression->root, 
                                                                 outTex);
    ExpressionType diffExpression = {};
    ExpressionCtor(&diffExpression);

    diffExpression.root = diffRootToken;

    ExpressionCopyVariables(&diffExpression, expression);

    if (outTex)
    {
        ExpressionPrintTex(&diffExpression, outTex, "The ancient Rus, like us, got this result\n");
        fprintf(outTex, "No one gives a **** what's going on here, "
                        "but according to the standards I have to say it - "
                        "\"***********************\".");
    }

    ExpressionSimplify(&diffExpression, outTex);

    return diffExpression;
}

static ExpressionTokenType* ExpressionDifferentiate(const ExpressionTokenType* token,
                                                    FILE* outTex)
{
    assert(token);

    ExpressionTokenValue val = {};
    ExpressionTokenType* diffToken = nullptr;

    switch(token->valueType)
    {
        case ExpressionTokenValueTypeof::VALUE:
            val.value = 0;
            diffToken =  ExpressionTokenCreate(val, ExpressionTokenValueTypeof::VALUE);
            break;
        case ExpressionTokenValueTypeof::VARIABLE:
            val.value = 1;
            diffToken =  ExpressionTokenCreate(val, ExpressionTokenValueTypeof::VALUE);
            break;

        case ExpressionTokenValueTypeof::OPERATION:
            diffToken = ExpressionDiffOperation(token, outTex);
            break;

        default:
            break;
    }

    TokenPrintDifferenceToTex(token, diffToken, outTex, "Let's take the derivative of: ");

    return diffToken;  
}

//---------------------------------------------------------------------------------------

static ExpressionTokenType* ExpressionDiffOperation(const ExpressionTokenType* token,
                                                    FILE* outTex)
{
    assert(token->valueType == ExpressionTokenValueTypeof::OPERATION);

    #define GENERATE_OPERATION_CMD(NAME, v1, v2, v3, v4, v5, v6, v7, v8, DIFF_CODE, ...)\
        case ExpressionOperationId::NAME:                                               \
        {                                                                               \
            DIFF_CODE;                                                                  \
            break;                                                                      \
        }

    switch(token->value.operation)
    {
        #include "Operations.h"

        //THERE IS RECURSION TO ExpressionDifferentiate inside include
        default:
            break;
    }
    
    #undef GENERATE_OPERATION_CMD

    return nullptr;
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
        expression->root = ExpressionSimplifyConstants(expression->root, &simplifiesCount, 
                                                       nullptr, outTex);
        expression->root = ExpressionSimplifyNeutralTokens(expression->root, &simplifiesCount,
                                                           outTex);
    } while (simplifiesCount != 0);

    if (outTex) ExpressionPrintTex(expression, outTex, "Final expression after simplifications:");
}

static ExpressionTokenType* ExpressionSimplifyConstants (ExpressionTokenType* token,
                                                         int* simplifiesCount,
                                                         bool* haveVariables,
                                                         FILE* outTex)
{
    assert(simplifiesCount);

    if (token == nullptr || token->valueType == ExpressionTokenValueTypeof::VALUE)
    {
        if (haveVariables != nullptr)
            *haveVariables = false;
        
        return token;
    }

    if (token->valueType == ExpressionTokenValueTypeof::VARIABLE)
    {
        if (haveVariables != nullptr)
            *haveVariables = true;

        return token;
    }

    bool leftTokenHaveVariables  = false;
    bool rightTokenHaveVariables = false;
    ExpressionTokenType* left  = ExpressionSimplifyConstants(token->left,  
                                                             simplifiesCount,
                                                             &leftTokenHaveVariables,
                                                             outTex);
    ExpressionTokenType* right = ExpressionSimplifyConstants(token->right, 
                                                             simplifiesCount,
                                                             &rightTokenHaveVariables,
                                                             outTex);

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

    bool tokenHaveVariables = leftTokenHaveVariables | rightTokenHaveVariables;

    if (haveVariables != nullptr) *haveVariables = tokenHaveVariables;

    if (!tokenHaveVariables)
    {
        (*simplifiesCount)++;
        
        double leftVal = NAN, rightVal = NAN;

        assert(token->left);

        leftVal = token->left->value.value;
        if (token->right) rightVal = token->right->value.value;

        ExpressionTokenType* simplifiedToken = NUM_TOKEN(
                                    CalculateUsingOperation(token->value.operation, leftVal, 
                                                                                    rightVal));

        TokenPrintDifferenceToTex(token, simplifiedToken, outTex, "Let's simplify this expression: "); 

        return simplifiedToken;
    }
    return token;
}

//---------------------------------------------------------------------------------------

static inline void TokenPrintDifferenceToTex(const ExpressionTokenType* prevToken, 
                                         const ExpressionTokenType* newToken, FILE* outTex, 
                                         const char* stringToPrint)
{
    assert(prevToken);
    assert(newToken);

    if (outTex)
    {
        ExpressionTokenPrintTexWithTrollString(prevToken, outTex, stringToPrint);
        ExpressionTokenPrintTexWithTrollString(newToken, outTex);
    }
}

static ExpressionTokenType* ExpressionSimplifyNeutralTokens(ExpressionTokenType* token, 
                                                            int* simplifiesCount,
                                                            FILE* outTex)
{
    if (token == nullptr || token->valueType != ExpressionTokenValueTypeof::OPERATION)
        return token;
    
    ExpressionTokenType* left  = ExpressionSimplifyNeutralTokens(token->left, simplifiesCount,
                                                                 outTex);
    ExpressionTokenType* right = ExpressionSimplifyNeutralTokens(token->right, simplifiesCount,
                                                                 outTex);
    
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
        return ExpressionSimplifyReturnConstToken(token, 0, outTex);

    bool rightIsValue = (right->valueType == ExpressionTokenValueTypeof::VALUE);
    bool leftIsValue  = (left->valueType  == ExpressionTokenValueTypeof::VALUE);

    if (!leftIsValue && !rightIsValue)
        return token;

    assert(token->left  == left);
    assert(token->right == right);
    switch (token->value.operation)
    {
        case ExpressionOperationId::ADD:
            return ExpressionSimplifyAdd(token, simplifiesCount, outTex);
        case ExpressionOperationId::SUB:
            return ExpressionSimplifySub(token, simplifiesCount, outTex);
        case ExpressionOperationId::MUL:
            return ExpressionSimplifyMul(token, simplifiesCount, outTex);
        case ExpressionOperationId::DIV:
            return ExpressionSimplifyDiv(token, simplifiesCount, outTex);
        
        case ExpressionOperationId::POW:
            return ExpressionSimplifyPow(token, simplifiesCount, outTex);
        case ExpressionOperationId::LOG:
            return ExpressionSimplifyLog(token, simplifiesCount, outTex);
        
        default:
            break;
    }

    return token;
}

//---------------------------------------------------------------------------------------

static inline ExpressionTokenType* ExpressionSimplifyAdd(ExpressionTokenType* token,   
                                                         int* simplifiesCount,
                                                         FILE* outTex)
{
    
    assert(simplifiesCount);
    assert(token);
    assert(token->left);
    assert(token->right);

    ExpressionTokenType* left  = token->left;
    ExpressionTokenType* right = token->right;

    bool rightIsValue = (right->valueType == ExpressionTokenValueTypeof::VALUE);
    bool leftIsValue  = (left->valueType  == ExpressionTokenValueTypeof::VALUE);

    if (rightIsValue && DoubleEqual(right->value.value, 0))
    {
        (*simplifiesCount)++;
        return ExpressionSimplifyReturnLeftToken(token, outTex);
    }

    if (leftIsValue && DoubleEqual(left->value.value, 0))
    {
        (*simplifiesCount)++;
        return ExpressionSimplifyReturnRightToken(token, outTex);
    }

    return token;
}

static inline ExpressionTokenType* ExpressionSimplifySub(ExpressionTokenType* token,   
                                                          int* simplifiesCount,
                                                         FILE* outTex)
{
    assert(token);
    assert(token->left);
    assert(token->right);

    ExpressionTokenType* left  = token->left;
    ExpressionTokenType* right = token->right;

    bool rightIsValue = (right->valueType == ExpressionTokenValueTypeof::VALUE);
    bool leftIsValue  = (left->valueType  == ExpressionTokenValueTypeof::VALUE);

    if (rightIsValue && DoubleEqual(right->value.value, 0))
    {    
        (*simplifiesCount)++;
        return ExpressionSimplifyReturnLeftToken(token, outTex);
    }

    if (leftIsValue && DoubleEqual(left->value.value, 0))
    {
        (*simplifiesCount)++;

        ExpressionTokenDtor(token->left);
        token->left = nullptr;

        ExpressionTokenType* simpledToken = _MUL(NUM_TOKEN(-1), token->right);
        token->right = nullptr;

        return simpledToken;
    }

    return token;
}

static inline ExpressionTokenType* ExpressionSimplifyMul(ExpressionTokenType* token,   
                                                          int* simplifiesCount,
                                                         FILE* outTex)
{
    assert(token);
    assert(token->left);
    assert(token->right);

    ExpressionTokenType* left  = token->left;
    ExpressionTokenType* right = token->right;

    bool rightIsValue = (right->valueType == ExpressionTokenValueTypeof::VALUE);
    bool leftIsValue  = (left->valueType  == ExpressionTokenValueTypeof::VALUE);

    if (rightIsValue && DoubleEqual(right->value.value, 0))
    {
        (*simplifiesCount)++;
        return ExpressionSimplifyReturnConstToken(token, 0, outTex);
    }

    if (leftIsValue && DoubleEqual(left->value.value, 0))
    {
        (*simplifiesCount)++;
        return ExpressionSimplifyReturnConstToken(token, 0, outTex);
    }

    if (rightIsValue && DoubleEqual(right->value.value, 1))
    {
        (*simplifiesCount)++;
        return ExpressionSimplifyReturnLeftToken(token, outTex);
    }

    if (leftIsValue && DoubleEqual(left->value.value, 1))
    {
        (*simplifiesCount)++;
        return ExpressionSimplifyReturnRightToken(token, outTex);
    }

    return token;
}

static inline ExpressionTokenType* ExpressionSimplifyDiv(ExpressionTokenType* token,   
                                                          int* simplifiesCount,
                                                         FILE* outTex)
{
    assert(token);
    assert(token->left);
    assert(token->right);

    ExpressionTokenType* left  = token->left;
    ExpressionTokenType* right = token->right;

    bool rightIsValue = (right->valueType == ExpressionTokenValueTypeof::VALUE);
    bool leftIsValue  = (left->valueType  == ExpressionTokenValueTypeof::VALUE);

    if (leftIsValue && DoubleEqual(left->value.value, 0))
    {
        (*simplifiesCount)++;
        return ExpressionSimplifyReturnConstToken(token, 0, outTex);
    }

    if (rightIsValue && DoubleEqual(right->value.value, 1))
    {
        (*simplifiesCount)++;
        return ExpressionSimplifyReturnLeftToken(token, outTex);
    }

    return token;
}

static inline ExpressionTokenType* ExpressionSimplifyPow(ExpressionTokenType* token,   
                                                          int* simplifiesCount,
                                                         FILE* outTex)
{
    assert(token);
    assert(token->left);
    assert(token->right);

    ExpressionTokenType* left  = token->left;
    ExpressionTokenType* right = token->right;

    bool rightIsValue = (right->valueType == ExpressionTokenValueTypeof::VALUE);
    bool leftIsValue  = (left->valueType  == ExpressionTokenValueTypeof::VALUE);

    if (rightIsValue && DoubleEqual(right->value.value, 0))
    {
        (*simplifiesCount)++;
        return ExpressionSimplifyReturnConstToken(token, 1, outTex);
    }

    if (leftIsValue && DoubleEqual(left->value.value, 0))
    {
        (*simplifiesCount)++;
        return ExpressionSimplifyReturnConstToken(token, 0, outTex);
    }

    if (rightIsValue && DoubleEqual(right->value.value, 1))
    {
        (*simplifiesCount)++;
        return ExpressionSimplifyReturnLeftToken(token, outTex);
    }

    if (leftIsValue && DoubleEqual(left->value.value, 1))
    {
        (*simplifiesCount)++;
        return ExpressionSimplifyReturnConstToken(token, 1, outTex);
    }

    return token;
}

static inline ExpressionTokenType* ExpressionSimplifyLog(ExpressionTokenType* token,   
                                                          int* simplifiesCount,
                                                         FILE* outTex)
{
    assert(token);
    assert(token->left);
    assert(token->right);

    ExpressionTokenType* left  = token->left;
    ExpressionTokenType* right = token->right;

    bool rightIsValue = (right->valueType == ExpressionTokenValueTypeof::VALUE);

    if (rightIsValue && DoubleEqual(right->value.value, 1))
    {
        (*simplifiesCount)++;
        return ExpressionSimplifyReturnConstToken(token, 0, outTex);
    }

    return token;
}

//---------------------------------------------------------------------------------------

static inline ExpressionTokenType* ExpressionSimplifyReturnLeftToken(ExpressionTokenType* token,
                                                                     FILE* outTex)
{
    assert(token);

    TokenPrintDifferenceToTex(token, token->left, outTex, "Slozhno ne ponyat, chto delat s etim:");

    ExpressionTokenDtor(token->right);

    ExpressionTokenType* left = token->left;

    token->left  = nullptr;
    token->right = nullptr;

    return left;
}

static inline ExpressionTokenType* ExpressionSimplifyReturnRightToken(
                                                                ExpressionTokenType* token,
                                                                FILE* outTex)
{
    assert(token);

    TokenPrintDifferenceToTex(token, token->right, outTex, 
                        "Avtor ne smog perevesti na english(");

    ExpressionTokenDtor(token->left);

    ExpressionTokenType* right = token->right;
    
    token->left  = nullptr;
    token->right = nullptr;

    return right;
}

static inline ExpressionTokenType* ExpressionSimplifyReturnConstToken(
                                                                ExpressionTokenType* token,
                                                                double value,
                                                                FILE* outTex)
{
    ExpressionTokenType* constToken = NUM_TOKEN(value);
    
    TokenPrintDifferenceToTex(token, constToken, outTex,
                        "Let's use the theorem ..."
                        "(The author doesn't know how this theorem is called in English, "
                        "you are left to guess for yourself)");
    
    ExpressionTokenDtor(token->right);
    ExpressionTokenDtor(token->left);

    token->left  = nullptr;
    token->right = nullptr;

    return NUM_TOKEN(value);
}

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

//---------------------------------------------------------------------------------------

ExpressionType ExpressionTaylor(const ExpressionType* expression, const int n, const double x)
{
    assert(expression);
    assert(expression->variables.size == 1);
    assert(n >= 0);

    ExpressionType tmpDiffExpr  = ExpressionCopy(expression);
    ExpressionVariableSet(&tmpDiffExpr, tmpDiffExpr.variables.data[0].variableName, x);

    ExpressionType taylorSeries = {};
    ExpressionCtor(&taylorSeries);
    ExpressionCopyVariables(&taylorSeries, expression);

    taylorSeries.root = NUM_TOKEN(ExpressionCalculate(&tmpDiffExpr));
    ExpressionTokenType* xToken = VAR_TOKEN(&taylorSeries.variables, 
                                             taylorSeries.variables.data[0].variableName);

    for (size_t i = 1; i <= n; ++i)
    {
        ExpressionType tmp = ExpressionDifferentiate(&tmpDiffExpr);
        ExpressionDtor(&tmpDiffExpr);

        taylorSeries.root = _ADD(taylorSeries.root, 
                                 _MUL(NUM_TOKEN(ExpressionCalculate(tmp.root)), 
                                     _POW(C(xToken), NUM_TOKEN(i))));

        tmpDiffExpr = tmp;
    }

    ExpressionDtor(&tmpDiffExpr);
    ExpressionTokenDtor(xToken);
    xToken = nullptr;

    ExpressionSimplify(&taylorSeries);

    return taylorSeries;
}

//---------------------------------------------------------------------------------------

ExpressionType ExpressionTangent(ExpressionType* expression, const double x)
{
    assert(expression);
    assert(expression->variables.size == 1);

    const char* varName = expression->variables.data[0].variableName;

    ExpressionType exprDiff = ExpressionDifferentiate(expression);
    ExpressionVariableSet(&exprDiff, varName, x);

    double prevVal = expression->variables.data[0].variableValue;
    ExpressionVariableSet(expression, varName, x);

    double diffValInX = ExpressionCalculate(&exprDiff);
    double exprValInX = ExpressionCalculate(expression);
    ExpressionVariableSet(expression, varName, prevVal);

    ExpressionDtor(&exprDiff);

    ExpressionType tangent = {};
    ExpressionCtor(&tangent);
    ExpressionCopyVariables(&tangent, expression);

    ExpressionTokenType* xToken = VAR_TOKEN(&tangent.variables, varName);

    tangent.root = _ADD(_MUL(NUM_TOKEN(diffValInX), C(xToken)), 
                        _SUB(NUM_TOKEN(exprValInX), _MUL(NUM_TOKEN(diffValInX), NUM_TOKEN(x))));

    ExpressionSimplify(&tangent);
    
    return tangent;
}

ExpressionType ExpressionSubTwoExpressions(const ExpressionType* expr1, 
                                           const ExpressionType* expr2)
{
    assert(expr1);
    assert(expr2);


    ExpressionTokenType* root = _SUB(expr1->root, expr2->root);

    ExpressionType subExpr = {};
    ExpressionCtor(&subExpr);
    subExpr.root = root;

    ExpressionCopyVariables(&subExpr, expr1);

    return subExpr;
}

//---------------------------------------------------------------------------------------

#undef C
#undef D
#undef CONST_TOKEN

//---------------------------------------------------------------------------------------
