#include "SumOfDelays.h"
#include "LogSumExp.h"

double GetNetKOpt(PetscScalar* coordinates, int nClusterCoordinates, int i, AppCtx* context)
{
  double kopt = (CalcNetLSE(context, coordinates, i) 
    + context->SODdata.netSODData[i].Lnet - context->SODdata.Lbuf) / context->SODdata.Dbuf - 1;
  return kopt;
}

double GetNetDelay(PetscScalar* coordinates, int nClusterCoordinates, int i, AppCtx* context)
{
  double ki = 0.0;
  ki = coordinates[nClusterCoordinates + i]; //WARNING: this index is correct
  
  context->SODdata.netSODData[i].braces = 
    CalcNetLSE(context, coordinates, i) + 
    context->SODdata.netSODData[i].Lnet + 
    ki * context->SODdata.Lbuf;

  //NOTE: debugging variables
  double x = 0.0;
  double y = 0.0;

  x = context->SODdata.netSODData[i].braces;
  x = x * x;
  x = x / (ki + 1);
  y = ki * context->SODdata.DbufLbufDifferenceOfSquares;

  return x + y;
}

double SumOfDelays(void* data, PetscScalar* coordinates)
{
  AppCtx* context = (AppCtx*)data;

  double sumOfDelays = 0.0;
  int netListSize = static_cast<int>(context->ci->netList.size());
  int nClusterCoordinates = 2 * context->ci->mCurrentNumberOfClusters;

  for (int i = 0; i < netListSize; ++i)
  {
    sumOfDelays = sumOfDelays + GetNetDelay(coordinates, nClusterCoordinates, i, context);
  }

  return sumOfDelays;
}

void AddSumOfDelaysGradient(AppCtx* context, PetscScalar* coordinates, PetscScalar* gradient)
{
  int nAdjacentNets;
  int netIdx;
  int netListSize = static_cast<int>(context->ci->netList.size());

  int nClusterCoordinates = 2*context->ci->mCurrentNumberOfClusters;

  for (int i = 0; i < nClusterCoordinates; i++)
  {
    int clusterIdx = context->solutionIdx2clusterIdxLUT[i];
    nAdjacentNets = static_cast<int>(context->ci->tableOfAdjacentNets[clusterIdx].size());

    for (int j = 0; j < nAdjacentNets; ++j)
    {//consider all adjacent nets
      netIdx = context->ci->tableOfAdjacentNets[clusterIdx][j];
      gradient[i] += 2 * context->SODdata.netSODData[netIdx].braces * 
        CalcNetLSEGradient(context, netIdx, i);
    }
  }

  double ki = 0.0;

  for (int i = nClusterCoordinates; i < nClusterCoordinates + netListSize; i++)
  {
    ki = coordinates[i]; //NOTE: this index is correct
    netIdx = i - nClusterCoordinates;
    gradient[i] = context->SODdata.DbufLbufDifferenceOfSquares + 
      (2 * context->SODdata.netSODData[netIdx].braces * context->SODdata.Lbuf * (ki + 1) - 
      context->SODdata.netSODData[netIdx].braces * context->SODdata.netSODData[netIdx].braces) / 
      ((ki + 1) * (ki + 1));
  }
}

void SOD_AddObjectiveAndGradient(AppCtx* context, PetscScalar* solution)
{
  context->criteriaValues.sod = SumOfDelays(context, solution);;
  AddSumOfDelaysGradient(context, solution, context->criteriaValues.gSOD);
}