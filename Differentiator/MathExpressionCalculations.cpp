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
    static inline ExpressionTokenType* _##NAME(ExpressionTokenType* left,                       \
                                               ExpressionTokenType* right = nullptr)            \
    {                                                                                           \
        return ExpressionTokenCreate(ExpressionTokenValueСreate(ExpressionOperationId::NAME),   \
                                   ExpressionTokenValueTypeof::OPERATION,                       \
                                   left, right);                                                \
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
    
    if (IS_VAL(token))
        return VAL(token);

    if (IS_VAR(token))
        return token->value.varPtr->variableValue;

    double firstVal  = ExpressionCalculate(L(token));
    double secondVal = ExpressionCalculate(R(token));
    
    return CalculateUsingOperation(OP(token), firstVal, secondVal);
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
            "According to the legend, the ancient Ruses were able to defeat the Raptors "
            "by taking this derivative:");

    ExpressionTokenType* diffRootToken = ExpressionDifferentiate(expression->root, 
                                                                 outTex);
    ExpressionType diffExpression = {};
    ExpressionCtor(&diffExpression);

    diffExpression.root = diffRootToken;

    ExpressionCopyVariables(&diffExpression, expression);

    if (outTex)
    {
        ExpressionPrintTex(&diffExpression, outTex, "The ancient Ruses got this result\n");
        fprintf(outTex, "No one gives a **** what's going on here, "
                        "but according to the standards I have to say it - "
                        "\"gksjfpejdsifljdkfjsefijdsflfj\".\\\\\n");
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
        //THERE IS RECURSION TO ExpressionDifferentiate inside include
        #include "Operations.h"

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

    if (token == nullptr || IS_VAL(token))
    {
        if (haveVariables != nullptr)
            *haveVariables = false;
        
        return token;
    }

    if (IS_VAR(token))
    {
        if (haveVariables != nullptr)
            *haveVariables = true;

        return token;
    }

    bool leftTokenHaveVariables  = false;
    bool rightTokenHaveVariables = false;
    ExpressionTokenType* left  = ExpressionSimplifyConstants(L(token),  
                                                             simplifiesCount,
                                                             &leftTokenHaveVariables,
                                                             outTex);
    ExpressionTokenType* right = ExpressionSimplifyConstants(R(token), 
                                                             simplifiesCount,
                                                             &rightTokenHaveVariables,
                                                             outTex);

    if (L(token) != left)
    {
        ExpressionTokenDtor(L(token));
        token->left = left;
    }

    if (R(token) != right)
    {
        ExpressionTokenDtor(R(token));
        token->right = right;
    }

    bool tokenHaveVariables = leftTokenHaveVariables | rightTokenHaveVariables;

    if (haveVariables != nullptr) *haveVariables = tokenHaveVariables;

    if (!tokenHaveVariables)
    {
        (*simplifiesCount)++;
        
        double leftVal = NAN, rightVal = NAN;

        assert(L(token));

        leftVal = L_VAL(token);
        if (R(token)) rightVal = R_VAL(token);

        ExpressionTokenType* simplifiedToken = CRT_NUM(
                                    CalculateUsingOperation(OP(token), leftVal, rightVal));

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
    if (token == nullptr || !IS_OP(token))
        return token;
    
    ExpressionTokenType* left  = ExpressionSimplifyNeutralTokens(L(token), simplifiesCount,
                                                                 outTex);
    ExpressionTokenType* right = ExpressionSimplifyNeutralTokens(R(token), simplifiesCount,
                                                                 outTex);
    
    if (L(token) != left)
    {
        ExpressionTokenDtor(L(token));
        token->left = left;        
    }

    if (R(token) != right)
    {
        ExpressionTokenDtor(R(token));
        token->right = right;
    }

    if (left == nullptr || right == nullptr)
        return token;
    
    assert(IS_OP(token));

    if (IS_VAR(right) && IS_VAR(left) && VAR(left) == VAR(right))
        return ExpressionSimplifyReturnConstToken(token, 0, outTex);

    if (!IS_VAL(left) && !IS_VAL(right))
        return token;

    assert(L(token)  == left);
    assert(R(token) == right);
    switch (OP(token))
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

#define CHECK()                 \
do                              \
{                               \
    assert(simplifiesCount);    \
    assert(token);              \
    assert(L(token));           \
    assert(R(token));           \
} while (0)


static inline ExpressionTokenType* ExpressionSimplifyAdd(ExpressionTokenType* token,   
                                                         int* simplifiesCount,
                                                         FILE* outTex)
{
    CHECK();

    if (R_IS_VAL(token) && DoubleEqual(R_VAL(token), 0))
    {
        (*simplifiesCount)++;
        return ExpressionSimplifyReturnLeftToken(token, outTex);
    }

    if (L_IS_VAL(token) && DoubleEqual(L_VAL(token), 0))
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
    CHECK();

    if (R_IS_VAL(token) && DoubleEqual(R_VAL(token), 0))
    {    
        (*simplifiesCount)++;
        return ExpressionSimplifyReturnLeftToken(token, outTex);
    }

    if (L_IS_VAL(token) && DoubleEqual(L_VAL(token), 0))
    {
        (*simplifiesCount)++;

        ExpressionTokenDtor(L(token));
        token->left = nullptr;

        ExpressionTokenType* simpledToken = _MUL(CRT_NUM(-1), R(token));
        token->right = nullptr;

        return simpledToken;
    }

    return token;
}

static inline ExpressionTokenType* ExpressionSimplifyMul(ExpressionTokenType* token,   
                                                          int* simplifiesCount,
                                                         FILE* outTex)
{
    CHECK();

    if (R_IS_VAL(token) && DoubleEqual(R_VAL(token), 0))
    {
        (*simplifiesCount)++;
        return ExpressionSimplifyReturnConstToken(token, 0, outTex);
    }

    if (L_IS_VAL(token) && DoubleEqual(L_VAL(token), 0))
    {
        (*simplifiesCount)++;
        return ExpressionSimplifyReturnConstToken(token, 0, outTex);
    }

    if (R_IS_VAL(token) && DoubleEqual(R_VAL(token), 1))
    {
        (*simplifiesCount)++;
        return ExpressionSimplifyReturnLeftToken(token, outTex);
    }

    if (L_IS_VAL(token) && DoubleEqual(L_VAL(token), 1))
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
    CHECK();

    if (L_IS_VAL(token) && DoubleEqual(L_VAL(token), 0))
    {
        (*simplifiesCount)++;
        return ExpressionSimplifyReturnConstToken(token, 0, outTex);
    }

    if (R_IS_VAL(token) && DoubleEqual(R_VAL(token), 1))
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
    CHECK();

    if (R_IS_VAL(token) && DoubleEqual(R_VAL(token), 0))
    {
        (*simplifiesCount)++;
        return ExpressionSimplifyReturnConstToken(token, 1, outTex);
    }

    if (L_IS_VAL(token) && DoubleEqual(L_VAL(token), 0))
    {
        (*simplifiesCount)++;
        return ExpressionSimplifyReturnConstToken(token, 0, outTex);
    }

    if (R_IS_VAL(token) && DoubleEqual(R_VAL(token), 1))
    {
        (*simplifiesCount)++;
        return ExpressionSimplifyReturnLeftToken(token, outTex);
    }

    if (L_IS_VAL(token) && DoubleEqual(L_VAL(token), 1))
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
    CHECK();

    if (R_IS_VAL(token) && DoubleEqual(R_VAL(token), 1))
    {
        (*simplifiesCount)++;
        return ExpressionSimplifyReturnConstToken(token, 0, outTex);
    }

    return token;
}

#undef CHECK
//---------------------------------------------------------------------------------------

static inline ExpressionTokenType* ExpressionSimplifyReturnLeftToken(ExpressionTokenType* token,
                                                                     FILE* outTex)
{
    assert(token);

    TokenPrintDifferenceToTex(token, L(token), outTex, "Slozhno ne ponyat, chto delat s etim:");

    ExpressionTokenDtor(token->right);

    ExpressionTokenType* left = L(token);

    token->left  = nullptr;
    token->right = nullptr;

    return left;
}

static inline ExpressionTokenType* ExpressionSimplifyReturnRightToken(
                                                                ExpressionTokenType* token,
                                                                FILE* outTex)
{
    assert(token);

    TokenPrintDifferenceToTex(token, R(token), outTex, 
                        "Avtor ne smog perevesti na english(");

    ExpressionTokenDtor(token->left);

    ExpressionTokenType* right = R(token);
    
    token->left  = nullptr;
    token->right = nullptr;

    return right;
}

static inline ExpressionTokenType* ExpressionSimplifyReturnConstToken(
                                                                ExpressionTokenType* token,
                                                                double value,
                                                                FILE* outTex)
{
    ExpressionTokenType* constToken = CRT_NUM(value);
    
    TokenPrintDifferenceToTex(token, constToken, outTex,
                        "Let's use the theorem ..."
                        "(The author doesn't know how this theorem is called in English, "
                        "you are left to guess for yourself)");
    
    ExpressionTokenDtor(R(token));
    ExpressionTokenDtor(L(token));

    token->left  = nullptr;
    token->right = nullptr;

    return CRT_NUM(value);
}

//---------------------------------------------------------------------------------------

static bool ExpressionTokenContainVariable(const ExpressionTokenType* token)
{
    if (token == nullptr)
        return false;
    
    if (IS_VAR(token))
        return true;

    bool containVariable  = ExpressionTokenContainVariable(L(token));

    if (containVariable)
        return containVariable;
    
    containVariable = ExpressionTokenContainVariable(R(token));

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

    taylorSeries.root = CRT_NUM(ExpressionCalculate(&tmpDiffExpr));
    ExpressionTokenType* xToken = CRT_VAR(&taylorSeries.variables, 
                                             taylorSeries.variables.data[0].variableName);

    for (size_t i = 1; i <= n; ++i)
    {
        ExpressionType tmp = ExpressionDifferentiate(&tmpDiffExpr);
        ExpressionDtor(&tmpDiffExpr);

        taylorSeries.root = _ADD(taylorSeries.root, 
                                 _MUL(CRT_NUM(ExpressionCalculate(tmp.root)), 
                                     _POW(C(xToken), CRT_NUM(i))));

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

    ExpressionTokenType* xToken = CRT_VAR(&tangent.variables, varName);

    tangent.root = _ADD(_MUL(CRT_NUM(diffValInX), C(xToken)), 
                        _SUB(CRT_NUM(exprValInX), _MUL(CRT_NUM(diffValInX), CRT_NUM(x))));

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

#undef D
#undef C

#undef  OP_TYPE_CNST
#undef VAR_TYPE_CNST
#undef VAL_TYPE_CNST
#undef TOKEN_OP

#undef VAL_TYPE
#undef VAL      
#undef VAR      
#undef OP      
#undef L
#undef R

#undef L_VAL
#undef R_VAL
#undef L_VAR
#undef R_VAR
#undef  L_OP
#undef  R_OP

#undef   IS_VAL
#undef L_IS_VAL
#undef R_IS_VAL
#undef   IS_VAR
#undef L_IS_VAR
#undef R_IS_VAR
#undef    IS_OP
#undef  L_IS_OP
#undef  R_IS_OP

#undef CRT_NUM
#undef CRT_VAR

//---------------------------------------------------------------------------------------
