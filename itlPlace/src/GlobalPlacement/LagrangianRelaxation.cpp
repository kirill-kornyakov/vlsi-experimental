#include "LagrangianRelaxation.h"
#include "LogSumExp.h"

extern timetype expTime;

double CalcSourceMu(AppCtx* context, int netIdx)
{
  //FIXME: use correct values
  int nArcs = 1;
  return context->LRdata.mu * (double)nArcs;
}

double GetCi(AppCtx* context, int sinkIdx)
{
  return context->LRdata.r * context->LRdata.c * context->LRdata.mu; //FIXME: use correct values
}

double GetBi(AppCtx* context, int netIdx, int sinkIdx)
{
  double sinkLoad = 1.0;  //FIXME: use correct values
  return context->LRdata.r * context->LRdata.mu * sinkLoad;
}

double GetDoi(AppCtx* context, PetscScalar* solution, int netIdx, int sinkIdx)
{
  double sourceX = 0.0;
  double sourceY = 0.0;
  double sinkX = 0.0;
  double sinkY = 0.0;

  int realClusterIdx;
  realClusterIdx = context->ci->netList[netIdx].clusterIdxs[0];
  GetClusterCoordinates(realClusterIdx, solution, context, sourceX, sourceY);
  realClusterIdx = context->ci->netList[netIdx].clusterIdxs[sinkIdx];
  GetClusterCoordinates(realClusterIdx, solution, context, sinkX, sinkY);

  return fabs(sourceX - sinkX) + fabs(sourceY - sinkY);
}

double GetPartnerCoordinate(AppCtx* context, PetscScalar* solution, bool isX, int netIdx, int pinIdx)
{
  int realClusterIdx = context->ci->netList[netIdx].clusterIdxs[pinIdx];
  
  double sourceX, sourceY;
  GetClusterCoordinates(realClusterIdx, solution, context, sourceX, sourceY);

  if (isX)
    return sourceX;
  else
    return sourceY;
}

int GetDoiDerivative(AppCtx* context, PetscScalar* solution, 
                     double clusterCoordinate, double partnerCoordinate)
{
  if (clusterCoordinate > partnerCoordinate)
    return 1;
  else if (clusterCoordinate < partnerCoordinate)
    return -1;
  else
    return 0;
}

double GetBraces(AppCtx* context, PetscScalar* solution, int netIdx)
{
  double sourceDrive = 1.0; //FIXME: use correct values

  double A = context->LRdata.alphaTWL + context->LRdata.c * CalcSourceMu(context, netIdx) * sourceDrive;
  
  double secondTerm = 0.0;
  for (int s = 1; s < context->netListSizes[netIdx]; s++)
  {
    double Ci = GetCi(context, s);
    double Doi = GetDoi(context, solution, netIdx, s);

    secondTerm += Ci * Doi;
  }
  secondTerm *= 0.5;

  return A + secondTerm;
}

double GetGreenTerm(AppCtx* context, PetscScalar* solution, int netIdx)
{
  double termG = 0.0;
  for (int s = 1; s < context->netListSizes[netIdx]; s++)
  {
    double Bi = GetBi(context, netIdx, s);
    double Doi = GetDoi(context, solution, netIdx, s);
    
    termG += Bi * Doi;
  }

  return termG;
}

int GetClusterNetPinIdx(AppCtx* context, int netIdx, int clusterIdx)
{
  int netSize = context->netListSizes[netIdx];

  for (int j = 0; j < netSize; ++j)
  {
    int realClusterIdx = context->ci->netList[netIdx].clusterIdxs[j];
    if (clusterIdx == realClusterIdx)
    {
      return j;
    }
  }
  return -1;
}

double LR(AppCtx* context, PetscScalar* solution)
{
  double LRValue = 0.0;

  int netListSize = static_cast<int>(context->ci->netList.size());
  for (int netIdx = 0; netIdx < netListSize; ++netIdx)
  {
    double braces = GetBraces(context, solution, netIdx);
    double LSE = CalcNetLSE(context, solution, netIdx);
    double greenTerm = GetGreenTerm(context, solution, netIdx);

    LRValue += braces * LSE + greenTerm;
    //ALERTFORMAT(("%d LRValue =\t%f", netIdx, LRValue));
  }

  return LRValue;
}

void AddLRGradient(AppCtx* context, int nCoordinates, PetscScalar* solution, PetscScalar* g)
{
  //#pragma omp parallel for
  for (int idxInSolutionVector = 0; idxInSolutionVector < nCoordinates; idxInSolutionVector++)
  {
    int clusterIdx = context->solutionIdx2clusterIdxLUT[idxInSolutionVector];
    int tableSize = static_cast<int>(context->ci->tableOfAdjacentNets[clusterIdx].size());
    //consider all adjacent nets
    for (int j = 0; j < tableSize; ++j)
    {
      double term1 = 0.0;
      double term2 = 0.0;
      double term3 = 0.0;

      int netIdx = context->ci->tableOfAdjacentNets[clusterIdx][j];
      double braces = GetBraces(context, solution, netIdx);
      double LSE = CalcNetLSE(context, solution, netIdx);
      double gLSE = CalcNetLSEGradient(context, netIdx, idxInSolutionVector);

      term1 = braces * gLSE;

      int pinIdx = GetClusterNetPinIdx(context, netIdx, clusterIdx);
      ERROR_ASSERT(pinIdx != -1);

      double clusterCoordinate = solution[idxInSolutionVector];
      if (pinIdx != 0)
      {//this cluster is a sink
        double Ci = GetCi(context, pinIdx);
        double Bi = GetBi(context, netIdx, pinIdx);
        double partnerCoordinate = GetPartnerCoordinate(context, solution, idxInSolutionVector%2, netIdx, 0);
        int doiDerivative = GetDoiDerivative(context, solution, clusterCoordinate, partnerCoordinate);

        term2 = LSE * 0.5 * Ci * doiDerivative;
        term3 = Bi * doiDerivative;
      }
      else
      {
        for (int sinkIdx = 1; sinkIdx < context->netListSizes[netIdx]; sinkIdx++)
        {
          double Ci = GetCi(context, pinIdx);
          double Bi = GetBi(context, netIdx, pinIdx);
          double partnerCoordinate = GetPartnerCoordinate(context, solution, idxInSolutionVector%2, netIdx, sinkIdx);
          int doiDerivative = GetDoiDerivative(context, solution, clusterCoordinate, partnerCoordinate);

          term2 += LSE * 0.5 * Ci * doiDerivative;
          term3 += Bi * doiDerivative;
        }
      }

      g[idxInSolutionVector] += term1 + term2 + term3;
    }
  }
}

void LR_AddObjectiveAndGradient(AppCtx* context, PetscScalar* solution, double*& f)
{
  PrecalcExponents(context, solution);

  *f += LR(context, solution);

  int nClusterVariables = 2 * context->ci->mCurrentNumberOfClusters;
  AddLRGradient(context, nClusterVariables, solution, context->gLR);
}