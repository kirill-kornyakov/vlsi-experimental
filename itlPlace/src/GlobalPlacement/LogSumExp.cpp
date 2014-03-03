#include "LogSumExp.h"
//#include "mkl_vml.h"

extern timetype expTime;
extern timetype lseTime;
extern timetype lseGradTime;

int GetPrecalcedExponentsIdx(AppCtx* context, int clusterIdx)
{
  if (IsMovableCell(clusterIdx))
  {
    return context->clusterIdx2solutionIdxLUT[clusterIdx];
  }
  else if (IsTerminal(clusterIdx))
  {
    return clusterIdx - SHIFT_NUMBER_FOR_TERMINALS + context->ci->mCurrentNumberOfClusters;
  }
  else
  {
    return clusterIdx - SHIFT_NUMBER_FOR_PRIMARY_PINS + context->ci->mCurrentNumberOfClusters
      + context->ci->terminalCells.size();
  }
}

void GetClusterCoordinates(int clusterIdx, PetscScalar* coordinates, 
                           AppCtx* context, double& x, double& y)
{
  if (IsMovableCell(clusterIdx))
  {
    int clusterIdxInCoordinatesArray = (context->clusterIdx2solutionIdxLUT)[clusterIdx];
    CRITICAL_ASSERT(clusterIdxInCoordinatesArray != -1);
    x = coordinates[2*clusterIdxInCoordinatesArray+0];
    y = coordinates[2*clusterIdxInCoordinatesArray+1];
  }
  else if (IsTerminal(clusterIdx))
  {
    HCell terminal = context->ci->terminalCells[clusterIdx - SHIFT_NUMBER_FOR_TERMINALS];
    x = context->hd->GetDouble<HCell::X>(terminal);
    y = context->hd->GetDouble<HCell::Y>(terminal);
  }
  else
  {
    HPin pin = context->ci->primaryPins[clusterIdx - SHIFT_NUMBER_FOR_PRIMARY_PINS];
    x = context->hd->GetDouble<HPin::X>(pin);
    y = context->hd->GetDouble<HPin::Y>(pin);
  }
}

double CalcNetLSE(AppCtx* context, PetscScalar* coordinates, int netIdx)
{
  double sumExp1 = 0.0;
  double sumExp2 = 0.0;
  double sumExp3 = 0.0;
  double sumExp4 = 0.0;

  int precalcedExponentsIdx;
  int realClusterIdx;

  int netSize = context->netListSizes[netIdx];

  for (int j = 0; j < netSize; ++j)
  {
    realClusterIdx = context->ci->netList[netIdx].clusterIdxs[j];
    precalcedExponentsIdx = GetPrecalcedExponentsIdx(context, realClusterIdx);

    sumExp1 += context->LSEdata.precalcedExponents[4*precalcedExponentsIdx + 0];
    sumExp2 += context->LSEdata.precalcedExponents[4*precalcedExponentsIdx + 1];
    sumExp3 += context->LSEdata.precalcedExponents[4*precalcedExponentsIdx + 2];
    sumExp4 += context->LSEdata.precalcedExponents[4*precalcedExponentsIdx + 3];
  }
  context->LSEdata.SUM1[netIdx] = sumExp1;
  context->LSEdata.SUM2[netIdx] = sumExp2;
  context->LSEdata.SUM3[netIdx] = sumExp3;
  context->LSEdata.SUM4[netIdx] = sumExp4;

  return context->LSEdata.alpha * log(sumExp1 * sumExp2 * sumExp3 * sumExp4);
}

double CalcNetLSEGradient(AppCtx* context, int netIdx, int idxInSolutionVector)
{
  double sum1 = 0.0;
  double sum2 = 0.0;

  if (idxInSolutionVector % 2 == 0)
  {
    sum1 = context->LSEdata.SUM1[netIdx];
    sum2 = context->LSEdata.SUM2[netIdx];
  }
  else
  {
    sum1 = context->LSEdata.SUM3[netIdx];
    sum2 = context->LSEdata.SUM4[netIdx];
  }

  //TODO [issue 131]: we need alpha here, but parameters tuning will follow
  return /*context->alpha * */
    (context->LSEdata.precalcedExponents[2*idxInSolutionVector + 0] / sum1 - 
    context->LSEdata.precalcedExponents[2*idxInSolutionVector + 1] / sum2) * 
    context->ci->netList[netIdx].weight;
}

