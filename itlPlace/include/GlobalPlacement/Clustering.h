#ifndef __CLUSTERING__
#define __CLUSTERING__

#include "HDesign.h"
#include "Auxiliary.h"
#include "ClusteringData.h"

const int OK = 0;

const int SHIFT_NUMBER_FOR_TERMINALS = 1000000; 
const int SHIFT_NUMBER_FOR_PRIMARY_PINS = 2000000;

inline bool IsMovableCell(int idx)
{
    return idx < SHIFT_NUMBER_FOR_TERMINALS;
}

inline bool IsTerminal(int idx)
{
    return idx >= SHIFT_NUMBER_FOR_TERMINALS && idx < SHIFT_NUMBER_FOR_PRIMARY_PINS;
}

inline bool IsPrimaryPin(int idx)
{
    return idx >= SHIFT_NUMBER_FOR_PRIMARY_PINS;
}

class ClusteringInformation
{
public:
    int    mClusterRatio; // decrease ratio of the number of clusters after each iteration
    int    mDesiredFinalNumberOfClusters;
    double mClustersAreaTolerance;
    int    mCurrentNumberOfClusters;
    std::vector<Cluster> clusters;
    std::vector<HPin> primaryPins;     //PP and terminal also indexed as usual cells for LSE calculation
    std::vector<HCell> terminalCells;  //they aren't clustered, but  they has their own cluster index

    std::list<ClusteringLog> clusteringLog; // РёРЅС„РѕСЂРјР°С†РёСЏ РґР»СЏ РїРµСЂРµС…РѕРґР° РјРµР¶РґСѓ СѓСЂРѕРІРЅСЏРјРё РєР»Р°СЃС‚РµСЂРёР·Р°С†РёРё

    NetList netList;
    std::list<NetList> netLevels;  // список нетлистов на каждом уровне кластеризации

    std::vector<ConnectionsVector> tableOfAdjacentNets;
    pAffinityFunction affinityFunction;

    ClusteringInformation(HDesign& hd)
    {
        mClusterRatio                 = hd.cfg.ValueOf(".Clustering.clusterRatio", 5);
        mDesiredFinalNumberOfClusters = hd.cfg.ValueOf(".Clustering.desiredFinalNumberOfClusters", 2000);
        mClustersAreaTolerance        = hd.cfg.ValueOf(".Clustering.clustersAreaTolerance", 1.5);
    }

    string GetClusteringInformationFileName(HDesign& hd);
    void SaveToFile(const char* fileName, const char* benchName, HDesign& hd);
    bool LoadFromFile(const char* fileName, const char* benchName, HDesign& hd);

private:
    void SaveClustersToFile(FILE* rf, HDesign& hd);
    void SaveClusteringLogToFile(FILE* rf);
    void SaveNetListToFile(FILE* rf, NetList& netList);
    void SaveNetLevelsToFile(FILE* rf);
    void SaveCurrTableOfAdjacentNetsToFile(FILE* rf);

    void LoadClustersFromFile(FILE* rf, HDesign& hd);
    void LoadClusteringLogFromFile(FILE* rf);
    void LoadNetListFromFile(FILE* rf, NetList& netList);
    void LoadNetLevelsFromFile(FILE* rf);
    void LoadCurrTableOfAdjacentNetsFromFile(FILE* rf);
};

int Clustering(HDesign& hd, ClusteringInformation& ci);

void UnclusterLevelUp(HDesign& hd, ClusteringInformation& ci, ClusteringLogIterator& iter);

void CalculateNetListSizes(NetList& netList, int* netListSizes);

bool PredicateMergePairGreater(MergeCandidate elem1, MergeCandidate elem2);

double Affinity(HDesign& hd, const int& firstClusterIdx, const int& secondClusterIdx, 
                std::vector<Cluster>& clusters, NetList& netList, int* netListSizes,
                std::vector<ConnectionsVector>& currTableOfAdjacentNets);

int FindBestNeighbour(HDesign& hd, ClusteringInformation& ci, int* netListSizes, 
                      MergeCandidate& mergeCandidate);

void CreateTableOfAdjacentNets(HDesign& hd, ClusteringInformation& ci);

//returns next not fake cluster index
bool GetNextActiveClusterIdx(ClusteringInformation* ci, int& clusterIdx);

void CalculateScores(HDesign& hd, ClusteringInformation& ci, 
                     std::list<MergeCandidate>& mergeCandidates, int*& netListSizes);
void CalculateScores(HDesign& hd, ClusteringInformation& ci, 
                     std::vector<MergeCandidate>& clustersData, int*& netListSizes);

int CalculateNumberOfActiveClusters(ClusteringInformation& ci);
void AssignWeightForClusteredNet(HDesign& hd, ClusteringInformation& ci, int netIdx);
void WriteWeightsToClusteredNets(HDesign& hd, ClusteringInformation& ci);

#endif