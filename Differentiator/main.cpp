#include <assert.h>

#include "MathExpressionsMain.h"
#include "MathExpressionInOut.h"
#include "MathExpressionCalculations.h"
#include "MathExpressionGnuPlot.h"
#include "MathExpressionTexDump.h"

#include "Common/Log.h"

#define IF_ERR_RETURN(err)                  \
    if (err != ExpressionErrors::NO_ERR)    \
        return (int)err;

int main(const int argc, const char* argv[])
{
    LogOpen(argv[0]);

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
    err = ExpressionPrintTex   (&expressionDifferentiate, outputTex, 
                                                    "Result differentiate answer: ");
    IF_ERR_RETURN(err);
    
    //-----------------------TANGENT--------------------

    LaTexStartNewSection("Tangent", outputTex);
    ExpressionType tangent = ExpressionTangent(&expression, 0);
    err = ExpressionPrintTex(&tangent, outputTex, "Tangent in 0: ");
    IF_ERR_RETURN(err);

    //------------------------MACLOREN------------------

    LaTexStartNewSection("Macloren", outputTex);
    ExpressionType maclorenSeries = ExpressionTaylor(&expression, 5, 0);
    err = ExpressionPrintTex(&maclorenSeries, outputTex, "Macloren series: ");
    IF_ERR_RETURN(err);

    //-----------------------Graphs--------------

    char* imgFunc        = nullptr;
    err = ExpressionPlotFunc(&expression, "main func", "red", -1.3, 1.3, &imgFunc);
    IF_ERR_RETURN(err);
    
    char* imgMacloren    = nullptr;
    err = ExpressionPlotFunc(&maclorenSeries, "macloren", "green", -2, 2, &imgMacloren);
    IF_ERR_RETURN(err);

    char* imgFuncAndTangent = nullptr;
    err = ExpressionPlotTwoFuncs(&expression, "main func", "red",
                                 &tangent, "tangent", "green",
                                 -1.3, 1.3, 
                                 &imgFuncAndTangent);
    IF_ERR_RETURN(err);   

    char* imgFuncAndMaclorenSmallRange = nullptr;
    err = ExpressionPlotTwoFuncs(&expression, "main func", "red", 
                                 &maclorenSeries, "macloren", "green", 
                                 -0.6, 0.6,
                                 &imgFuncAndMaclorenSmallRange);
    IF_ERR_RETURN(err);

    char* imgFuncAndMaclorenLargeRange = nullptr;
    err = ExpressionPlotTwoFuncs(&expression, "main func", "red", 
                                 &maclorenSeries, "macloren", "green", 
                                 -1.3, 1.3,
                                 &imgFuncAndMaclorenLargeRange);
    IF_ERR_RETURN(err);
    
    char* imgFuncAndMaclorenDifference = nullptr;
    ExpressionType expressionDifference = ExpressionSubTwoExpressions(&expression, 
                                                                      &maclorenSeries);
    err = ExpressionPlotFunc(&expressionDifference, "difference function", "blue", 
                             -1.3, 1.3,
                             &imgFuncAndMaclorenDifference);
    IF_ERR_RETURN(err);

    //--------------------PRINT GRAPHS TO LATEX--------------------

    LaTexStartNewSection("Graphs", outputTex);
    
    LaTexInsertImg(imgFunc, outputTex, "Function graph:\n");

    LaTexInsertImg(imgMacloren, outputTex, "Macloren series graph:\n");

    LaTexInsertImg(imgFuncAndTangent, outputTex, "Main graph and tangent:\n");

    LaTexInsertImg(imgFuncAndMaclorenSmallRange, outputTex, 
                            "Comparing func graph and macloren's series graph, small range:\n");

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