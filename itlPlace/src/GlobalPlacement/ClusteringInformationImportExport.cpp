#include "Utils.h"
#include "Clustering.h"

string ClusteringInformation::GetClusteringInformationFileName(HDesign& hd)
{
    string fileName = hd.cfg.ValueOf(".Clustering.clusteringInformationLoadFileName");
    if (fileName == "")
    {
        const char* techname = hd.cfg.ValueOf("params.techname", "IWLS");
        string name = hd.Circuit.Name();
        fileName = Aux::Format(
            "./ClusteringInformation/%s_%s_%d.ci",
            techname,
            name.c_str(),
            hd.Cells.CellsCount());
    }
    return fileName;
}

bool ClusteringInformation::LoadFromFile(const char* fileName, const char* benchName, HDesign& hd)
{
  FILE* resultFile = fopen(fileName, "r");
  char bmName[128];

  if (resultFile)
  {
    fscanf(resultFile, "%s\n", bmName);
    if (strcmp(bmName, benchName))
    {
      GLOGERROR(LOGINPLACE, "Information in %s doesn't correspond to the design", fileName);
      return false;
    }

    int lClusterRatio;
    int lDesiredFinalNumberOfClusters;
    double lClustersAreaTolerance;
    int lCurrentNumberOfClusters;

    if (0 == fscanf(resultFile, "a %d %d %I64X %d\n",
                    &lClusterRatio, 
                    &lDesiredFinalNumberOfClusters, 
                    &lClustersAreaTolerance,
                    &lCurrentNumberOfClusters)
      || lClusterRatio != mClusterRatio
      || lDesiredFinalNumberOfClusters != mDesiredFinalNumberOfClusters
      || lClustersAreaTolerance != mClustersAreaTolerance)
    {
      GLOGERROR(LOGINPLACE, "Information in %s doesn't correspond to the design", fileName);
      return false;
    }

    mCurrentNumberOfClusters = lCurrentNumberOfClusters;
    LoadClustersFromFile(resultFile, hd);
    LoadClusteringLogFromFile(resultFile);
    LoadNetLevelsFromFile(resultFile);
    LoadCurrTableOfAdjacentNetsFromFile(resultFile);

    fclose(resultFile);
    return true;
  }
  else
  {
    ALERT("Error during opening clustering information import file");
    return false;
  }
}

void ClusteringInformation::LoadClustersFromFile(FILE* rf, HDesign& hd)
{
  char  buffer[256];
  int   clustersCellsSize;
  double area;

  do
  {
    fgets(buffer, 255, rf);
  }while (strcmp(buffer, "Clusters\n"));
  fgets(buffer, 255, rf); // skip '{'

  for (unsigned int i = 0; i < clusters.size(); ++i)
  {
    fscanf(rf, "%d : %I64X %s", &clustersCellsSize, &area, buffer);
    clusters[i].area = area;
    clusters[i].cells.resize(clustersCellsSize);
    if (!strcmp(buffer, "true"))
    {
      clusters[i].isFake = true;
    }
    else
    {
      clusters[i].isFake = false;
    }

    for (int j = 0; j < clustersCellsSize; ++j)
    {
      fscanf(rf, "%s", buffer);
      clusters[i].cells[j] = Utils::FindCellByName(hd, buffer);
    }
  }
}

void ClusteringInformation::LoadClusteringLogFromFile(FILE* rf)
{
  char  buffer[256];
  int   nLevels;
  int   nMerges;
  ClusteringLog thisLevelClusteringLog;

  do 
  {
    fscanf(rf, "%s %d", buffer, &nLevels);
  } while (strcmp(buffer, "ClusteringLog"));

  fgets(buffer, 255, rf); // skip '{'
  fgets(buffer, 255, rf); // skip '{'

  for (int levelCounter = 0; levelCounter < nLevels; ++levelCounter)
  {
    ClusteringLog thisLevelClusteringLog;
    fscanf(rf, "%s %d", buffer, &nMerges);
    if (strcmp(buffer, "next_level"))
    {
      LOGCRITICAL("Wrong string in clustering file");
      return;
    }

    for (int j = 0; j < nMerges; ++j)
    {
      MergedCluster mergedCluster;
      fscanf(rf, "\t%d %d %u\n", &mergedCluster.cluster1Idx, &mergedCluster.cluster2Idx, &mergedCluster.nCellsInCluster1);
      thisLevelClusteringLog.push_back(mergedCluster);
    }
    clusteringLog.push_back(thisLevelClusteringLog);
  }
}

