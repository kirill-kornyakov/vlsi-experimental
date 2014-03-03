#include "Spreading.h"

#include <vector>

extern timetype calcPotentialsTime;
extern timetype quadraticSpreading;
extern timetype quadraticSpreadingGradTime;

using namespace std;

void MoveBinIndexesIntoBorders(AppCtx* context, int& min_col, int& min_row, int& max_col, int& max_row)
{
    BinGrid& binGrid = context->sprData.binGrid;

    min_col = std::min(std::max(0, min_col), binGrid.nBinCols-1);
    min_row = std::min(std::max(0, min_row), binGrid.nBinRows-1);
    max_col = std::max(0, std::min(max_col, binGrid.nBinCols-1));
    max_row = std::max(0, std::min(max_row, binGrid.nBinRows-1));
}

void DetermineBordersOfClusterPotential(BinGrid& binGrid, AppCtx* context,
                                        int& min_col, int& max_col, 
                                        int& min_row, int& max_row,
                                        double x, double y, 
                                        double potentialRadiusX, double potentialRadiusY)
{
    min_col = (Aux::cool_dtoi(
        (x - context->hd->Circuit.PlacementMinX() - potentialRadiusX) / binGrid.binWidth));
    max_col = (Aux::cool_dtoi(
        (x - context->hd->Circuit.PlacementMinX() + potentialRadiusX) / binGrid.binWidth));
    min_row = (Aux::cool_dtoi(
        (y - context->hd->Circuit.PlacementMinY() - potentialRadiusY) / binGrid.binHeight));    
    max_row = (Aux::cool_dtoi(
        (y - context->hd->Circuit.PlacementMinY() + potentialRadiusY) / binGrid.binHeight));

    MoveBinIndexesIntoBorders(context, min_col, min_row, max_col, max_row);
}

double CalcBellShapedFunction(AppCtx* context, int solutionIdx, int clusterIdx, 
                              int colIdx, int rowIdx, PetscScalar* x)
{
    double potentialRadiusX = context->sprData.potentialRadiusX;
    double potentialRadiusY = context->sprData.potentialRadiusY;
    BinGrid& binGrid = context->sprData.binGrid;
    double invPSX = context->sprData.invPSX;
    double invPSY = context->sprData.invPSY;

    double potX = 0;
    double potY = 0;

    double dx = x[2*solutionIdx+0] - binGrid.bins[rowIdx][colIdx].xCoord;
    double dy = x[2*solutionIdx+1] - binGrid.bins[rowIdx][colIdx].yCoord;

    double fabsdx = fabs(dx);
    double fabsdy = fabs(dy);

    if (fabsdx > potentialRadiusX || fabsdy > potentialRadiusY)
    {//bin out of potential
        return 0;
    }

    //calculate solution-potential
    if (fabsdx <= potentialRadiusX/2)
    {
        potX = 1 - 2 * dx * dx * invPSX;
    }
    else
    {
        potX = 2 * (fabsdx - potentialRadiusX) * (fabsdx - potentialRadiusX) * invPSX;
    }

    //calculate y-potential
    if (fabsdy <= potentialRadiusY/2)
    {
        potY = 1 - 2 * dy * dy * invPSY;
    }
    else
    {
        potY = 2 * (fabsdy - potentialRadiusY) * (fabsdy - potentialRadiusY) * invPSY;
    }

    return potX * potY;
}

