#include "OptimizationContext.h"
#include "Spreading.h"

void AppCtx::Initialize(HDesign& ahd, ClusteringInformation& aci)
{
  hd = &ahd;
  ci = &aci;

  int j = 0;
  solutionIdx2clusterIdxLUT = new int[2*CalculateNumberOfActiveClusters(aci) + static_cast<int>(ci->clusters.size())]; 
  clusterIdx2solutionIdxLUT = solutionIdx2clusterIdxLUT + 2*CalculateNumberOfActiveClusters(aci); 
  for(int i = 0; i < static_cast<int>(ci->clusters.size()); i++)
  {
    if (!ci->clusters[i].isFake)
    {
      clusterIdx2solutionIdxLUT[i] = j;
      solutionIdx2clusterIdxLUT[2*j+0] = i;
      solutionIdx2clusterIdxLUT[2*j+1] = i;
      ++j;
    }
    else
      clusterIdx2solutionIdxLUT[i] = -1;
  }
  //x,y,ki
  nVariables = 2 * ci->mCurrentNumberOfClusters + ci->netList.size();

  criteriaValues.gLSE = new double[4*nVariables];
  criteriaValues.gSOD = criteriaValues.gLSE + nVariables;
  criteriaValues.gLR  = criteriaValues.gSOD + nVariables;
  criteriaValues.gQS  = criteriaValues.gLR  + nVariables;

  ConstructBinGrid(ahd, *this, 
    ahd.cfg.ValueOf("Clustering.desiredNumberOfClustersAtEveryBin", 10));

  netListSizes = new int[ci->netList.size()];
  CalculateNetListSizes(ci->netList, netListSizes);
  
  useLogSumExp          = ahd.cfg.ValueOf(".useLogSumExp", false);
  useSumOfDelays        = ahd.cfg.ValueOf(".useSumOfDelays", false);
  useLR                 = ahd.cfg.ValueOf(".useLR", false);
  useSpreading = ahd.cfg.ValueOf(".useSpreading", false);
  useBorderBounds       = ahd.cfg.ValueOf(".useBorderBounds", true);

  if (useLogSumExp || useSumOfDelays || useLR)
    LSEdata.Initialize(ahd, aci, sprData.binGrid.binWidth);
  if (useSumOfDelays)
    SODdata.Initialize(ahd);
  if (useLR)
    LRdata.Initialize(ahd);

  sprData.useUnidirectSpreading = ahd.cfg.ValueOf(".useUnidirectSpreading", false);
}

void AppCtx::FreeMemory()
{
  for (int j = 0; j < sprData.binGrid.nBinRows; ++j)
  {
    delete [] sprData.binGrid.bins[j];
  }
  delete [] sprData.binGrid.bins;

  int maxAffectedRows = CalcMaxAffectedArea(sprData.potentialRadiusY, 
    sprData.binGrid.binHeight);
  for (int i = 0; i < maxAffectedRows; ++i)
  {
    delete [] sprData.clusterPotentialOverBins[i];
  }
  delete [] sprData.clusterPotentialOverBins;

  //TODO: check correctness of memory deletion
  delete[] LSEdata.precalcedExponents;
  delete[] netListSizes;
  delete[] solutionIdx2clusterIdxLUT;
  delete[] criteriaValues.gLSE;
}