#include "MathExpressionEquationRead.h"

// G       ::= ADD_SUB '\0'
// ADD_SUB ::= MUL_DIV {[+-] MUL_DIV}*
// MUL_DIV ::= POW