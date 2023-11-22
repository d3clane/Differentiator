#include "Differentiator.h"
#include "../Common/Log.h"

int main(const int argc, const char* argv[])
{
    setbuf(stdout, nullptr);
    LogOpen(argv[0]);

    DiffTreeType diffTreePrefixInput;
    DiffTreeType diffTreeInfixInput;
    DiffCtor(&diffTreePrefixInput);
    DiffCtor(&diffTreeInfixInput);

    FILE* inStreamPrefix = fopen("input.txt",  "r");
    FILE* inStreamInfix  = fopen("input2.txt", "r");
    DiffReadPrefixFormat(&diffTreePrefixInput, inStreamPrefix);
    DiffReadInfixFormat(&diffTreeInfixInput,  inStreamInfix);

    DiffReadVariables(&diffTreePrefixInput);

    DiffPrintPrefixFormat     (&diffTreePrefixInput);
    DiffPrintEquationFormat   (&diffTreePrefixInput);
    DiffPrintEquationFormatTex(&diffTreePrefixInput);
    DiffGraphicDump(&diffTreePrefixInput);
    DiffGraphicDump(&diffTreeInfixInput);
    printf("\n\n");
    DiffPrintEquationFormat(&diffTreeInfixInput);

    //printf("Calculation result: %lf", DiffCalculate(&diffTreePrefixInput));
}