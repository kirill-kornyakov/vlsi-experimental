#include "HDesign.h"
#include "Clustering.h"
#include "OptimizationContext.h"

#include "tao.h"

namespace AnalyticalGlobalPlacement
{
  void InitializeDataStructures(std::vector<Cluster>& clusters, NetList& netList);  
  int InitializeTAO(HDesign& hd, ClusteringInformation& ci, AppCtx &context, 
    Vec& x, Vec& xl, Vec& xu, 
    TAO_SOLVER& tao, TAO_APPLICATION& taoapp);
  
  int Relaxation(HDesign& hd, ClusteringInformation& ci, int metaIteration);
  int Interpolation(HDesign& hd, ClusteringInformation& ci);
  int Solve(HDesign& hd, ClusteringInformation& ci, AppCtx& context, TAO_APPLICATION taoapp, 
    TAO_SOLVER tao, Vec x, int metaIteration);

  void ExportNetWeights( NetList::iterator &netListIter, ClusteringInformation &ci, int &i ); 

  void SetVariablesValues(ClusteringInformation& ci, Vec& x);
  void GetVariablesValues(ClusteringInformation& ci, Vec x);
  void SetBounds(HDesign& hd, ClusteringInformation& ci, AppCtx &context, Vec& xl, Vec& xu);
  
  void PlaceToTheCenterIntially(HDesign& hd, ClusteringInformation& ci);
  void UpdateCellsCoordinates(HDesign& hd, ClusteringInformation& ci);
  void WriteCellsCoordinates2Clusters(HDesign& hd, ClusteringInformation& ci);
  void SetClustersCoords(ClusteringInformation& ci, Vec& x);
  void GetClusterCoordinates(ClusteringInformation& ci, Vec x);  
  
  void SetKValues(ClusteringInformation& ci, Vec& x);
  void GetKValues(ClusteringInformation& ci, Vec x);
  double CalculateSumOfK(HDesign& hd, ClusteringInformation& ci);

  int* InitIdxs(int nVariables, int shift);
}