#include "DoubleFuncs.h"

static const double eps = 1e-7;

bool DoubleEqual(double a, double b)
{
    return (a - b) < eps && (b - a) < eps;
}
