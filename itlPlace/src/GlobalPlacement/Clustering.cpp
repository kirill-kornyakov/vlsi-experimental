#include "Clustering.h"

#include <vector>
#include "math.h"
#include <stdlib.h>
//#include <conio.h>

//TODO: do smth with this trash
#define MARK_NEIGHBORS_INVALID true
#define DONT_MARK_NEIGHBORS_INVALID false


inline int CellToClusterIdx(ClusteringInformation& ci, HCell cell)
{
    return ::ToID(cell) - 1;
}


void InitializeClusters(HDesign& hd, ClusteringInformation& ci)
{
    ASSERT(ci.clusters.size() >= hd.Cells.PlaceableCellsCount());
    //put clusters
    int clusterIdx = 0;
    for (HCells::PlaceableCellsEnumeratorW cell = hd.Cells.GetPlaceableCellsEnumeratorW(); cell.MoveNext(); )
    {
        ci.clusters[clusterIdx].cells.push_back(cell);
        ci.clusters[clusterIdx].area = cell.Width() * cell.Height();
        ASSERT(CellToClusterIdx(ci, cell) == clusterIdx);
        clusterIdx++;
    }
}

int AssignClusterIdx(HDesign& hd, ClusteringInformation& ci, HPinWrapper& pin)
{
    int clusterIdx = 0;

    if (pin.IsPrimary())
    {
        clusterIdx = SHIFT_NUMBER_FOR_PRIMARY_PINS + (int)ci.primaryPins.size();
        ci.primaryPins.push_back(pin);
    }
    else if (hd.GetBool<HCell::IsTerminal>(pin.Cell()))
    {
        clusterIdx = SHIFT_NUMBER_FOR_TERMINALS + (int)ci.terminalCells.size();
        ci.terminalCells.push_back(pin.Cell());
    }
    else
    {
        clusterIdx = CellToClusterIdx(ci, pin.Cell());
    }

    return clusterIdx;
}

void MoveValueToTheFirstPosition(IntVector& vector, int value)
{
    IntIterator iter = find(vector.begin(), vector.end(), value);
    ASSERT(iter != vector.end());
    vector.erase(iter);
    vector.insert(vector.begin(), value);
}

void AssignClusters(HDesign& design, ClusteringInformation& ci, HNetWrapper& net, int netIdx)
{
    IntVector& idxs = ci.netList[netIdx].clusterIdxs;

    //put pins (we also add terminals and primary pins for LogSumExp calculation)
    idxs.resize(net.PinsCount());
    int i = 0;
    int sourceIdx; //WARNING: be careful with this source
    for (HNet::PinsEnumeratorW pin = net.GetPinsEnumeratorW(); pin.MoveNext(); ++i)
    {
        idxs[i] = AssignClusterIdx(design, ci, pin);
        if (i == 0) sourceIdx = idxs[i];
    }

    //remove duplicates
    sort(idxs.begin(), idxs.end());
    IntIterator iter = unique(idxs.begin(), idxs.end());
    idxs.resize(iter - idxs.begin());

    //move source to the first position
    MoveValueToTheFirstPosition(idxs, sourceIdx); 
}

void AssignWeightForClusteredNet(HDesign& hd, ClusteringInformation& ci, int netIdx)
{
  double maxWeight = 0.0;
  //find original nets connected to all clusters in this clustered net
  for (int i = 0; i < static_cast<int>(ci.netList[netIdx].clusterIdxs.size()); i++)
  {
    int currClusterIdx = ci.netList[netIdx].clusterIdxs[i];

    if (!IsMovableCell(currClusterIdx) || ci.clusters[currClusterIdx].isFake)
      continue;

    //for all cells in this cluster ci.clusters[clusterIdx].isFake
    for (int j = 0; j < static_cast<int>(ci.clusters[currClusterIdx].cells.size()); j++)
    {
      HCell currCell = ci.clusters[currClusterIdx].cells[j];

      for (HCell::PinsEnumerator pe = hd.Get<HCell::Pins, HCell::PinsEnumerator>(currCell);
           pe.MoveNext(); )
      {
        HNet currNet = hd.Get<HPin::Net, HNet>(pe);
        double netWeight = hd.GetDouble<HNet::Weight>(currNet);

        if (netWeight > maxWeight)
          maxWeight = netWeight;
      }
    }
  }

  ASSERT(maxWeight > 0);
  ci.netList[netIdx].weight = maxWeight;
}

