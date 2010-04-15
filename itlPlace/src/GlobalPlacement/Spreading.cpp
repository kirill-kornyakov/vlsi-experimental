#include "Spreading.h"
#include <vector>

extern timetype calcPotentialsTime;
extern timetype quadraticSpreading;
extern timetype quadraticSpreadingGradTime;

using namespace std;

void MoveBinIndexesIntoBorders(AppCtx* context, int& min_col, int& min_row, int& max_col, int& max_row)
{
  BinGrid& binGrid = context->spreadingData.binGrid;

  min_col = std::min(std::max(0, min_col), binGrid.nBinCols-1);
  min_row = std::min(std::max(0, min_row), binGrid.nBinRows-1);
  max_col = std::max(0, std::min(max_col, binGrid.nBinCols-1));
  max_row = std::max(0, std::min(max_row, binGrid.nBinRows-1));
}

void DetermineBordersOfClusterPotential(int& min_col, int& max_col, 
                                        int& min_row, int& max_row,
                                        int i, PetscScalar* x, AppCtx* context)
{
  BinGrid& binGrid = context->spreadingData.binGrid;

  min_col = (Aux::cool_dtoi((x[2*i+0] - context->hd->Circuit.PlacementMinX() - 
    context->spreadingData.potentialRadiusX) / binGrid.binWidth));
  max_col = (Aux::cool_dtoi((x[2*i+0] - context->hd->Circuit.PlacementMinX() + 
    context->spreadingData.potentialRadiusX) / binGrid.binWidth));
  min_row = (Aux::cool_dtoi((x[2*i+1] - context->hd->Circuit.PlacementMinY() - 
    context->spreadingData.potentialRadiusY) / binGrid.binHeight));    
  max_row = (Aux::cool_dtoi((x[2*i+1] - context->hd->Circuit.PlacementMinY() + 
    context->spreadingData.potentialRadiusY) / binGrid.binHeight));

  MoveBinIndexesIntoBorders(context, min_col, min_row, max_col, max_row);
}

double CalcBellShapedFunction(AppCtx* context, int solutionIdx, int clusterIdx, 
                              int colIdx, int rowIdx, PetscScalar* x)
{
  double potentialRadiusX = context->spreadingData.potentialRadiusX;
  double potentialRadiusY = context->spreadingData.potentialRadiusY;
  BinGrid& binGrid = context->spreadingData.binGrid;
  double invPSX = context->spreadingData.invPSX;
  double invPSY = context->spreadingData.invPSY;

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

  double potentialRadiusX = context->spreadingData.potentialRadiusX;
  double potentialRadiusY = context->spreadingData.potentialRadiusY;
  BinGrid& binGrid = context->spreadingData.binGrid;
  double invPSX = context->spreadingData.invPSX;
  double invPSY = context->spreadingData.invPSY;

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

  residual = binGrid.bins[rowIdx][colIdx].sumPotential - context->spreadingData.desiredCellsAreaAtEveryBin;
  if (context->useUnidirectSpreading)
  {
    if (residual > 0.0)
      multiplier = 2 * residual *
      context->ci->clusters[clusterIdx].potentialMultiplier;
    else
      multiplier = 0.0;
  }
  else
    multiplier = 2 * residual *
    context->ci->clusters[clusterIdx].potentialMultiplier;

  gradX = multiplier * _potX * potY;
  gradY = multiplier * _potY * potX;
}

