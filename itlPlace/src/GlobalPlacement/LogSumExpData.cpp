#include "LogSumExpData.h"

void LSEData::Initialize(HDesign& hd, ClusteringInformation& ci, double binWidth)
{
  //TODO: probably we can choose this parameter better
  alpha = binWidth * hd.cfg.ValueOf(".alphaMultiplier", 0.5);

  int netListSize = ci.netList.size();
  SUM1 = new double[4*netListSize];
  SUM2 = SUM1 + netListSize;
  SUM3 = SUM2 + netListSize;
  SUM4 = SUM3 + netListSize;

  batchSize = hd.cfg.ValueOf(".batchSizeForvdExp", 1000);

  unsigned int coordsCount = ci.mCurrentNumberOfClusters 
    + ci.terminalCells.size() + ci.primaryPins.size();
  precalcedExponents        = new double[coordsCount * 8];
  argsForPrecalcedExponents = precalcedExponents + coordsCount * 4;
}