void WriteWeightsToClusteredNets(HDesign& hd, ClusteringInformation& ci)
{
  int netIdx = 0;

  for (HNets::ActiveNetsEnumeratorW net = hd.Nets.GetActiveNetsEnumeratorW(); net.MoveNext(); ++netIdx)
  {
    ci.netList[netIdx].weight = net.Weight();
    //AssignWeightForClusteredNet(hd, ci, netIdx);
  }
}

void InitializeNetList(HDesign& hd, ClusteringInformation& ci)
{
    ci.netList.resize(hd.Nets.Count(NetKind_Active));

    //put nets
    int netIdx = 0;
    for (HNets::ActiveNetsEnumeratorW net = hd.Nets.GetActiveNetsEnumeratorW(); net.MoveNext(); ++netIdx)
    {
        AssignClusters(hd, ci, net, netIdx);
    }

    WriteWeightsToClusteredNets(hd, ci);
    ci.netLevels.push_back(ci.netList);
}

//we add terminals and primary pins for LogSumExp calculation
//used only when if clustering is loaded from file
void InitializeTerminalsAndPrimaryPins(HDesign& hd, ClusteringInformation& ci)
{
    for (HNets::ActiveNetsEnumeratorW net = hd.Nets.GetActiveNetsEnumeratorW(); net.MoveNext(); )
    {
        for (HNet::PinsEnumeratorW pin = net.GetPinsEnumeratorW(); pin.MoveNext(); )
        {
            if (pin.IsPrimary())
            {
                ci.primaryPins.push_back(pin);
            }
            else if (hd.GetBool<HCell::IsTerminal>(pin.Cell()))
            {
                ci.terminalCells.push_back(pin.Cell());
            }
        }
    }
}

void PrintClusteringInfo(HDesign& hd, ClusteringInformation& ci, int nClusters)
{
    ALERT("Currently there are %d clusters", ci.mCurrentNumberOfClusters);

    int upLimit = ci.mCurrentNumberOfClusters;
    if (nClusters >= 0)
        upLimit = nClusters;

    for (int i = 0; i<upLimit; i++)
    {
        ALERT("cluster %d:\t", i);
        if (ci.clusters[i].isFake)
        {
            ALERT("FAKE");
            continue;
        }
        else
        {
            for (int j = 0; j < static_cast<int>(ci.clusters[i].cells.size()); j++)
            {
                ALERT(hd[ci.clusters[i].cells[j]].Name().c_str());
            }
        }
    }
}

void CreateTableOfAdjacentNets(HDesign& hd, ClusteringInformation& ci)
{
    //prepare table
    ci.tableOfAdjacentNets.clear();
    ci.tableOfAdjacentNets.resize(ci.clusters.size());

    //fill the table
    for (int netIdx = 0; netIdx < (int)ci.netList.size(); ++netIdx)
    {
        for (int j = 0; j < (int)ci.netList[netIdx].clusterIdxs.size(); ++j)
        {
            int clusterIdx = ci.netList[netIdx].clusterIdxs[j];
            if (IsMovableCell(clusterIdx) && !ci.clusters[clusterIdx].isFake)
                ci.tableOfAdjacentNets[clusterIdx].push_back(netIdx);
        }
    }

    //remove duplicated nets
    for (unsigned int i = 0; i < ci.tableOfAdjacentNets.size(); i++)
    {
        std::sort(ci.tableOfAdjacentNets[i].begin(), ci.tableOfAdjacentNets[i].end());
        ConnectionsVector::iterator newEnd
            = unique(ci.tableOfAdjacentNets[i].begin(), ci.tableOfAdjacentNets[i].end());
        ci.tableOfAdjacentNets[i].resize(newEnd - ci.tableOfAdjacentNets[i].begin());
    }
}

//NOTE: you have to set initial value -1 for clusterIdx if
//you want to go through all clusters
bool GetNextActiveClusterIdx(ClusteringInformation* ci, /*ref*/ int& clusterIdx)
{
    //CHECK:what about terminals and primary pins?
    do
    {
        ++clusterIdx;
    } while (clusterIdx < (int)ci->clusters.size() && ci->clusters[clusterIdx].isFake);
    return clusterIdx < (int)ci->clusters.size();
}