//REFACTOR: use CalcBellShapedFunc() function
void CalcBellShapedFuncAndDerivative(AppCtx* context, int solutionIdx, int clusterIdx, 
                                     int colIdx, int rowIdx,
                                     PetscScalar* solution,
                                     double &gradX, double &gradY)
{
    double potX  = 0;
    gradX = 0;
    double _potX = 0;
    double potY  = 0;
    gradY = 0;
    double _potY = 0;

    double potentialRadiusX = context->sprData.potentialRadiusX;
    double potentialRadiusY = context->sprData.potentialRadiusY;
    BinGrid& binGrid = context->sprData.binGrid;
    double invPSX = context->sprData.invPSX;
    double invPSY = context->sprData.invPSY;

    double dx = solution[2*solutionIdx+0] - binGrid.bins[rowIdx][colIdx].xCoord;
    double dy = solution[2*solutionIdx+1] - binGrid.bins[rowIdx][colIdx].yCoord;

    double fabsdx = fabs(dx);
    double fabsdy = fabs(dy);
    double sigx4 = 4 * Aux::sign(dx);
    double sigy4 = 4 * Aux::sign(dy);

    double multiplier;
    double residual;

    if (fabsdx > potentialRadiusX || fabsdy > potentialRadiusY)
    {//cluster doesn't affect this bin
        return;
    } 

    // calculate x-potential
    if (2 * fabsdx <= potentialRadiusX)
    {
        potX  = 1 - 2 * dx * dx * invPSX;
        _potX = -sigx4 * fabsdx * invPSX;
    }
    else
    {
        potX  = 2 * (fabsdx - potentialRadiusX) * (fabsdx - potentialRadiusX) * invPSX;
        _potX = sigx4 * (fabsdx - potentialRadiusX) * invPSX;
    }

    // calculate y-potential
    if (2 * fabsdy <= potentialRadiusY)
    {
        potY  = 1 - 2 * dy * dy * invPSY;
        _potY = -sigy4 * fabsdy * invPSY;
    }
    else
    {
        potY  = 2 * (fabsdy - potentialRadiusY) * (fabsdy - potentialRadiusY) * invPSY;
        _potY = sigy4 * (fabsdy - potentialRadiusY) * invPSY;
    }

    residual = binGrid.bins[rowIdx][colIdx].sumPotential - context->sprData.desiredCellsAreaAtEveryBin;
    if (context->sprData.useUnidirectSpreading)
    {
        if (residual > 0.0)
            multiplier = 2 * residual * context->ci->clusters[clusterIdx].potentialMultiplier;
        else
            multiplier = 0.0;
    }
    else
        multiplier = 2 * residual * context->ci->clusters[clusterIdx].potentialMultiplier;

    gradX = multiplier * _potX * potY;
    gradY = multiplier * _potY * potX;
}

// calculate every cluster's potential
void CalculatePotentials(AppCtx* context, PetscScalar* x)
{
    BinGrid& binGrid = context->sprData.binGrid;

    //null the penalties
    double sum = 0;
    double sum2 = 0;
    if (context->hd->cfg.ValueOf(".UseBuffering", false))
    {
        for (int i = 0; i < binGrid.nBinRows; ++i)
        {
            for (int j = 0; j < binGrid.nBinCols; ++j)
            {
                binGrid.bins[i][j].sumPotential = binGrid.bins[i][j].sumBufPotential;
                sum += binGrid.bins[i][j].sumBufPotential;
            }
        }
    }
    else
    {
        for (int i = 0; i < binGrid.nBinRows; ++i)
        {
            for (int j = 0; j < binGrid.nBinCols; ++j)
            {
                binGrid.bins[i][j].sumPotential = 0.0;
            }
        }
    }

    // each cell must have total potential equal to its area
    // this variable are needed to control this issue
    double currClusterTotalPotential = 0.0;

    // we take every cluster and consider only bins 
    // which are affected by this cluster potential
    int idxInSolutionVector = 0;
    int clusterIdx = -1;
    int min_row, min_col, max_row, max_col; // area affected by cluster potential 

    while (GetNextActiveClusterIdx(context->ci, clusterIdx))
    {
        idxInSolutionVector = context->clusterIdx2solutionIdxLUT[clusterIdx];
        currClusterTotalPotential = 0.0;

        DetermineBordersOfClusterPotential(binGrid, context, min_col, max_col, min_row, max_row, 
            x[2*idxInSolutionVector+0], x[2*idxInSolutionVector+1], 
            context->sprData.potentialRadiusX, context->sprData.potentialRadiusY);

        // loop over affected bins - now we just precalculate potential
        // later we will scale it so that currClusterTotalPotential = cluster area
        //#pragma omp parallel for reduction(+:currClusterTotalPotential)
        for (int rowIdx = min_row; rowIdx <= max_row; ++rowIdx)
        {
            for (int colIdx = min_col; colIdx <= max_col; ++colIdx)
            {
                double bsf = CalcBellShapedFunction(context, idxInSolutionVector, clusterIdx, colIdx, rowIdx, x);

                context->sprData.clusterPotentialOverBins[rowIdx-min_row][colIdx-min_col] = bsf;
                currClusterTotalPotential += bsf;
            }
        }

        // scale the potential
        if (currClusterTotalPotential != 0)
        {
            context->ci->clusters[clusterIdx].potentialMultiplier = 
                context->ci->clusters[clusterIdx].area / currClusterTotalPotential;
        } 
        else
        {
            context->ci->clusters[clusterIdx].potentialMultiplier = 0;
        }

        // add scaled cluster potential 
        for (int k = min_row; k <= max_row; ++k)
            for (int j = min_col; j <= max_col; ++j)
            {
                binGrid.bins[k][j].sumPotential += 
                    context->ci->clusters[clusterIdx].potentialMultiplier * 
                    context->sprData.clusterPotentialOverBins[k-min_row][j-min_col];
                sum2 += context->ci->clusters[clusterIdx].potentialMultiplier * 
                    context->sprData.clusterPotentialOverBins[k-min_row][j-min_col];
            }
    }

    double totalPotential = 0.0;
    for (int i = 0; i < binGrid.nBinRows; ++i)
    {
        for (int j = 0; j < binGrid.nBinCols; ++j)
        {
            totalPotential += binGrid.bins[i][j].sumPotential;
        }
    }

    if (context->hd->cfg.ValueOf(".UseBuffering", false))
        if (context->hd->cfg.ValueOf(".New_Buffering.Reporting.PrintSumPotencial", false))
        {
            ALERT("Total sum potential + buffer potential   = %f", totalPotential);
            ALERT("Total buffer potential = %f", sum);
            ALERT("Total sum potential = %f", sum2);
            ALERT("(Total buffer potential)/(Total sum potential) = %f", sum / sum2);
        }
}