void ClusteringInformation::LoadNetListFromFile(FILE* rf, NetList& nl)
{
  char  buffer[256];
  unsigned int netListSize;
  unsigned int clusterIdxsSize;
  int clusterIdx;
  double weight;

  do
  {
    fscanf(rf, "\t%s %u\n", buffer, &netListSize);
  } while (strcmp(buffer, "NetList"));
  nl.resize(netListSize);

  for (unsigned int i = 0; i < netListSize; ++i)
  {
    //while(!fscanf(rf, "\t%u :\n", &clusterIdxsSize));
    do
    {
      fgets(buffer, 128, rf);
    } while (!sscanf(buffer, "\t%u :\n", &clusterIdxsSize));
    nl[i].clusterIdxs.resize(clusterIdxsSize);

    for (unsigned int j = 0; j < clusterIdxsSize; ++j)
    {
      fscanf(rf, "\t\t%d\n", &clusterIdx);
      nl[i].clusterIdxs[j] = clusterIdx;
    }
    fscanf(rf, "\tweight = %I64X\n", &weight);
    nl[i].weight = weight;
  }
  // This line should be equal to "}; // NetList"
  // we don't need it
  fgets(buffer, 128, rf);
}

void ClusteringInformation::LoadNetLevelsFromFile(FILE* rf)
{
  char  buffer[256];
  unsigned int netLevelsSize;
  unsigned int netLevelsCounter;

  do
  {
    fscanf(rf, "\t%s %u\n", buffer, &netLevelsSize);
  } while (strcmp(buffer, "NetLevels"));
  netLevels.clear();

  for (netLevelsCounter = 0; netLevelsCounter < netLevelsSize; ++netLevelsCounter)
  {
    LoadNetListFromFile(rf, netList);
    netLevels.push_back(netList);
  }
  // This line should be equal to "}; // NetLevels"
  // we don't need it
  fgets(buffer, 128, rf);
}

void ClusteringInformation::LoadCurrTableOfAdjacentNetsFromFile(FILE* rf)
{
  char  buffer[256];
  unsigned int ctoanSize;
  unsigned int connectionsVectorSize;
  int netIdx;

  do
  {
    fscanf(rf, "%s %u\n", buffer, &ctoanSize);
  } while (strcmp(buffer, "CurrTableOfAdjacentNets"));
  tableOfAdjacentNets.resize(ctoanSize);

  for (unsigned int i = 0; i < ctoanSize; ++i)
  {
    //while(!fscanf(rf, "\t%u :\n", &clusterIdxsSize));
    do
    {
      fgets(buffer, 128, rf);
    } while (!sscanf(buffer, "%u :\n", &connectionsVectorSize));
    tableOfAdjacentNets[i].resize(connectionsVectorSize);

    for (unsigned int j = 0; j < connectionsVectorSize; ++j)
    {
      fscanf(rf, "\t%d\n", &netIdx);
      tableOfAdjacentNets[i][j] = netIdx;
    }
  }
  // This line should be equal to "}; // NetList"
  // we don't need it
  fgets(buffer, 128, rf);
}

void ClusteringInformation::SaveToFile(const char* fileName, const char* benchName, HDesign& hd)
{
  FILE* resultFile = fopen(fileName, "w");

  if (resultFile)
  {
    fprintf(resultFile, "%s\n", benchName);
    fprintf(resultFile, "a %d %d %I64X %d\n", mClusterRatio, mDesiredFinalNumberOfClusters, 
      mClustersAreaTolerance, mCurrentNumberOfClusters);
    SaveClustersToFile(resultFile, hd);
    SaveClusteringLogToFile(resultFile);
    SaveNetLevelsToFile(resultFile);
    SaveCurrTableOfAdjacentNetsToFile(resultFile);

    fclose(resultFile);
  }
  else
  {
    LOGCRITICAL("Error during opening clustering information export file");
  }
}