double AffinityLite(HDesign& hd, std::vector<int>& a, std::vector<int>& b, int* netListSizes, Cluster& ac, Cluster& bc)
{
    double result = 0.0;
    long size1 = static_cast<int>(a.size());
    long size2 = static_cast<int>(b.size());
    long pos1=0;
    long pos2=0;

    while (pos1 < size1 && pos2 < size2)
    {
        if (a[pos1] < b[pos2])
            ++pos1;
        else if (a[pos1] > b[pos2])
            ++pos2;
        else
        {
            if (netListSizes[a[pos1]] > 1)
                result += hd._Design.NetList.netWeight[pos1] / ((netListSizes[a[pos1]] - 1) * ac.area * bc.area);
            ++pos1;
            ++pos2;
        }
    }
    return result;
}

double Affinity(HDesign& hd, const int& firstClusterIdx, const int& secondClusterIdx, 
                std::vector<Cluster>& clusters, NetList& netList, int* netListSizes,
                std::vector<ConnectionsVector>& currTableOfAdjacentNets)
{
    return AffinityLite(hd, currTableOfAdjacentNets[firstClusterIdx],
        currTableOfAdjacentNets[secondClusterIdx],
        netListSizes,
        clusters[firstClusterIdx],
        clusters[secondClusterIdx]);
}

void CalculateNetListSizes(NetList& netList, int* netListSizes)
{
    int netListSize = (int)netList.size();
    for (int i = 0; i < netListSize; i++)
        netListSizes[i] = (int)netList[i].clusterIdxs.size();
}

bool AreClustersMergeable(ClusteringInformation& ci, int clusterIdx, int neighborCandidateIdx)
{
    if (clusterIdx == neighborCandidateIdx) 
        return false;

    //WARNING: do not join following 2 is statement, because we have
    //to call IsMovableCell() first
    if (!IsMovableCell(neighborCandidateIdx)) return false;
    return !ci.clusters[neighborCandidateIdx].isFake;
}

int FindBestNeighbour(HDesign& hd, ClusteringInformation& ci, int* netListSizes, MergeCandidate& mergeCandidate)
{
    mergeCandidate.score = 0.0;
    IntVector& mrgNets = ci.tableOfAdjacentNets[mergeCandidate.clusterIdx];
    for(IntIterator j = mrgNets.begin(); j != mrgNets.end(); ++j)
    {
        IntVector& mrgClusters = ci.netList[*j].clusterIdxs;
        for (IntIterator k = mrgClusters.begin(); k != mrgClusters.end(); ++k)
        {
            if (!AreClustersMergeable(ci, mergeCandidate.clusterIdx, *k)) continue;

            //double score = Affinity(hd, mergeCandidate.clusterIdx, *k, ci.clusters, ci.netList, 
            //                        netListSizes, ci.tableOfAdjacentNets);

            double score = AffinityLite(hd,
                ci.tableOfAdjacentNets[mergeCandidate.clusterIdx],
                ci.tableOfAdjacentNets[*k],
                netListSizes,
                ci.clusters[mergeCandidate.clusterIdx],
                ci.clusters[*k]);

            if (mergeCandidate.score < score)
            {
                ASSERT(score > 0.0);
                mergeCandidate.score = score;
                mergeCandidate.bestNeighborIdx = *k;
            }
        }
    }
    return OK;
}

int MarkNeighboursAsInvalid(ClusteringInformation& ci, int clusterIdx)
{
    IntVector& mrgNets = ci.tableOfAdjacentNets[clusterIdx];
    for(IntIterator j = mrgNets.begin(); j != mrgNets.end(); ++j)
    {
        IntVector& mrgClusters = ci.netList[*j].clusterIdxs;
        for (IntIterator k = mrgClusters.begin(); k != mrgClusters.end(); ++k)
            if (clusterIdx != *k && IsMovableCell(*k))
                ci.clusters[*k].isValid = false;
    }
    return OK;
}

