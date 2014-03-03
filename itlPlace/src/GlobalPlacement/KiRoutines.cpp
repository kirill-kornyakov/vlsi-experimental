#include "HDesign.h"
#include "AnalyticalPlacement.h"

using namespace AnalyticalGlobalPlacement;

double AnalyticalGlobalPlacement::CalculateSumOfK(HDesign& hd, ClusteringInformation& ci)
{
  double s = 0.0;
  int netListSize = static_cast<int>(ci.netList.size());
  for (int i = 0; i < netListSize; i++)
  {
    s += ci.netList[i].k;
  }
  return s;
}

void AnalyticalGlobalPlacement::GetKValues(ClusteringInformation& ci, Vec x)
{
  int nK = ci.netList.size();
  PetscScalar* values = new PetscScalar[nK];
  int* idxs = InitIdxs(nK, 2*ci.mCurrentNumberOfClusters);

  VecGetValues(x, PetscInt(nK), idxs, values);
  for (int i = 0; i < nK; i++)
  {
    ci.netList[i].k = values[i];
  }

  delete[] values;
  delete[] idxs;
}

void AnalyticalGlobalPlacement::SetKValues(ClusteringInformation& ci, Vec& x)
{
  int nK = ci.netList.size();

  PetscScalar* initValues = new PetscScalar[nK];
  int* idxs = InitIdxs(nK, 2*ci.mCurrentNumberOfClusters);

  for (int i = 0; i < nK; i++)
  {
    initValues[i] = 0.0;
  }
  VecSetValues(x, nK, idxs, initValues, INSERT_VALUES);

  delete[] initValues;
  delete[] idxs;
}