double SpreadingPenalty(AppCtx* context, PetscScalar* x)
{
    timetype start_local, finish_local;
    start_local = GET_TIME_METHOD();
    CalculatePotentials(context, x);
    finish_local = GET_TIME_METHOD();
    calcPotentialsTime += finish_local - start_local;

    double spreadingPenalty = 0.0;
    double residual;

    BinGrid& binGrid = context->sprData.binGrid;

    for (int i = 0; i < binGrid.nBinRows; ++i)
    {
        for (int j = 0; j < binGrid.nBinCols; ++j)
        {
            residual = binGrid.bins[i][j].sumPotential - context->sprData.desiredCellsAreaAtEveryBin;
            if (context->sprData.useUnidirectSpreading)
            {
                if (residual > 0.0)
                {
                    spreadingPenalty += residual * residual;
                }
            }
            else
            {
                spreadingPenalty += residual * residual;
            }
        }
    }

    return spreadingPenalty;
}

void AddSpreadingPenaltyGradient(AppCtx* context, PetscScalar* x, PetscScalar* grad)
{
    int idxInSolutionVector = 0;
    int clusterIdx = -1;

    while (GetNextActiveClusterIdx(context->ci, clusterIdx))
    {
        idxInSolutionVector = context->clusterIdx2solutionIdxLUT[clusterIdx];

        int min_row, min_col, max_row, max_col; // area affected by cluster potential 
        DetermineBordersOfClusterPotential(context->sprData.binGrid, context, min_col, max_col, min_row, max_row, 
            x[2*idxInSolutionVector+0], x[2*idxInSolutionVector+1], 
            context->sprData.potentialRadiusX, context->sprData.potentialRadiusY);

        double gX = 0;
        double gY = 0;

        //#pragma omp parallel for reduction(+:gX) reduction(+:gY)
        for (int rowIdx = min_row; rowIdx <= max_row; ++rowIdx)
        {
            for (int colIdx = min_col; colIdx <= max_col; ++colIdx)
            {
                double gradX;
                double gradY;
                CalcBellShapedFuncAndDerivative(context, idxInSolutionVector, clusterIdx, colIdx, rowIdx, x, 
                    /*potX,*/ gradX, /*potY,*/ gradY);
                gX += gradX;
                gY += gradY;
            }
        }

        grad[2*idxInSolutionVector+0] = gX;
        grad[2*idxInSolutionVector+1] = gY;
    }
}

void QS_AddObjectiveAndGradient(AppCtx* context, PetscScalar * solution)
{
    timetype start = GET_TIME_METHOD();
    context->criteriaValues.spr = SpreadingPenalty(context, solution);
    timetype finish = GET_TIME_METHOD();
    quadraticSpreading += finish - start;

    start = GET_TIME_METHOD();
    AddSpreadingPenaltyGradient(context, solution, context->criteriaValues.gQS);
    finish = GET_TIME_METHOD();
    quadraticSpreadingGradTime += finish - start;
}

int CalcMaxAffectedArea(double potentialSize, double binSize)
{
    return 1+2*static_cast<int>(ceil(potentialSize/binSize));
}