int CalculateNumberOfActiveClusters(ClusteringInformation& ci)
{
    int nActiveClusters = 0;
    int clusterIdx = -1;
    while (GetNextActiveClusterIdx(&ci, clusterIdx))
        nActiveClusters++;
    return nActiveClusters;
}

void UnclusterLevelUp(HDesign& hd, ClusteringInformation& ci, ClusteringLogIterator& clusteringLogIterator)
{
    std::list<MergedCluster>::reverse_iterator iter = clusteringLogIterator->rbegin();

    for (; iter != clusteringLogIterator->rend(); ++iter)
    {
        ci.clusters[iter->cluster1Idx].cells.resize(iter->nCellsInCluster1);
        ci.clusters[iter->cluster1Idx].area -= ci.clusters[iter->cluster2Idx].area;
        ci.clusters[iter->cluster2Idx].isFake = false;
        ci.clusters[iter->cluster2Idx].xCoord = ci.clusters[iter->cluster1Idx].xCoord;
        ci.clusters[iter->cluster2Idx].yCoord = ci.clusters[iter->cluster1Idx].yCoord;
    }

    ci.mCurrentNumberOfClusters = CalculateNumberOfActiveClusters(ci);

    CreateTableOfAdjacentNets(hd, ci);
}

int ShiftHeadAccordingToScore(std::list<MergeCandidate>& mergeCandidates)
{
    std::list<MergeCandidate>::iterator headCandidate = mergeCandidates.begin();

    //find proper place for head of the list
    std::list<MergeCandidate>::iterator iter = headCandidate;
    iter++;
    for (; iter != mergeCandidates.end(); iter++)
    {
        if (headCandidate->score >= iter->score)
            break;
    }

    //move the head
    mergeCandidates.insert(iter, *headCandidate);
    mergeCandidates.pop_front();

    return OK;
}

void MergeCurrTableOfAdjacentNets(std::vector<ConnectionsVector>& currTableOfAdjacentNets, 
                                  const int cluster1Idx, const int cluster2Idx)
{
    ConnectionsVector tempMergedVector(currTableOfAdjacentNets[cluster1Idx].size() 
        + currTableOfAdjacentNets[cluster2Idx].size());

    merge(currTableOfAdjacentNets[cluster1Idx].begin(), currTableOfAdjacentNets[cluster1Idx].end(), 
        currTableOfAdjacentNets[cluster2Idx].begin(), currTableOfAdjacentNets[cluster2Idx].end(),
        tempMergedVector.begin());

    ConnectionsVector::iterator newEnd = unique(tempMergedVector.begin(), tempMergedVector.end());
    currTableOfAdjacentNets[cluster1Idx].resize(newEnd - tempMergedVector.begin());
    copy(tempMergedVector.begin(), newEnd, currTableOfAdjacentNets[cluster1Idx].begin());
}

void MergeCellIdxs(std::vector<Cluster>& clusters, const int& clusterIdx1, const int& clusterIdx2)
{
    int size1 = clusters[clusterIdx1].cells.size();
    int size2 = clusters[clusterIdx2].cells.size();

    std::vector<HCell> tempVector;
    tempVector.resize(size1 + size2);
    copy(clusters[clusterIdx1].cells.begin(), clusters[clusterIdx1].cells.end(), tempVector.begin());
    copy(clusters[clusterIdx2].cells.begin(), clusters[clusterIdx2].cells.end(), tempVector.begin() + size1);

    clusters[clusterIdx1].cells.resize(size1 + size2);
    copy(tempVector.begin(), tempVector.end(), clusters[clusterIdx1].cells.begin());
}

void UpdateNetList(std::vector<ConnectionsVector>& currTableOfAdjacentNets, NetList& netList, 
                   const int firstClusterIdx, const int secondClusterIdx)
{
    std::vector<int>::iterator iter;
    for (int j = 0; j < static_cast<int>(currTableOfAdjacentNets[secondClusterIdx].size()); ++j)
    {
        int netIdx = currTableOfAdjacentNets[secondClusterIdx][j];
        IntVector& clusterIdxs = netList[netIdx].clusterIdxs;

        if (clusterIdxs.size() == 0)
            continue;

        //rewrite second cluster idx with first cluster idx
        iter = find(clusterIdxs.begin(), clusterIdxs.end(), secondClusterIdx);
        ASSERT(iter != clusterIdxs.end());
        (*iter) = firstClusterIdx;

        //save head of the vector
        int head = clusterIdxs[0];

        //remove duplicates
        sort(clusterIdxs.begin(), clusterIdxs.end());
        iter = unique(clusterIdxs.begin(), clusterIdxs.end());
        clusterIdxs.resize(iter - clusterIdxs.begin());

        //restore head
        MoveValueToTheFirstPosition(clusterIdxs, head);

        //clear net if it is totally inside cluster
        //if (netList[netIdx].clusterIdxs.size() == 1)
        //{
        //  netList[netIdx].clusterIdxs.clear();
        //}
    }
}

