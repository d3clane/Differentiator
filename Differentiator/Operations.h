#ifndef GENERATE_OPERATION_CMD
#define GENERATE_OPERATION_CMD(...)
#endif

//GENERATE_OPERATION_CMD(NAME, FORMAT, TEX_FORMAT, IS_UNARY, SHORT_CUT_STRING, TEX_NAME,
//                       NEED_LEFT_TEX_BRACES, NEED_RIGHT_TEX_BRACES,                  
//                       OPERATION_CALCULATION_CODE, OPERATION_DIFF_CODE) 

//OPERATION_CALCILATION_CODE - format of function f(const double val1, const double val2)
//OPERATION_DIFF_CODE        - format of function f(const ExpressionTokenType* token)

/*
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

GENERATE_OPERATION_CMD(ADD, INFIX,  INFIX, false, "+", "+", false, false,
{
    assert(isfinite(val1));
    assert(isfinite(val2));

    return val1 + val2;
},
{
    assert(token);
    assert(token->valueType == ExpressionTokenValueTypeof::OPERATION);
    assert(token->value.operation == ExpressionOperationId::ADD);

    return _ADD(D(token->left), D(token->right));
})

GENERATE_OPERATION_CMD(SUB, INFIX,  INFIX, false, "-", "-",      false, false,
{
    assert(isfinite(val1));
    assert(isfinite(val2));

    return val1 - val2;
},
{
    assert(token);
    assert(token->valueType == ExpressionTokenValueTypeof::OPERATION);
    assert(token->value.operation == ExpressionOperationId::SUB);

    return _SUB(D(token->left), D(token->right));
})

GENERATE_OPERATION_CMD(MUL, INFIX,  INFIX, false, "*", "\\cdot", false, false,
{
    assert(isfinite(val1));
    assert(isfinite(val2));

    return val1 * val2;
},
{
    assert(token);
    assert(token->valueType == ExpressionTokenValueTypeof::OPERATION);
    assert(token->value.operation == ExpressionOperationId::MUL);

    return _ADD(_MUL(D(token->left), C(token->right)), 
                      _MUL(C(token->left), D(token->right)));
})

GENERATE_OPERATION_CMD(DIV, INFIX, PREFIX, false, "/", "\\frac", true,  true,
{
    assert(isfinite(val1));
    assert(isfinite(val2));
    assert(!DoubleEqual(val2, 0));

    return val1 / val2;
},
{
    assert(token);
    assert(token->valueType == ExpressionTokenValueTypeof::OPERATION);
    assert(token->value.operation == ExpressionOperationId::DIV);

    return _DIV(_SUB(_MUL(D(token->left), C(token->right)), 
                                 _MUL(C(token->left), D(token->right))),
                      _POW(C(token->right), CONST_TOKEN(2)));
})

GENERATE_OPERATION_CMD(POW, INFIX, INFIX, false,     "^",     "^",  false, true,
{
    assert(isfinite(val1));
    assert(isfinite(val2));

    return pow(val1, val2);
},
{
    assert(token);
    assert(token->valueType == ExpressionTokenValueTypeof::OPERATION);
    assert(token->value.operation == ExpressionOperationId::POW);

    bool baseContainVar  = ExpressionTokenContainVariable(token->left);
    bool powerContainVar = ExpressionTokenContainVariable(token->right);

    if (!baseContainVar && !powerContainVar)
        return CONST_TOKEN(0);

    if (baseContainVar && !powerContainVar)
        return _MUL(_MUL(C(token->right), D(token->left)), 
                          _POW(C(token->left), 
                                     _SUB(C(token->right), CONST_TOKEN(1))));
                        
    if (!baseContainVar && powerContainVar)
        return _MUL(_POW(C(token->left), C(token->right)),
                          _MUL(_LN(C(token->left)),
                                     D(token->right)));

    return _MUL(_POW(C(token->left), C(token->right)),
                      _ADD(_MUL(C(token->right),
                                            _DIV(D(token->left), C(token->left))),
                                 _MUL(_LN(C(token->left)), D(token->right))));
})

GENERATE_OPERATION_CMD(LOG, PREFIX, PREFIX, false, "log", "\\log_", true, false,
{
    assert(isfinite(val1));
    assert(isfinite(val2));

    double log_base = log(val1);

    assert(!DoubleEqual(log_base, 0));

    return log(val2) / log_base;
},
{
    assert(token);
    assert(token->valueType == ExpressionTokenValueTypeof::OPERATION);
    assert(token->value.operation == ExpressionOperationId::LOG);


    return _DIV(D(token->left), C(token->left));
})

GENERATE_OPERATION_CMD(LN,  PREFIX, PREFIX, true,  "ln",  "\\ln",   false, false,
{
    assert(isfinite(val1));

    return log(val1);
},
{
    assert(token);
    assert(token->valueType == ExpressionTokenValueTypeof::OPERATION);
    assert(token->value.operation == ExpressionOperationId::LN);

    return _DIV(D(token->left), C(token->left));
})

GENERATE_OPERATION_CMD(SIN, PREFIX, PREFIX, true, "sin", "\\sin", false, false,
{
    assert(isfinite(val1));

    return sin(val1);
},
{
    assert(token);
    assert(token->valueType == ExpressionTokenValueTypeof::OPERATION);
    assert(token->value.operation == ExpressionOperationId::SIN);

    return _MUL(_COS(C(token->left)), D(token->left));
})

GENERATE_OPERATION_CMD(COS, PREFIX, PREFIX, true, "cos", "\\cos", false, false,
{
    assert(isfinite(val1));

    return cos(val1);
},
{
    assert(token);
    assert(token->valueType == ExpressionTokenValueTypeof::OPERATION);
    assert(token->value.operation == ExpressionOperationId::COS);

    return _MUL(CONST_TOKEN(-1), 
                      _MUL(_SIN(C(token->left)), D(token->left)));
})

GENERATE_OPERATION_CMD(TAN, PREFIX, PREFIX, true, "tan", "\\tan", false, false,
{
    assert(isfinite(val1));

    return tan(val1);
},
{
    assert(token);
    assert(token->valueType == ExpressionTokenValueTypeof::OPERATION);
    assert(token->value.operation == ExpressionOperationId::TAN);

    return _DIV(D(token->left), 
                      _POW(_COS(C(token->left)), CONST_TOKEN(2)));
})

GENERATE_OPERATION_CMD(COT, PREFIX, PREFIX, true, "cot", "\\cot", false, false,
{
    assert(isfinite(val1));

    double tan_val1 = tan(val1);

    assert(!DoubleEqual(tan_val1, 0));
    assert(isfinite(tan_val1));

    return 1 / tan_val1;
},
{
    assert(token);
    assert(token->valueType == ExpressionTokenValueTypeof::OPERATION);
    assert(token->value.operation == ExpressionOperationId::COT);

    return _MUL(CONST_TOKEN(-1), 
                      _DIV(D(token->left), 
                                 _POW(_SIN(C(token->left)), CONST_TOKEN(2))));
})

GENERATE_OPERATION_CMD(ARCSIN, PREFIX, PREFIX, true, "arcsin", "\\arcsin", false, false,
{
    assert(isfinite(val1));

    return asin(val1);
},
{
    assert(token);
    assert(token->valueType == ExpressionTokenValueTypeof::OPERATION);
    assert(token->value.operation == ExpressionOperationId::ARCSIN);

    return _DIV(D(token->left),
                      _POW(_SUB(CONST_TOKEN(1), 
                                            _POW(C(token->left), CONST_TOKEN(2))),
                                 CONST_TOKEN(0.5)));
})

GENERATE_OPERATION_CMD(ARCCOS, PREFIX, PREFIX, true, "arccos", "\\arccos", false, false,
{
    assert(isfinite(val1));

    return acos(val1);
},
{
    assert(token);
    assert(token->valueType == ExpressionTokenValueTypeof::OPERATION);
    assert(token->value.operation == ExpressionOperationId::ARCCOS);

    return _DIV(CONST_TOKEN(-1),
                      _MUL(D(token->left),
                                 _POW(_SUB(CONST_TOKEN(1), 
                                                       _POW(C(token->left), CONST_TOKEN(2))),
                                            CONST_TOKEN(0.5))));
})

GENERATE_OPERATION_CMD(ARCTAN, PREFIX, PREFIX, true, "arctan", "\\arctan", false, false,
{
    assert(isfinite(val1));

    return atan(val1);
},
{
    assert(token);
    assert(token->valueType == ExpressionTokenValueTypeof::OPERATION);
    assert(token->value.operation == ExpressionOperationId::ARCTAN);

    return _DIV(D(token->left), 
                      _ADD(CONST_TOKEN(1),
                                 _POW(C(token->left), CONST_TOKEN(2))));
})

GENERATE_OPERATION_CMD(ARCCOT, PREFIX, PREFIX, true, "arccot", "\\arccot", false, false,
{
    assert(isfinite(val1));

    return PI / 2 - atan(val1);
},
{
    assert(token);
    assert(token->valueType == ExpressionTokenValueTypeof::OPERATION);
    assert(token->value.operation == ExpressionOperationId::ARCCOT);

    return _MUL(CONST_TOKEN(-1),
                      _DIV(D(token->left),
                                 _ADD(CONST_TOKEN(1),
                                            _POW(C(token->left), CONST_TOKEN(2)))));
})