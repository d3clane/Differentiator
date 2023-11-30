#ifndef GENERATE_OPERATION_CMD
#define GENERATE_OPERATION_CMD(...)
#endif

//GENERATE_OPERATION_CMD(NAME, FORMAT, TEX_FORMAT, IS_UNARY, SHORT_CUT_STRING, TEX_NAME,
//                       NEED_LEFT_TEX_BRACES, NEED_RIGHT_TEX_BRACES,                  
//                       OPERATION_CALCULATION_CODE, OPERATION_DIFF_CODE,
//                       GNU_PLOT_NAME, GNU_PLOT_FORMAT) 

//OPERATION_CALCILATION_CODE - format of function f(const double val1, const double val2)
//OPERATION_DIFF_CODE        - format of function f(const ExpressionTokenType* token)

/*

#include "DSL.h"

#ifndef TOKEN
#define TOKEN(...)
#endif

#ifndef D
#define D(...)
#endif

#ifndef C
#define C(...)
#endif

#ifndef GENERATE_OPERATION_CMD
#define GENERATE_OPERATION_CMD(...)
#endif

*/

#define CALC_CHECK()            \
do                              \
{                               \
    assert(isfinite(val1));     \
    assert(isfinite(val2));     \
} while (0)

#ifdef DSL_H

#define DIFF_CHECK(NAME)                                    \
do                                                          \
{                                                           \
    assert(token);                                          \
    assert(TOKEN_VAL_TYPE(token) == OP_TYPE)                \
    assert(TOKEN_OP(token) == ExpressionOperationId::NAME)  \
} while (0)

#else

#define DIFF_CHECK(NAME) 

#endif

GENERATE_OPERATION_CMD(ADD, INFIX,  INFIX, false, "+", "+", false, false,
{
    CALC_CHECK();

    return val1 + val2;
},
{
    DIFF_CHECK(ADD);

    return _ADD(D(token->left), D(token->right));
},
"+", INFIX)

GENERATE_OPERATION_CMD(SUB, INFIX,  INFIX, false, "-", "-",      false, false,
{
    CALC_CHECK();

    return val1 - val2;
},
{
    DIFF_CHECK(SUB);

    return _SUB(D(token->left), D(token->right));
},
"-", INFIX)

GENERATE_OPERATION_CMD(MUL, INFIX,  INFIX, false, "*", "\\cdot", false, false,
{
    CALC_CHECK();

    return val1 * val2;
},
{
    DIFF_CHECK(MUL);

    return _ADD(_MUL(D(token->left), C(token->right)), 
                      _MUL(C(token->left), D(token->right)));
},
"*", INFIX)

GENERATE_OPERATION_CMD(DIV, INFIX, PREFIX, false, "/", "\\frac", true,  true,
{
    CALC_CHECK();
    assert(!DoubleEqual(val2, 0));

    return val1 / val2;
},
{
    DIFF_CHECK(DIV);

    return _DIV(_SUB(_MUL(D(token->left), C(token->right)), 
                                 _MUL(C(token->left), D(token->right))),
                      _POW(C(token->right), NUM(2)));
},
"/", INFIX)

GENERATE_OPERATION_CMD(POW, INFIX, INFIX, false,     "^",     "^",  false, true,
{
    CALC_CHECK();

    return pow(val1, val2);
},
{
    DIFF_CHECK(POW);

    bool baseContainVar  = ExpressionTokenContainVariable(token->left);
    bool powerContainVar = ExpressionTokenContainVariable(token->right);

    if (!baseContainVar && !powerContainVar)
        return NUM(0);

    if (baseContainVar && !powerContainVar)
        return _MUL(_MUL(C(token->right), D(token->left)), 
                          _POW(C(token->left), 
                                     _SUB(C(token->right), NUM(1))));
                        
    if (!baseContainVar && powerContainVar)
        return _MUL(_POW(C(token->left), C(token->right)),
                          _MUL(_LN(C(token->left)),
                                     D(token->right)));

    return _MUL(_POW(C(token->left), C(token->right)),
                      _ADD(_MUL(C(token->right),
                                            _DIV(D(token->left), C(token->left))),
                                 _MUL(_LN(C(token->left)), D(token->right))));
},
"**", INFIX)