// the result is stored in clusterIdx1
int MergePairOfClusters(const int cluster1Idx, const int cluster2Idx, 
                        std::vector<Cluster>& clusters, NetList& netList,
                        std::vector<ConnectionsVector>& currTableOfAdjacentNets)
{
    MergeCurrTableOfAdjacentNets(currTableOfAdjacentNets, cluster1Idx, cluster2Idx);
    MergeCellIdxs(clusters, cluster1Idx, cluster2Idx);
    UpdateNetList(currTableOfAdjacentNets, netList, cluster1Idx, cluster2Idx);

    clusters[cluster1Idx].xCoord = (clusters[cluster1Idx].xCoord * clusters[cluster1Idx].area + 
        clusters[cluster2Idx].xCoord * clusters[cluster2Idx].area) / 
        (clusters[cluster1Idx].area + clusters[cluster2Idx].area);
    clusters[cluster1Idx].yCoord = (clusters[cluster1Idx].yCoord * clusters[cluster1Idx].area + 
        clusters[cluster2Idx].yCoord * clusters[cluster2Idx].area) / 
        (clusters[cluster1Idx].area + clusters[cluster2Idx].area);
    clusters[cluster1Idx].area += clusters[cluster2Idx].area;
    clusters[cluster2Idx].isFake = true;

    return OK;
}

bool PredicateMergePairGreater(MergeCandidate elem1, MergeCandidate elem2)
{
    return elem1.score > elem2.score;
}

bool PredicateNetListLess(ClusteredNet elem1, ClusteredNet elem2)
{
    int size1 = static_cast<int>(elem1.clusterIdxs.size());
    int size2 = static_cast<int>(elem2.clusterIdxs.size());

    if (size1 < size2)
        return true;
    else if (size1 > size2)
        return false;

    for (int i = 0; i < size1; ++i)
    {
        if (elem1.clusterIdxs[i] < elem2.clusterIdxs[i])
            return true;
        else if (elem1.clusterIdxs[i] > elem2.clusterIdxs[i])
            return false;
    }

    // сортировка по весам в обратном порядке
    // при применении функции unique автоматически будет выбран первый нет
    // при таком способе сортировки у этого нета будет максимальный вес
    if (elem1.weight > elem2.weight)
        return true;
    else if (elem1.weight < elem2.weight)
        return false;

    return false;
}

bool IsEqualNetListBinaryPredicate(ClusteredNet elem1, ClusteredNet elem2)
{
    int size1 = static_cast<int>(elem1.clusterIdxs.size());
    int size2 = static_cast<int>(elem2.clusterIdxs.size());

    if (size1 != size2)
        return false;

    for (int i = 0; i < size2; ++i)
    {
        if (elem1.clusterIdxs[i] != elem2.clusterIdxs[i])
            return false;
    }

    return true;
}

double CalculateTotalCellArea(HDesign &hd)
{
    double totalCellArea = 0.0;

    for (HCells::PlaceableCellsEnumeratorW cell = hd.Cells.GetPlaceableCellsEnumeratorW(); cell.MoveNext(); )
    {
        totalCellArea += cell.Width() * cell.Height();
    }  

    return totalCellArea;
}

void InitializeMergeCandidates(ClusteringInformation& ci, std::list<MergeCandidate>& mergeCandidates)
{
    mergeCandidates.clear();
    mergeCandidates.resize(ci.clusters.size());

    std::list<MergeCandidate>::iterator mcIter;
    int clusterIdx = 0;
    for (mcIter = mergeCandidates.begin(); mcIter != mergeCandidates.end(); mcIter++)
    {
        mcIter->clusterIdx = clusterIdx;
        mcIter->bestNeighborIdx = -1; 
        mcIter->score = 0.0;
        clusterIdx++;
    }
}

