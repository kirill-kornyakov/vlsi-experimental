#include "LagrangianRelaxation.h"
#include "LogSumExp.h"

double GetCi(AppCtx* context, HClusteredNet netIdx, int sinkIdx)
{
    return context->LRdata.r * context->LRdata.c 
        * context->LRdata.netLRData[::ToID(netIdx) - 1].netArcMus[sinkIdx-1];
}

double GetBi(AppCtx* context, HClusteredNet netIdx, int sinkIdx)
{
    return context->LRdata.r 
        * context->LRdata.netLRData[::ToID(netIdx) - 1].netArcMus[sinkIdx-1] 
    * context->LRdata.netLRData[::ToID(netIdx) - 1].sinkLoad[sinkIdx-1];
}

double GetDoi(AppCtx* context, PetscScalar* solution, HClusteredNet netIdx, int sinkIdx)
{
    double sourceX = 0.0;
    double sourceY = 0.0;
    double sinkX = 0.0;
    double sinkY = 0.0;

    HCluster realClusterIdx;
    HClusteredNetWrapper netIdxW = (*context->hd)[netIdx];
    realClusterIdx = (*netIdxW.clusterIdxs())[0];
    GetClusterCoordinates(realClusterIdx, solution, context, sourceX, sourceY);
    realClusterIdx = (*netIdxW.clusterIdxs())[sinkIdx];
    GetClusterCoordinates(realClusterIdx, solution, context, sinkX, sinkY);

    return fabs(sourceX - sinkX) + fabs(sourceY - sinkY);
}

double GetPartnerCoordinate(AppCtx* context, PetscScalar* solution, bool isX, HClusteredNet netIdx, int pinIdx)
{
    HClusteredNetWrapper netIdxW = (*context->hd)[netIdx];
    HCluster realClusterIdx = (*netIdxW.clusterIdxs())[pinIdx];

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

double GetA(AppCtx* context, HClusteredNet netIdx)
{
    return context->LRdata.c * context->LRdata.netLRData[::ToID(netIdx) - 1].sourceAFactor;
}

double GetBraces(AppCtx* context, PetscScalar* solution, HClusteredNet netIdx)
{
    double A = GetA(context, netIdx);

    double secondTerm = 0.0;
    HClusteredNetWrapper netIdxW = (*context->hd)[netIdx];
    for (int sinkIdx = 1; sinkIdx < netIdxW.clusterIdxs()->size(); sinkIdx++)
    {
        double Ci = GetCi(context, netIdx, sinkIdx);
        double Doi = GetDoi(context, solution, netIdx, sinkIdx);

        secondTerm += Ci * Doi;
    }
    secondTerm *= 0.5;

    return A + secondTerm;
}

double GetGreenTerm(AppCtx* context, PetscScalar* solution, HClusteredNet netIdx)
{
    double termG = 0.0;
    HClusteredNetWrapper netIdxW = (*context->hd)[netIdx];
    for (int s = 1; s < netIdxW.clusterIdxs()->size(); s++)
    {
        double Bi = GetBi(context, netIdx, s);
        double Doi = GetDoi(context, solution, netIdx, s);

        termG += Bi * Doi;
    }

    return termG;
}

int GetClusterNetPinIdx(AppCtx* context, HClusteredNet netIdx, HCluster clusterIdx)
{
    HClusteredNetWrapper netIdxW = (*context->hd)[netIdx];
    int netSize = netIdxW.clusterIdxs()->size();

    for (int j = 0; j < netSize; ++j)
    {
        HCluster realClusterIdx = (*netIdxW.clusterIdxs())[j];
        if (clusterIdx == realClusterIdx)
        {
            return j;
        }
    }
    return -1;
}

void LR(AppCtx* context, PetscScalar* solution)
{
    double termTNS = 0.0;

    //int netListSize = static_cast<int>(context->ci->netList.size());
    //for (int netIdx = 0; netIdx < netListSize; ++netIdx)
    for (HClusteredNets::ClusteredNetsEnumeratorW netIdx = context->hd->ClustersNetList.GetEnumeratorW(); netIdx.MoveNext(); )
    {
        double braces = GetBraces(context, solution, netIdx);
        double LSE = CalcNetLSE(context, solution, netIdx);
        double greenTerm = GetGreenTerm(context, solution, netIdx);

        termTNS += braces * LSE + greenTerm;
    }

    context->criteriaValues.lr = termTNS;
}

void GetNetDerivative(AppCtx* context, HCluster clusterIdx, int j, PetscScalar* solution, int idxInSolutionVector)
{
    double term1 = 0.0;
    double term2 = 0.0;
    double term3 = 0.0;

    //int netIdx = context->ci->tableOfAdjacentNets[clusterIdx][j];
    HClusteredNetWrapper netIdx = (*context->hd)[(*(*context->hd)[clusterIdx].tableOfAdjacentNets())[j]];//(*context->hd)[context->ci->tableOfAdjacentNets[::ToID(clusterIdx) - 1][j]];

    double braces = GetBraces(context, solution, netIdx);
    double LSE = CalcNetLSE(context, solution, netIdx);
    double gLSE = CalcNetLSEGradient(context, netIdx, idxInSolutionVector);

    term1 = braces * gLSE;

    int clusterPinIdx = GetClusterNetPinIdx(context, netIdx, clusterIdx);
    ASSERT(clusterPinIdx != -1);

    double clusterCoordinate = solution[idxInSolutionVector];
    if (clusterPinIdx != 0)
    {//this cluster is a sink
        double Ci = GetCi(context, netIdx, clusterPinIdx);
        double Bi = GetBi(context, netIdx, clusterPinIdx);
        double partnerCoordinate = GetPartnerCoordinate(context, solution, idxInSolutionVector%2, netIdx, 0);
        int doiDerivative = GetDoiDerivative(context, solution, clusterCoordinate, partnerCoordinate);

        term2 = LSE * 0.5 * Ci * doiDerivative;
        term3 = Bi * doiDerivative;
    }
    else
    {
        for (int sinkIdx = 1; sinkIdx < netIdx.clusterIdxs()->size(); sinkIdx++)
        {
            double Ci = GetCi(context, netIdx, sinkIdx);
            double Bi = GetBi(context, netIdx, sinkIdx);
            double partnerCoordinate = GetPartnerCoordinate(context, solution, idxInSolutionVector%2, netIdx, sinkIdx);
            int doiDerivative = GetDoiDerivative(context, solution, clusterCoordinate, partnerCoordinate);

            term2 += LSE * 0.5 * Ci * doiDerivative;
            term3 += Bi * doiDerivative;
        }
    }

    context->criteriaValues.gLR[idxInSolutionVector] += term1 + term2/* + term3*/;
}

void AddLRGradient(AppCtx* context, int nCoordinates, PetscScalar* solution)
{
    for (int idxInSolutionVector = 0; idxInSolutionVector < nCoordinates; idxInSolutionVector++)
    {
        HCluster clusterIdx = context->solutionIdx2clusterIdxLUT[idxInSolutionVector];
        int tableSize = static_cast<int>(context->ci->design[clusterIdx].tableOfAdjacentNets()->size());
        //consider all adjacent nets
        for (int j = 0; j < tableSize; ++j)
        {
            GetNetDerivative(context, clusterIdx, j, solution, idxInSolutionVector);
        }
    }
}

void LR_AddObjectiveAndGradient(AppCtx* context, PetscScalar* solution)
{
    LR(context, solution);
    AddLRGradient(context, 2 * context->ci->mCurrentNumberOfClusters, solution);
}