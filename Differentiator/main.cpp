#include "Differentiator.h"
#include "../Common/Log.h"

int main(const int argc, const char* argv[])
{
    setbuf(stdout, nullptr);
    LogOpen(argv[0]);

    DiffTreeType diffTree;
    DiffCtor(&diffTree);

    FILE* inStream = fopen("input.txt", "r");
    DiffReadPrefixFormat(&diffTree, inStream);

    DiffPrintPrefixFormat     (&diffTree);
    DiffPrintEquationFormat   (&diffTree);
    DiffPrintEquationFormatTex(&diffTree);
    DiffGraphicDump(&diffTree, false);

    printf("%lf", DiffCalculate(&diffTree));
}