void CalculateScores(HDesign& hd, ClusteringInformation& ci, 
                     std::list<MergeCandidate>& mergeCandidates, int*& netListSizes)
{
    std::list<MergeCandidate>::iterator mcIter;
    for (mcIter = mergeCandidates.begin(); mcIter != mergeCandidates.end(); mcIter++)
    {
        MergeCandidate& mc = *mcIter;
        FindBestNeighbour(hd, ci, netListSizes, mc);
    }
}

void InitializeClustersData(ClusteringInformation& ci, std::vector<MergeCandidate>& clustersData)
{
    for (int i = 0; i < static_cast<int>(ci.clusters.size()); ++i)
    {
        clustersData[i].clusterIdx = i;
        clustersData[i].score = 0.0;
    }
}

void CalculateScores(HDesign& hd, ClusteringInformation& ci, 
                     std::vector<MergeCandidate>& clustersData, int*& netListSizes)
{
    InitializeClustersData(ci, clustersData);

    std::vector<MergeCandidate>::iterator mcIter;
    for (mcIter = clustersData.begin(); mcIter != clustersData.end(); mcIter++)
    {
        MergeCandidate& mc = *mcIter;
        FindBestNeighbour(hd, ci, netListSizes, mc);
    }
}

NetList::iterator FindInVector(NetList::iterator start, 
                               NetList::iterator end, 
                               ClusteredNet item)
{
    NetList::iterator iter = start;
    while (iter != end && !IsEqualNetListBinaryPredicate(*iter, item))
    {
        ++iter;
    }
    return iter;
}

void Swap(NetList::iterator i1, NetList::iterator i2)
{
    ClusteredNet tmp;
    tmp = *i1;
    *i1 = *i2;
    *i2 = tmp;
}

void PurifyNetList(HDesign& hd, ClusteringInformation& ci)
{
    // Points to where the unique vector ends (and the duplicating part starts)
    NetList::iterator netListIter;
    string aggregationMethod = hd.cfg.ValueOf(".NetWeighting.APlace.aggregationMethod", "max");

    sort(ci.netList.begin(), ci.netList.end(), PredicateNetListLess);
    ALERT("NetList size before one cluster nets removing: %d", ci.netList.size());
    //delete nets with less than 2 clusters
    netListIter = ci.netList.begin();
    while (netListIter->clusterIdxs.size() < 2) 
        ++netListIter;
    ci.netList.erase(ci.netList.begin(), netListIter);
    ALERT("NetList size after  one cluster nets removing: %d", ci.netList.size());

    //delete duplicated nets
    if (hd.cfg.ValueOf(".Clustering.deleteDuplicatingNets", true))
    {
        // This will point to the last element of unique part of the vector
        NetList::iterator lastUniqueIdx = ci.netList.begin();
        if (aggregationMethod == "sum-1")
        {
            ALERT("Aggregation method for net weights inheritance is SUM-1");
            for (netListIter = ci.netList.begin(); netListIter != ci.netList.end(); ++netListIter)
            {
                while (netListIter != ci.netList.end() && 
                    IsEqualNetListBinaryPredicate(*netListIter, *lastUniqueIdx))
                {
                    lastUniqueIdx->weight += netListIter->weight - 1;
                    ++netListIter;
                }
                if (netListIter != ci.netList.end())
                {
                    *(++lastUniqueIdx) = *netListIter;
                } else
                    break;
            }
            /* Now lastUniqueIdx points to the last unique value, for instance for vector<int>
            was:
            4 5 5 5 5 5 5 6 7 7 7 7 7 9 9 9 9
            now:
            4 5 6 7 9 5 5 6 7 7 7 7 7 9 9 9 9
            ^
            |
            lastUniqueIdx
            so we need to increment it
            */
            ++lastUniqueIdx;
        } else if (aggregationMethod == "sum")
        {
            ALERT("Aggregation method for net weights inheritance is SUM");
            for (netListIter = ci.netList.begin(); netListIter != ci.netList.end(); ++netListIter)
            {
                while (netListIter != ci.netList.end() && 
                    IsEqualNetListBinaryPredicate(*netListIter, *lastUniqueIdx))
                {
                    lastUniqueIdx->weight += netListIter->weight;
                    ++netListIter;
                }
                if (netListIter != ci.netList.end())
                {
                    *(++lastUniqueIdx) = *netListIter;
                } else
                    break;
            }
            /* Now lastUniqueIdx points to the last unique value, for instance for vector<int>
            was:
            4 5 5 5 5 5 5 6 7 7 7 7 7 9 9 9 9
            now:
            4 5 6 7 9 5 5 6 7 7 7 7 7 9 9 9 9
            ^
            |
            lastUniqueIdx
            so we need to increment it
            */
            ++lastUniqueIdx;
        } else
            /* In this case the net with maximum weight will be inherited */
        {
            ALERT("Aggregation method for net weights inheritance is MAX");
            lastUniqueIdx = unique(ci.netList.begin(), ci.netList.end(),
                IsEqualNetListBinaryPredicate);
        }
        ci.netList.resize(lastUniqueIdx - ci.netList.begin());
        ALERT("NetList size after duplicates removing: %d", ci.netList.size());
    }
}