void ClusteringInformation::SaveClustersToFile(FILE* rf, HDesign& hd)
{
  unsigned int clustersSize = clusters.size();
  unsigned int clustersCellsSize;

  fprintf(rf, "\nClusters\n{\n");

  for (unsigned int i = 0; i < clustersSize; ++i)
  {
    clustersCellsSize = clusters[i].cells.size();

    fprintf(rf, "%u : %I64X", clustersCellsSize, clusters[i].area);

    if (clusters[i].isFake)
      fprintf(rf, " true\n");
    else
      fprintf(rf, " false\n");

    for (unsigned int j = 0; j < clustersCellsSize; ++j)
    {
      fprintf(rf, "\t%s\n", hd[clusters[i].cells[j]].Name().c_str());
      //std::cout << clusters[i].cells[j].Name << std::endl;
    }
  }

  fputs("}; // Clusters\n", rf);
}

void ClusteringInformation::SaveClusteringLogToFile(FILE* rf)
{
  unsigned int clusteringLogSize = clusteringLog.size();
  std::list<ClusteringLog>::iterator clustersLogIterator;

  fprintf(rf, "\nClusteringLog %u\n{\n", clusteringLogSize);

  for (clustersLogIterator = clusteringLog.begin(); clustersLogIterator != clusteringLog.end(); ++clustersLogIterator)
  {
    fprintf(rf, "next_level %u\n", clustersLogIterator->size());
    ClusteringLog::iterator cii; // clusteringInfoIterator
    for (cii = clustersLogIterator->begin();
      cii != clustersLogIterator->end(); ++cii)
    {
      fprintf(rf, "\t%d %d %u\n", cii->cluster1Idx, cii->cluster2Idx, cii->nCellsInCluster1);
    }
  }

  fputs("}; // ClusteringLog\n", rf);
}

void ClusteringInformation::SaveNetListToFile(FILE* rf, NetList& nl)
{
  unsigned int netListSize = nl.size();
  unsigned int clusterIdxsSize;

  fprintf(rf, "\tNetList %u\n\t{\n", netListSize);

  for (unsigned int i = 0; i < netListSize; ++i)
  {
    clusterIdxsSize = nl[i].clusterIdxs.size();
    fprintf(rf, "\t%u :\n", clusterIdxsSize);

    for (unsigned int j = 0; j < clusterIdxsSize; ++j)
    {
      fprintf(rf, "\t\t%d\n", nl[i].clusterIdxs[j]);
    }
    fprintf(rf, "\tweight = %I64X\n", nl[i].weight);
  }

  fputs("\t}; // NetList\n", rf);
}

void ClusteringInformation::SaveNetLevelsToFile(FILE* rf)
{
  std::list<NetList>::iterator netLevelsIterator;

  fprintf(rf, "\nNetLevels %u\n{\n", netLevels.size());

  for (netLevelsIterator = netLevels.begin(); netLevelsIterator != netLevels.end(); ++netLevelsIterator)
  {
    SaveNetListToFile(rf, *netLevelsIterator);
  }

  fputs("}; // NetLevels\n", rf);
}

void ClusteringInformation::SaveCurrTableOfAdjacentNetsToFile(FILE* rf)
{
  unsigned int ctoanSize = tableOfAdjacentNets.size();
  unsigned int connectionsVectorSize;

  fprintf(rf, "\nCurrTableOfAdjacentNets %u\n{\n", ctoanSize);

  for (unsigned int i = 0; i < ctoanSize; ++i)
  {
    connectionsVectorSize = tableOfAdjacentNets[i].size();
    fprintf(rf, "%u :\n", connectionsVectorSize);

    for (unsigned int j = 0; j < connectionsVectorSize; ++j)
    {
      fprintf(rf, "\t%d\n", tableOfAdjacentNets[i][j]);
    }
  }

  fputs("}; // CurrTableOfAdjacentNets\n", rf);
}