// calculate every cluster's potential
void CalculatePotentials(AppCtx* context, PetscScalar* x)
{
  BinGrid& binGrid = context->spreadingData.binGrid;

  //null the penalties
  for (int i = 0; i < binGrid.nBinRows; ++i)
  {
    for (int j = 0; j < binGrid.nBinCols; ++j)
    {
      binGrid.bins[i][j].sumPotential = 0.0;
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

    DetermineBordersOfClusterPotential(min_col, max_col, min_row, max_row, idxInSolutionVector, x, context);

    // loop over affected bins - now we just precalculate potential
    // later we will scale it so that currClusterTotalPotential = cluster area
    //#pragma omp parallel for reduction(+:currClusterTotalPotential)
    for (int rowIdx = min_row; rowIdx <= max_row; ++rowIdx)
    {
      for (int colIdx = min_col; colIdx <= max_col; ++colIdx)
      {
        double bsf = CalcBellShapedFunction(context, idxInSolutionVector, clusterIdx, colIdx, rowIdx, x);

        context->spreadingData.clusterPotentialOverBins[rowIdx-min_row][colIdx-min_col] = bsf;
        currClusterTotalPotential += bsf;
      }
    }// loop over affected bins

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
        binGrid.bins[k][j].sumPotential += 
        context->ci->clusters[clusterIdx].potentialMultiplier * 
        context->spreadingData.clusterPotentialOverBins[k-min_row][j-min_col];
  }

  double totalPotential = 0.0;
  for (int i = 0; i < binGrid.nBinRows; ++i)
  {
    for (int j = 0; j < binGrid.nBinCols; ++j)
    {
      totalPotential += binGrid.bins[i][j].sumPotential;
    }
  }
  //ALERT("Total Potential   = %f", totalPotential));
  //ALERT("Desired Potential = %f", 
  //  context->desiredCellsAreaAtEveryBin * context->binGrid.nBins));
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

  BinGrid& binGrid = context->spreadingData.binGrid;

  if (context->useLRSpreading)
  {
    int binIdx;

    for (int i = 0; i < binGrid.nBinRows; ++i)
    {
      for (int j = 0; j < binGrid.nBinCols; ++j)
      {
        binIdx = i * binGrid.nBinCols + j;
        residual = binGrid.bins[i][j].sumPotential - context->spreadingData.desiredCellsAreaAtEveryBin;

        if (context->useUnidirectSpreading)
        {
          if (residual > 0.0)
          {
            context->spreadingData.binsPenaltyValues[binIdx] = residual * residual;
          }
        }
        else
        {
          context->spreadingData.binsPenaltyValues[binIdx] = residual * residual;
        }

        spreadingPenalty += context->spreadingData.binsPenaltyValues[binIdx];
      }
    } 
  }
  else
  {
    for (int i = 0; i < binGrid.nBinRows; ++i)
    {
      for (int j = 0; j < binGrid.nBinCols; ++j)
      {
        residual = binGrid.bins[i][j].sumPotential - context->spreadingData.desiredCellsAreaAtEveryBin;
        if (context->useUnidirectSpreading)
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
  }

  return spreadingPenalty;
}

void AddSpreadingPenaltyGradient(AppCtx* context, PetscScalar* x, PetscScalar* grad)
{
  int idxInSolutionVector = 0;
  int clusterIdx = -1;
  double muBinsPen;

  if (context->useLRSpreading)
  {
    while (GetNextActiveClusterIdx(context->ci, clusterIdx))
    {
      idxInSolutionVector = context->clusterIdx2solutionIdxLUT[clusterIdx];

      int min_row, min_col, max_row, max_col; // area affected by cluster potential 
      DetermineBordersOfClusterPotential(min_col, max_col, min_row, max_row, idxInSolutionVector, x, context);

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
            gradX, gradY);

          muBinsPen = context->spreadingData.muBinsPen[rowIdx * context->spreadingData.binGrid.nBinCols + colIdx];
          gX += muBinsPen * gradX;
          gY += muBinsPen * gradY;
        }
      }

      grad[2*idxInSolutionVector+0] = gX;
      grad[2*idxInSolutionVector+1] = gY;
    }
  }
  else
  {
    while (GetNextActiveClusterIdx(context->ci, clusterIdx))
    {
      idxInSolutionVector = context->clusterIdx2solutionIdxLUT[clusterIdx];

      int min_row, min_col, max_row, max_col; // area affected by cluster potential 
      DetermineBordersOfClusterPotential(min_col, max_col, min_row, max_row, idxInSolutionVector, x, context);

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
          gX += context->spreadingData.spreadingWeight * gradX;
          gY += context->spreadingData.spreadingWeight * gradY;
        }
      }

      grad[2*idxInSolutionVector+0] = gX;
      grad[2*idxInSolutionVector+1] = gY;
    }
  }
}

void QS_AddObjectiveAndGradient(AppCtx* context, PetscScalar * solution, double* f)
{
  timetype start = GET_TIME_METHOD();
  *f += context->spreadingData.spreadingWeight * SpreadingPenalty(context, solution);
  //ALERT("SPR = %f", addValue));
  timetype finish = GET_TIME_METHOD();
  quadraticSpreading += finish - start;

  start = GET_TIME_METHOD();
  AddSpreadingPenaltyGradient(context, solution, context->gQS);
  finish = GET_TIME_METHOD();
  quadraticSpreadingGradTime += finish - start;
}

int CalcMaxAffectedArea(double potentialSize, double binSize)
{
  return 1+2*static_cast<int>(ceil(potentialSize/binSize));
}