void EraseZeroScorePairs(std::list<MergeCandidate>& mergeCandidates)
{
    std::list<MergeCandidate>::iterator mcIter;
    std::list<MergeCandidate>::iterator eraseIter;
    for (mcIter = mergeCandidates.begin(); mcIter != mergeCandidates.end(); )
    {
        if (mcIter->score != 0.0)
        {
            mcIter++;
        }
        else
        {
            eraseIter = mcIter;
            mcIter++;
            mergeCandidates.erase(eraseIter);
        }
    }
}

void CreateListOfMergeCandidates(HDesign& hd, ClusteringInformation& ci, 
                                 int*& netListSizes, std::list<MergeCandidate>& mergeCandidates)
{
    InitializeMergeCandidates(ci, mergeCandidates);
    CalculateScores(hd, ci, mergeCandidates, netListSizes);
    mergeCandidates.sort(PredicateMergePairGreater);
    EraseZeroScorePairs(mergeCandidates);
}

bool IsMergedAreaAcceptable(ClusteringInformation &ci, int currClusterIdx, int bestNeighborIdx, double maxClusterArea)
{
    ASSERT(bestNeighborIdx >= 0);
    return ci.clusters[currClusterIdx].area + ci.clusters[bestNeighborIdx].area <= maxClusterArea;
}

void PrintMergeCandidates(std::list<MergeCandidate>& mergeCandidates)
{
    for (std::list<MergeCandidate>::iterator iter = mergeCandidates.begin(); 
        iter != mergeCandidates.end(); iter++)
    {
        WRITE("(%d, %d)\t", iter->clusterIdx, iter->bestNeighborIdx); //KNOTE: added .id
    }
    WRITELINE("");
}