void PerformArgsAssignment(AppCtx* context, PetscScalar* coordinates, double* arguments)
{
  int i;
  int nClusters    = context->ci->mCurrentNumberOfClusters;
  int nTerminals   = context->ci->terminalCells.size();
  int nPrimaryPins = context->ci->primaryPins.size();
  int realClusterIdx;
  double x = 0.0;
  double y = 0.0;
  double alpha = context->LSEdata.alpha;

  for (i = 0; i < nClusters; ++i)
  {
    realClusterIdx = context->solutionIdx2clusterIdxLUT[2 * i];
    GetClusterCoordinates(realClusterIdx, coordinates, context, x, y);
    arguments[4*i + 0] = (+x / alpha);
    arguments[4*i + 1] = (-x / alpha);
    arguments[4*i + 2] = (+y / alpha);
    arguments[4*i + 3] = (-y / alpha);
  }

  for (i = 0; i < nTerminals; ++i)
  {
    GetClusterCoordinates(i + SHIFT_NUMBER_FOR_TERMINALS, coordinates, context, x, y);
    arguments[4*(i + nClusters) + 0] = (+x / alpha);
    arguments[4*(i + nClusters) + 1] = (-x / alpha);
    arguments[4*(i + nClusters) + 2] = (+y / alpha);
    arguments[4*(i + nClusters) + 3] = (-y / alpha);
  }

  for (i = 0; i < nPrimaryPins; ++i)
  {
    GetClusterCoordinates(i + SHIFT_NUMBER_FOR_PRIMARY_PINS, coordinates, context, x, y);
    arguments[4*(i + nClusters + nTerminals) + 0] = (+x / alpha);
    arguments[4*(i + nClusters + nTerminals) + 1] = (-x / alpha);
    arguments[4*(i + nClusters + nTerminals) + 2] = (+y / alpha);
    arguments[4*(i + nClusters + nTerminals) + 3] = (-y / alpha);
  }
}

void PrecalcExponents(AppCtx* context, PetscScalar* coordinates)
{
  int i;
  int nClusters    = context->ci->mCurrentNumberOfClusters;
  int nTerminals   = context->ci->terminalCells.size();
  int nPrimaryPins = context->ci->primaryPins.size();

  int count = (nClusters + nTerminals + nPrimaryPins) * 4; 
  double* arguments = context->LSEdata.argsForPrecalcedExponents;

  PerformArgsAssignment(context, coordinates, arguments);

#ifdef __MKL_VML_H__
  int batchSize = context->LSEdata.batchSize;
  for (i = 0; i < count / batchSize; ++i)
    vdExp(batchSize, &arguments[i * batchSize], &context->LSEdata.precalcedExponents[i * batchSize]);
  vdExp(count % batchSize, &arguments[count - count % batchSize], 
    &context->LSEdata.precalcedExponents[count - count % batchSize]);
#else
  for (i = 0; i < count; ++i)
    context->LSEdata.precalcedExponents[i] = exp(arguments[i]);
#endif
}

double LogSumExp(void* data, PetscScalar* coordinates)
{
  AppCtx* context = (AppCtx*)data;

  double logSumExp = 0.0;
  int netListSize = static_cast<int>(context->ci->netList.size());

  for (int i = 0; i < netListSize; ++i)
  {
    logSumExp += CalcNetLSE(context, coordinates, i) * context->ci->netList[i].weight;
    //ALERT("%d logSumExp =\t%f", i, logSumExp));
  }

  return logSumExp;
}

void AddLogSumExpGradient(AppCtx* context, int nCoordinates, PetscScalar* coordinates, PetscScalar* grad)
{
  //#pragma omp parallel for
  for (int idxInSolutionVector = 0; idxInSolutionVector < nCoordinates; idxInSolutionVector++)
  {
    int clusterIdx = context->solutionIdx2clusterIdxLUT[idxInSolutionVector];
    int tableSize = static_cast<int>(context->ci->tableOfAdjacentNets[clusterIdx].size());
    //consider all adjacent nets
    for (int j = 0; j < tableSize; ++j)
    {
      grad[idxInSolutionVector] += CalcNetLSEGradient(context, 
        context->ci->tableOfAdjacentNets[clusterIdx][j],
        idxInSolutionVector);
    }
  }
}

void LSE_AddObjectiveAndGradient(AppCtx* context, PetscScalar* solution)
{
  timetype start;
  timetype finish;

  start = GET_TIME_METHOD();
  context->criteriaValues.lse = LogSumExp(context, solution);
  //ALERT("LSE = %f", addValue));
  finish = GET_TIME_METHOD();
  lseTime += finish - start;

  start = GET_TIME_METHOD();
  int nClusterVariables = 2 * context->ci->mCurrentNumberOfClusters;
  AddLogSumExpGradient(context, nClusterVariables, solution, context->criteriaValues.gLSE);
  finish = GET_TIME_METHOD();
  lseGradTime += finish - start;
}