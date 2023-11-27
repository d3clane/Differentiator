#ifndef GENERATE_OPERATION_CMD
#define GENERATE_OPERATION_CMD(...)
#endif

//GENERATE_OPERATION_CMD(NAME, FORMAT, TEX_FORMAT, IS_UNARY, SHORT_CUT_STRING, TEX_NAME,
//                       NEED_LEFT_TEX_BRACES, NEED_RIGHT_TEX_BRACES,                  
//                       OPERATION_CALCULATION_CODE, OPERATION_DIFF_CODE) 

//OPERATION_CALCILATION_CODE - format of function f(const double val1, const double val2)
//OPERATION_DIFF_CODE        - format of function f(const ExpressionTokenType* token)

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

GENERATE_OPERATION_CMD(ADD, INFIX,  INFIX, false, "+", "+", false, false,
{
    assert(isfinite(val1));
    assert(isfinite(val2));

    return val1 + val2;
},
{
    assert(token);
    assert(token->valueType == ExpressionTokenValueTypeof::OPERATION);
    assert(token->value.operation.operationId == ExpressionOperationsIds::ADD);

    return TOKEN(ADD, D(token->left), D(token->right));
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
    assert(token->value.operation.operationId == ExpressionOperationsIds::SUB);

    return TOKEN(SUB, D(token->left), D(token->right));
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
    assert(token->value.operation.operationId == ExpressionOperationsIds::MUL);

    return TOKEN(ADD, TOKEN(MUL, D(token->left), C(token->right)), 
                      TOKEN(MUL, C(token->left), D(token->right)));
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
    assert(token->value.operation.operationId == ExpressionOperationsIds::DIV);

    return TOKEN(DIV, TOKEN(SUB, TOKEN(MUL, D(token->left), C(token->right)), 
                                 TOKEN(MUL, C(token->left), D(token->right))),
                      TOKEN(POW, C(token->right), CONST_TOKEN(2)));
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
    assert(token->value.operation.operationId == ExpressionOperationsIds::LN);


    return TOKEN(DIV, D(token->left), C(token->left));
})

GENERATE_OPERATION_CMD(LN,  PREFIX, PREFIX, true,  "ln",  "\\ln",   false, false,
{
    assert(isfinite(val1));

    return log(val1);
},
{
    assert(token);
    assert(token->valueType == ExpressionTokenValueTypeof::OPERATION);
    assert(token->value.operation.operationId == ExpressionOperationsIds::LN);

    return TOKEN(DIV, D(token->left), C(token->left));
})

GENERATE_OPERATION_CMD(SIN, PREFIX, PREFIX, true, "sin", "\\sin", false, false,
{
    assert(isfinite(val1));

    return sin(val1);
},
{
    assert(token);
    assert(token->valueType == ExpressionTokenValueTypeof::OPERATION);
    assert(token->value.operation.operationId == ExpressionOperationsIds::SIN);

    return TOKEN(MUL, UNARY_TOKEN(COS, C(token->left)), D(token->left));
})

GENERATE_OPERATION_CMD(COS, PREFIX, PREFIX, true, "cos", "\\cos", false, false,
{
    assert(isfinite(val1));

    return cos(val1);
},
{
    assert(token);
    assert(token->valueType == ExpressionTokenValueTypeof::OPERATION);
    assert(token->value.operation.operationId == ExpressionOperationsIds::COS);

    return TOKEN(MUL, CONST_TOKEN(-1), 
                      TOKEN(MUL, UNARY_TOKEN(SIN, C(token->left)), D(token->left)));
})

GENERATE_OPERATION_CMD(TAN, PREFIX, PREFIX, true, "tan", "\\tan", false, false,
{
    assert(isfinite(val1));

    return tan(val1);
},
{
    assert(token);
    assert(token->valueType == ExpressionTokenValueTypeof::OPERATION);
    assert(token->value.operation.operationId == ExpressionOperationsIds::TAN);

    return TOKEN(DIV, D(token->left), 
                      TOKEN(POW, UNARY_TOKEN(COS, C(token->left)), CONST_TOKEN(2)));
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
    assert(token->value.operation.operationId == ExpressionOperationsIds::COT);

    return TOKEN(MUL, CONST_TOKEN(-1), 
                      TOKEN(DIV, D(token->left), 
                                 TOKEN(POW, UNARY_TOKEN(SIN, C(token->left)), CONST_TOKEN(2))));
})

GENERATE_OPERATION_CMD(ARCSIN, PREFIX, PREFIX, true, "arcsin", "\\arcsin", false, false,
{
    assert(isfinite(val1));

    return asin(val1);
},
{
    assert(token);
    assert(token->valueType == ExpressionTokenValueTypeof::OPERATION);
    assert(token->value.operation.operationId == ExpressionOperationsIds::ARCSIN);

    return TOKEN(DIV, D(token->left),
                      TOKEN(POW, TOKEN(SUB, CONST_TOKEN(1), 
                                            TOKEN(POW, C(token->left), CONST_TOKEN(2))),
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
    assert(token->value.operation.operationId == ExpressionOperationsIds::ARCCOS);

    return TOKEN(DIV, CONST_TOKEN(-1),
                      TOKEN(MUL, D(token->left),
                                 TOKEN(POW, TOKEN(SUB, CONST_TOKEN(1), 
                                                       TOKEN(POW, C(token->left), CONST_TOKEN(2))),
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
    assert(token->value.operation.operationId == ExpressionOperationsIds::ARCCOS);

    return TOKEN(DIV, CONST_TOKEN(-1),
                      TOKEN(MUL, D(token->left),
                                 TOKEN(POW, TOKEN(SUB, CONST_TOKEN(1), 
                                                       TOKEN(POW, C(token->left), CONST_TOKEN(2))),
                                            CONST_TOKEN(0.5))));
})

GENERATE_OPERATION_CMD(ARCCOT, PREFIX, PREFIX, true, "arccot", "\\arccot", false, false,
{
    assert(isfinite(val1));

    return PI / 2 - atan(val1);
},
{
    assert(token);
    assert(token->valueType == ExpressionTokenValueTypeof::OPERATION);
    assert(token->value.operation.operationId == ExpressionOperationsIds::ARCTAN);

    return TOKEN(MUL, CONST_TOKEN(-1),
                      TOKEN(DIV, D(token->left),
                                 TOKEN(ADD, CONST_TOKEN(1),
                                            TOKEN(POW, C(token->left), CONST_TOKEN(2)))));
})