void MergeClusters(HDesign& hd, ClusteringInformation& ci)
{
    std::list<MergeCandidate> mergeCandidates;
    ClusteringLog thisLevelClusteringLog;

    int targetNClusters = static_cast<int>(ci.mCurrentNumberOfClusters / ci.mClusterRatio);
    double maxClusterArea = (CalculateTotalCellArea(hd) / targetNClusters) * ci.mClustersAreaTolerance;

    int* netListSizes = new int[ci.netList.size()];
    CalculateNetListSizes(ci.netList, netListSizes);

    CreateListOfMergeCandidates(hd, ci, netListSizes, mergeCandidates);

    //PrintMergeCandidates(mergeCandidates);

    //main loop of merging
    int mergedCount = 0;
    while (mergeCandidates.size() && ci.mCurrentNumberOfClusters - mergedCount > targetNClusters)
    {
        std::list<MergeCandidate>::iterator headCandidate = mergeCandidates.begin();
        int currClusterIdx  = headCandidate->clusterIdx;
        int bestNeighborIdx = headCandidate->bestNeighborIdx;

        if (ci.clusters[currClusterIdx].isValid && 
            IsMergedAreaAcceptable(ci, currClusterIdx, bestNeighborIdx, maxClusterArea)) 
        {//merge
            MergePairOfClusters(currClusterIdx, bestNeighborIdx, ci.clusters, 
                ci.netList, ci.tableOfAdjacentNets);
            mergedCount++;

            //save merge info to the log
            MergedCluster mergedCluster;
            mergedCluster.cluster1Idx = currClusterIdx;
            mergedCluster.cluster2Idx = bestNeighborIdx;
            mergedCluster.nCellsInCluster1 = ci.clusters[currClusterIdx].cells.size();
            thisLevelClusteringLog.push_back(mergedCluster);

            CalculateNetListSizes(ci.netList, netListSizes);

            MarkNeighboursAsInvalid(ci, bestNeighborIdx);
            MarkNeighboursAsInvalid(ci, currClusterIdx);

            //delete second cluster merge pair - it is already clustered
            for (std::list<MergeCandidate>::iterator iter = mergeCandidates.begin(); 
                iter != mergeCandidates.end(); iter++)
            {
                if (iter->clusterIdx == bestNeighborIdx)
                {
                    mergeCandidates.erase(iter);
                    break;
                }
            }

            //shift head of the list
            MergeCandidate& mc = *headCandidate;
            FindBestNeighbour(hd, ci, netListSizes, mc);
            ShiftHeadAccordingToScore(mergeCandidates);
            //reset validity - now cluster is valid, mark it
            ci.clusters[currClusterIdx].isValid = true;
        }
        else if (!ci.clusters[currClusterIdx].isValid)
        {
            //shift head of the list
            MergeCandidate& mc = *headCandidate;
            FindBestNeighbour(hd, ci, netListSizes, mc);
            ShiftHeadAccordingToScore(mergeCandidates);
            //reset validity - now cluster is valid, mark it
            ci.clusters[currClusterIdx].isValid = true;
        }
        else
        {//we shouldn't merge this pair of clusters - they are too fat
            mergeCandidates.pop_front();
        }
    }

    ci.clusteringLog.push_back(thisLevelClusteringLog);
    delete[] netListSizes;
}

void MarkClustersAsValid(ClusteringInformation& ci)
{
    for (int i = 0; i < static_cast<int>(ci.clusters.size()); ++i)
    {
        ci.clusters[i].isValid = true;
    }
}



int Clustering(HDesign& hd, ClusteringInformation& ci)
{
    WRITELINE("");
    ALERT("CLUSTERING STARTED");

    ci.mCurrentNumberOfClusters = hd.Cells.MovableCellsCount();
    ci.clusters.resize(ci.mCurrentNumberOfClusters);

    if (hd.cfg.ValueOf(".Clustering.useClusteringInformationFromFile", false))
    {//LOAD CLUSTERING INFO AND ESCAPE
        string fileName = ci.GetClusteringInformationFileName(hd);
        if (ci.LoadFromFile(fileName.c_str(), hd.Circuit.Name().c_str(), hd))
        {
            InitializeTerminalsAndPrimaryPins(hd, ci);
            ALERT("CLUSTERING INFORMATION LOADED");
            goto ClusteringExport;
            return OK;
        }
    }

    //PERFORM CLUSTERING
    InitializeClusters(hd, ci);
    InitializeNetList(hd, ci);
    CreateTableOfAdjacentNets(hd, ci);

    PrintClusteringInfo(hd, ci, 0);
    while (ci.mCurrentNumberOfClusters > ci.mDesiredFinalNumberOfClusters)
    {
        MergeClusters(hd, ci);

        PurifyNetList(hd, ci);
        ci.netLevels.push_back(ci.netList);
        CreateTableOfAdjacentNets(hd, ci);

        ci.mCurrentNumberOfClusters = CalculateNumberOfActiveClusters(ci);
        MarkClustersAsValid(ci);

        PrintClusteringInfo(hd, ci, 0);
    }
    ALERT("CLUSTERING FINISHED");
    ALERT("The number of levels is %u", ci.netLevels.size());

ClusteringExport:
    //EXPORT CLUSTERING
    if (ci.netLevels.size() > 1)
    {//do not export 1 level clustering
        string fileName = ci.GetClusteringInformationFileName(hd);
        ci.SaveToFile(fileName.c_str(), hd.Circuit.Name().c_str(), hd);
        ALERT("CLUSTERING INFORMATION EXPORTED");
    }

    return OK;
}
