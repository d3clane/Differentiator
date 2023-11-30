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
    ExpressionType taylorSeries = ExpressionTaylor(&expression, 5, 0);
    err = ExpressionPrintTex(&taylorSeries, outputTex, "Macloren series: ");
    IF_ERR_RETURN(err);

    //-----------------------Graphs--------------

    char* imgFunc        = nullptr;
    err = ExpressionPlotFunc(&expression, "main func", "red", &imgFunc);
    IF_ERR_RETURN(err);

    char* imgMacloren    = nullptr;
    err = ExpressionPlotFunc(&taylorSeries, "macloren", "green", &imgMacloren);
    IF_ERR_RETURN(err);

    char* imgFuncAndMacloren = nullptr;
    err = ExpressionPlotTwoFuncs(&expression, "main func", "red", 
                                 &taylorSeries, "macloren", "green", 
                                 &imgFuncAndMacloren);
    IF_ERR_RETURN(err);

    char* imgFuncAndTangent = nullptr;
    err = ExpressionPlotTwoFuncs(&expression, "main func", "red",
                                 &tangent, "tangent", "green", 
                                 &imgFuncAndTangent);
    IF_ERR_RETURN(err);   
    
    char* imgFuncAndMaclorenDifference = nullptr;
    ExpressionType expressionDifference = ExpressionSubTwoExpressions(&expression, 
                                                                      &taylorSeries);
    err = ExpressionPlotFunc(&expressionDifference, "difference function", "blue", 
                             &imgFuncAndMaclorenDifference);
    IF_ERR_RETURN(err);

    //--------------------PRINT GRAPHS TO LATEX--------------------

    LaTexStartNewSection("Graphs", outputTex);
    
    LaTexInsertImg(imgFunc, outputTex, "Function graph:\n");

    LaTexInsertImg(imgMacloren, outputTex, "Macloren series graph:\n");

    LaTexInsertImg(imgFuncAndTangent, outputTex, "Main graph and tangent:\n");

    LaTexInsertImg(imgFuncAndMacloren, outputTex, 
                            "Comparing func graph and macloren's series graph:\n");

    LaTexInsertImg(imgFuncAndMaclorenDifference, outputTex,
                            "Graph of the difference between main and macloren:\n");

    LatexFileTrollingEnd(outputTex);
    fclose(outputTex);
    LatexCreatePdf(outputTexFileName);

    free(imgFunc);
    free(imgMacloren);
    free(imgFuncAndMacloren);
    free(imgFuncAndMaclorenDifference);
    
}