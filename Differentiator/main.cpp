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

    ExpressionType expressionDifferentiate =  ExpressionDifferentiate(&expression, outputTex);
    err = ExpressionPrintTex   (&expressionDifferentiate, outputTex, "Итоговый ответ: ");

    IF_ERR_RETURN(err);
    
    //------------------------MACLOREN------------

    ExpressionType maclorenSeries = ExpressionMacloren(&expression, 5);
    err = ExpressionPrintTex   (&maclorenSeries, outputTex, "Разложение по маклорену: ");
    IF_ERR_RETURN(err);

    //-----------------------Graphs--------------

    char* imgFuncName        = nullptr;
    err = ExpressionPlotFunc(&expression, "main func", "red", &imgFuncName);
    IF_ERR_RETURN(err);

    char* imgMaclorenName    = nullptr;
    err = ExpressionPlotFunc(&maclorenSeries, "macloren", "green", &imgMaclorenName);
    IF_ERR_RETURN(err);

    char* imgFuncAndMacloren = nullptr;
    err = ExpressionPlotFuncAndMacloren(&expression, &maclorenSeries, &imgFuncAndMacloren);
    IF_ERR_RETURN(err);

    char* imgFuncAndMaclorenDifference = nullptr;
    ExpressionType expressionDifference = ExpressionSubTwoExpressions(&expression, 
                                                                      &maclorenSeries);
    err = ExpressionPlotFunc(&expressionDifference, "difference function", "blue", 
                             &imgFuncAndMaclorenDifference);
    IF_ERR_RETURN(err);

    //--------------------PRINT GRAPHS TO LATEX--------------------

    TexInsertImg(imgFuncName, outputTex, "График функции:\n");

    TexInsertImg(imgMaclorenName, outputTex, "График разложение по маклорену:\n");

    TexInsertImg(imgFuncAndMacloren, outputTex, 
                            "Сравнение графиков функции и маклорена в окрестности нуля:\n");

    TexInsertImg(imgFuncAndMaclorenDifference, outputTex,
                            "График разницы между функцией и разложение по маклорену:\n");

    LatexFileTrollingEnd(outputTex);
    fclose(outputTex);
    LatexCreatePdf(outputTexFileName);

    free(imgFuncName);
    free(imgMaclorenName);
    free(imgFuncAndMacloren);
    free(imgFuncAndMaclorenDifference);
    
}