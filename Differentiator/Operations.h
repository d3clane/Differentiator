#ifndef GENERATE_OPERATION_CMD
#define GENERATE_OPERATION_CMD(...)
#endif

GENERATE_OPERATION_CMD(ADD, INFIX,  INFIX, false, "+", "+",      false, false)
GENERATE_OPERATION_CMD(SUB, INFIX,  INFIX, false, "-", "-",      false, false)
GENERATE_OPERATION_CMD(MUL, INFIX,  INFIX, false, "*", "\\cdot", false, false)
GENERATE_OPERATION_CMD(DIV, INFIX, PREFIX, false, "/", "\\frac", true,  true)

GENERATE_OPERATION_CMD(POW, INFIX, INFIX, false,     "^",     "^",  false, true)
GENERATE_OPERATION_CMD(LOG, PREFIX, PREFIX, false, "log", "\\log_", true, false)
GENERATE_OPERATION_CMD(LN,  PREFIX, PREFIX, true,  "ln",  "\\ln",   false, false)

GENERATE_OPERATION_CMD(SIN, PREFIX, PREFIX, true, "sin", "\\sin", false, false)
GENERATE_OPERATION_CMD(COS, PREFIX, PREFIX, true, "cos", "\\cos", false, false)
GENERATE_OPERATION_CMD(TAN, PREFIX, PREFIX, true, "tan", "\\tan", false, false)
GENERATE_OPERATION_CMD(COT, PREFIX, PREFIX, true, "cot", "\\cot", false, false)

GENERATE_OPERATION_CMD(ARCSIN, PREFIX, PREFIX, true, "arcsin", "\\arcsin", false, false)
GENERATE_OPERATION_CMD(ARCCOS, PREFIX, PREFIX, true, "arccos", "\\arccos", false, false)
GENERATE_OPERATION_CMD(ARCTAN, PREFIX, PREFIX, true, "arctan", "\\arctan", false, false)
GENERATE_OPERATION_CMD(ARCCOT, PREFIX, PREFIX, true, "arccot", "\\arccot", false, false)