GENERATE_OPERATION_CMD(LOG, PREFIX, PREFIX, false, "log", "\\log_", true, false,
{
    CALC_CHECK();

    double log_base = log(val1);

    assert(!DoubleEqual(log_base, 0));

    return log(val2) / log_base;
},
{
    DIFF_CHECK(LOG);

    return _DIV(D(token->left), C(token->left));
},
"log", PREFIX)

#undef  CALC_CHECK
#define CALC_CHECK()        \
do                          \
{                           \
    assert(isfinite(val1)); \
} while (0)

GENERATE_OPERATION_CMD(LN,  PREFIX, PREFIX, true,  "ln",  "\\ln",   false, false,
{
    CALC_CHECK();

    return log(val1);
},
{
    DIFF_CHECK(LN);

    return _DIV(D(token->left), C(token->left));
},
"log", PREFIX)

GENERATE_OPERATION_CMD(SIN, PREFIX, PREFIX, true, "sin", "\\sin", false, false,
{
    CALC_CHECK();

    return sin(val1);
},
{
    DIFF_CHECK(SIN);

    return _MUL(_COS(C(token->left)), D(token->left));
},
"sin", PREFIX)

GENERATE_OPERATION_CMD(COS, PREFIX, PREFIX, true, "cos", "\\cos", false, false,
{
    CALC_CHECK();

    return cos(val1);
},
{
    DIFF_CHECK(COS);

    return _MUL(NUM(-1), 
                      _MUL(_SIN(C(token->left)), D(token->left)));
},
"cos", PREFIX)

GENERATE_OPERATION_CMD(TAN, PREFIX, PREFIX, true, "tan", "\\tan", false, false,
{
    CALC_CHECK();

    return tan(val1);
},
{
    DIFF_CHECK(TAN);

    return _DIV(D(token->left), 
                      _POW(_COS(C(token->left)), NUM(2)));
},
"tan", PREFIX)

GENERATE_OPERATION_CMD(COT, PREFIX, PREFIX, true, "cot", "\\cot", false, false,
{
    CALC_CHECK();

    double tan_val1 = tan(val1);

    assert(!DoubleEqual(tan_val1, 0));
    assert(isfinite(tan_val1));

    return 1 / tan_val1;
},
{
    DIFF_CHECK(COT);

    return _MUL(NUM(-1), 
                      _DIV(D(token->left), 
                                 _POW(_SIN(C(token->left)), NUM(2))));
},
"1 / tan", PREFIX)

GENERATE_OPERATION_CMD(ARCSIN, PREFIX, PREFIX, true, "arcsin", "\\arcsin", false, false,
{
    CALC_CHECK();

    return asin(val1);
},
{
    DIFF_CHECK(ARCSIN);

    return _DIV(D(token->left),
                      _POW(_SUB(NUM(1), 
                                            _POW(C(token->left), NUM(2))),
                                 NUM(0.5)));
},
"asin", PREFIX)

GENERATE_OPERATION_CMD(ARCCOS, PREFIX, PREFIX, true, "arccos", "\\arccos", false, false,
{
    CALC_CHECK();

    return acos(val1);
},
{
    DIFF_CHECK(ARCCOS);

    return _DIV(NUM(-1),
                      _MUL(D(token->left),
                                 _POW(_SUB(NUM(1), 
                                                       _POW(C(token->left), NUM(2))),
                                            NUM(0.5))));
},
"acos", PREFIX)

GENERATE_OPERATION_CMD(ARCTAN, PREFIX, PREFIX, true, "arctan", "\\arctan", false, false,
{
    CALC_CHECK();

    return atan(val1);
},
{
    DIFF_CHECK(ARCTAN);

    return _DIV(D(token->left), 
                      _ADD(NUM(1),
                                 _POW(C(token->left), NUM(2))));
},
"atan", PREFIX)

GENERATE_OPERATION_CMD(ARCCOT, PREFIX, PREFIX, true, "arccot", "\\arccot", false, false,
{
    CALC_CHECK();

    return PI / 2 - atan(val1);
},
{
    DIFF_CHECK(ARCCOT);

    return _MUL(NUM(-1),
                      _DIV(D(token->left),
                                 _ADD(NUM(1),
                                            _POW(C(token->left), NUM(2)))));
},
"pi / 2 - atan", PREFIX)

#undef CALC_CHECK
#undef DIFF_CHECK