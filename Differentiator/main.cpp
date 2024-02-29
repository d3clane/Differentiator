#include <assert.h>

#include "MathExpressionsMain.h"
#include "MathExpressionInOut.h"
#include "MathExpressionCalculations.h"
#include "MathExpressionGnuPlot.h"
#include "MathExpressionTexDump.h"
#include "MathExpressionEquationRead.h"

#include "Common/Log.h"

#define IF_ERR_RETURN(err)                  \
    if (err != ExpressionErrors::NO_ERR)    \
        return (int)err;

int main(const int argc, const char* argv[])
{
    LogOpen(argv[0]);

    setbuf(stdout, nullptr);
    
    //ExpressionParse("5 - -(2 + 3)^4");
    ExpressionParse("sin(x^2) + 2*x    - (3^(2 + 3*x)^21^(x+2))^2*16");
    //TODO: парсинг унарного минуса, очень просто за счет рекурсивного спуска просто создавать

    ExpressionErrors err = ExpressionErrors::NO_ERR;

    ExpressionType  expression = {};
    err = ExpressionCtor(&expression);
    
    IF_ERR_RETURN(err);

    static const char* outputTexFileName = "PHD.tex";
    static const char* inputFileName     = "input.txt";

    FILE* inStream  = fopen(inputFileName,     "r");
    FILE* outputTex = fopen(outputTexFileName, "w");

    LatexFileTrollingStart(outputTex);

    err = ExpressionReadPrefixFormat(&expression, inStream);
    IF_ERR_RETURN(err);
    err = ExpressionReadVariables(&expression);
    IF_ERR_RETURN(err);

    //-----------------------DIFFERENTIATE------------

    LaTexStartNewSection("Derivative", outputTex);
    ExpressionType expressionDifferentiate =  ExpressionDifferentiate(&expression, outputTex);
    IF_ERR_RETURN(err);
    
    //-----------------------TANGENT--------------------

    LaTexStartNewSection("Tangent", outputTex);
    ExpressionType tangent = ExpressionTangent(&expression, 0);
    err = ExpressionPrintTex(&tangent, outputTex, "Tangent in 0: ");
    IF_ERR_RETURN(err);

    //------------------------MACLOREN------------------

    LaTexStartNewSection("Macloren", outputTex);
    ExpressionType maclorenSeries1 = ExpressionTaylor(&expression, 1, 0);
    ExpressionType maclorenSeries3 = ExpressionTaylor(&expression, 3, 0);
    ExpressionType maclorenSeries5 = ExpressionTaylor(&expression, 5, 0);
    err = ExpressionPrintTex(&maclorenSeries5, outputTex, "Macloren series: ");
    IF_ERR_RETURN(err);

    //-----------------------Graphs--------------

    char* imgFunc        = nullptr;
    err = ExpressionPlotFunc(&expression, "main func", "red", -1.3, 1.3, &imgFunc);
    IF_ERR_RETURN(err);
    
    char* imgMacloren    = nullptr;
    err = ExpressionPlotFunc(&maclorenSeries5, "macloren o(x^5)", "green", -2, 2, &imgMacloren);
    IF_ERR_RETURN(err);

    char* imgFuncAndTangent = nullptr;
    err = ExpressionPlotTwoFuncs(&expression, "main func", "red",
                                 &tangent, "tangent", "green",
                                 -1.3, 1.3, 
                                 &imgFuncAndTangent);
    IF_ERR_RETURN(err);   

    //--------------------Macloren small range graph--------------------

    char* imgFuncAndMaclorenSmallRange = nullptr;
    double xRangeLeft  = -0.6;
    double xRangeRight =  0.6;

    char *imgFuncAndMacloren1SmallRange = nullptr;
    char *imgFuncAndMacloren3SmallRange = nullptr;
    char *imgFuncAndMacloren5SmallRange = nullptr;
    ExpressionPlotTwoFuncs(&expression, "main func", "red", 
                           &maclorenSeries1, "macloren o(x)", "green",
                           xRangeLeft, xRangeRight, &imgFuncAndMacloren1SmallRange);
    ExpressionPlotTwoFuncs(&expression, "main func", "red", 
                           &maclorenSeries3, "macloren o(x^3)", "green",
                           xRangeLeft, xRangeRight, &imgFuncAndMacloren3SmallRange);
    ExpressionPlotTwoFuncs(&expression, "main func", "red", 
                           &maclorenSeries5, "macloren o(x^5)", "green",
                           xRangeLeft, xRangeRight, &imgFuncAndMacloren5SmallRange);

    const char* plotFileName = GnuPlotFileCreate(xRangeLeft, xRangeRight, 
                                                 &imgFuncAndMaclorenSmallRange);
    ExpressionGnuPlotAddFunc(plotFileName, &expression,      "main func",           "red");
    ExpressionGnuPlotAddFunc(plotFileName, &maclorenSeries1, "macloren o(x)",    "orange");
    ExpressionGnuPlotAddFunc(plotFileName, &maclorenSeries3, "macloren o(x^3)",   "green");
    ExpressionGnuPlotAddFunc(plotFileName, &maclorenSeries5, "macloren o(x^5)",    "blue");
    GnuPlotImgCreate(plotFileName);

    //------------------Macloren large range graph-----------------

    char* imgFuncAndMaclorenLargeRange = nullptr;
    xRangeLeft  = -1.3;
    xRangeRight =  1.3;

    char *imgFuncAndMacloren1LargeRange = nullptr;
    char *imgFuncAndMacloren3LargeRange = nullptr;
    char *imgFuncAndMacloren5LargeRange = nullptr;
    ExpressionPlotTwoFuncs(&expression, "main func", "red", 
                           &maclorenSeries1, "macloren o(x)", "green",
                           xRangeLeft, xRangeRight, &imgFuncAndMacloren1LargeRange);
    ExpressionPlotTwoFuncs(&expression, "main func", "red", 
                           &maclorenSeries3, "macloren o(x^3)", "green",
                           xRangeLeft, xRangeRight, &imgFuncAndMacloren3LargeRange);
    ExpressionPlotTwoFuncs(&expression, "main func", "red", 
                           &maclorenSeries5, "macloren o(x^5)", "green",
                           xRangeLeft, xRangeRight, &imgFuncAndMacloren5LargeRange);

    plotFileName = GnuPlotFileCreate(xRangeLeft, xRangeRight, 
                                     &imgFuncAndMaclorenLargeRange);
    ExpressionGnuPlotAddFunc(plotFileName, &expression,      "main func",           "red");
    ExpressionGnuPlotAddFunc(plotFileName, &maclorenSeries1, "macloren o(x)",    "orange");
    ExpressionGnuPlotAddFunc(plotFileName, &maclorenSeries3, "macloren o(x^3)",   "green");
    ExpressionGnuPlotAddFunc(plotFileName, &maclorenSeries5, "macloren o(x^5)",    "blue");
    GnuPlotImgCreate(plotFileName);
    
    //--------------------Difference func and macloren graph----------------
    
    char* imgFuncAndMaclorenDifference = nullptr;
    ExpressionType expressionDifference = ExpressionSubTwoExpressions(&expression, 
                                                                      &maclorenSeries5);
    err = ExpressionPlotFunc(&expressionDifference, "difference function and macloren o(x^5)", 
                            "blue", -1.3, 1.3,
                             &imgFuncAndMaclorenDifference);
    IF_ERR_RETURN(err);

    //--------------------PRINT GRAPHS TO LATEX--------------------

    LaTexStartNewSection("Graphs", outputTex);
    
    LaTexInsertImg(imgFunc, outputTex, "Function graph:\n");

    LaTexInsertImg(imgMacloren, outputTex, "Macloren series graph:\n");

    LaTexInsertImg(imgFuncAndTangent, outputTex, "Main graph and tangent:\n");


    LaTexInsertImg(imgFuncAndMacloren1SmallRange, outputTex, 
                        "Comparing func graph and macloren's series o(x) graph, small range:\n");
    LaTexInsertImg(imgFuncAndMacloren3SmallRange, outputTex, 
                        "Comparing func graph and macloren's series o($x^3$) graph, small range:\n");
    LaTexInsertImg(imgFuncAndMacloren5SmallRange, outputTex, 
                        "Comparing func graph and macloren's series o($x^5$) graph, small range:\n");
    LaTexInsertImg(imgFuncAndMaclorenSmallRange, outputTex, 
                        "Comparing func graph and macloren's series graph, small range:\n");

    LaTexInsertImg(imgFuncAndMacloren1LargeRange, outputTex, 
                        "Comparing func graph and macloren's series o(x) graph, large range:\n");
    LaTexInsertImg(imgFuncAndMacloren3LargeRange, outputTex, 
                        "Comparing func graph and macloren's series o($x^3$) graph, large range:\n");
    LaTexInsertImg(imgFuncAndMacloren5LargeRange, outputTex, 
                        "Comparing func graph and macloren's series o($x^5$) graph, large range:\n");
    LaTexInsertImg(imgFuncAndMaclorenLargeRange, outputTex, 
                            "Comparing func graph and macloren's series graph, big range:\n");

    LaTexInsertImg(imgFuncAndMaclorenDifference, outputTex,
                            "Graph of the difference between main and macloren:\n");

    LatexFileTrollingEnd(outputTex);
    fclose(outputTex);
    LatexCreatePdf(outputTexFileName);

    free(imgFunc);
    free(imgMacloren);
    free(imgFuncAndMaclorenSmallRange);
    free(imgFuncAndMaclorenLargeRange);
    free(imgFuncAndMaclorenDifference);
}