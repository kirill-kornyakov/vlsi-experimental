#include "HDesign.h"
#include "Spreading.h"
#include "SpreadingData.h"
#include "OptimizationContext.h"

using namespace std;

void DetermineDimensionsOfBinGrid(HDesign& hd, vector<Cluster>& clusters, 
                                  const int nClusters, BinGrid& binGrid,
                                  int desiredNumberOfClustersAtEveryBin)
{
    static double minX = hd.Circuit.PlacementMinX();
    static double maxX = hd.Circuit.PlacementMaxX();
    static double minY = hd.Circuit.PlacementMinY();
    static double maxY = hd.Circuit.PlacementMaxY();

    double circuitAspectRatio = (maxX - minX) / (maxY - minY);

    binGrid.nBinRows = static_cast<int>(sqrt(circuitAspectRatio * nClusters/desiredNumberOfClustersAtEveryBin));
    binGrid.nBinCols = static_cast<int>(Aux::cool_dtoi(circuitAspectRatio * binGrid.nBinRows));

    //WARNING: we need odd numbers, because cells will not move otherwise
    if (binGrid.nBinRows % 2 == 0) binGrid.nBinRows++; 
    if (binGrid.nBinCols % 2 == 0) binGrid.nBinCols++;

    binGrid.binWidth = (maxX - minX) / binGrid.nBinCols;
    binGrid.binHeight = (maxY - minY) / binGrid.nBinRows;
}

void ConstructBinGrid(HDesign& hd, AppCtx& context, int aDesiredNumberOfClustersAtEveryBin)
{
    static int desiredNumberOfClustersAtEveryBin;
    if (aDesiredNumberOfClustersAtEveryBin != -1)
        desiredNumberOfClustersAtEveryBin = aDesiredNumberOfClustersAtEveryBin;
    else
        desiredNumberOfClustersAtEveryBin--;

    BinGrid& binGrid = context.sprData.binGrid;
    double& potentialRadiusX = context.sprData.potentialRadiusX;
    double& potentialRadiusY = context.sprData.potentialRadiusY;
    double& invPSX = context.sprData.invPSX;
    double& invPSY = context.sprData.invPSY;
    double**& clusterPotentialOverBins = context.sprData.clusterPotentialOverBins;
    double**& bufferPotentialOverBins = context.sprData.bufferPotentialOverBins;

    // Calculate current bin grid
    DetermineDimensionsOfBinGrid(hd, context.ci->clusters, context.ci->mCurrentNumberOfClusters, 
        binGrid, desiredNumberOfClustersAtEveryBin);

    //TODO: correct this potential radius calculation
    double potentialRatio = hd.cfg.ValueOf(".potentialRatio", 2.1); //WARNING: must be greater than 0.5
    potentialRadiusX = potentialRatio*binGrid.binWidth;
    potentialRadiusY = potentialRatio*binGrid.binHeight;
    invPSX = 1 / potentialRadiusX / potentialRadiusX;
    invPSY = 1 / potentialRadiusY / potentialRadiusY;

    int maxAffectedRows = CalcMaxAffectedArea(potentialRadiusY, binGrid.binHeight);
    int maxAffectedCols = CalcMaxAffectedArea(potentialRadiusX, binGrid.binWidth);

    //free memory from previous iteration
    //TODO: last iteration causes memory leak
    if (clusterPotentialOverBins)
    {
        for (int i = 0; i<maxAffectedRows; i++)
        {
            if (clusterPotentialOverBins[i])
                delete [] clusterPotentialOverBins[i];
        }
        delete [] clusterPotentialOverBins;
    }
    if (bufferPotentialOverBins)
    {
        for (int i = 0; i<binGrid.nBinRows; i++)
        {
            if (bufferPotentialOverBins[i])
                delete [] bufferPotentialOverBins[i];
        }
        delete [] bufferPotentialOverBins;
    }
    if (binGrid.bins)
    {
        for (int j = 0; j < binGrid.nBinRows; ++j)
        {
            if (binGrid.bins[j])
                delete [] binGrid.bins[j];
        }
        delete [] binGrid.bins;
    }

    //allocate memory
    clusterPotentialOverBins = new double*[maxAffectedRows];
    for (int i = 0; i < maxAffectedRows; i++)
    {
        clusterPotentialOverBins[i] = new double[maxAffectedCols];
    }

    bufferPotentialOverBins = new double* [binGrid.nBinRows];
    for (int i = 0; i < binGrid.nBinRows; i++)
    {
        bufferPotentialOverBins[i]  = new double[binGrid.nBinCols];
    }
    
    binGrid.bins = new Bin*[binGrid.nBinRows];
    for (int j = 0; j < binGrid.nBinRows; ++j)
    {
        binGrid.bins[j] = new Bin[binGrid.nBinCols];
    }

    for (int i = 0; i < binGrid.nBinRows; ++i)
    {
        for (int j = 0; j < binGrid.nBinCols; ++j)
        {
            binGrid.bins[i][j].xCoord = (j + 0.5) * binGrid.binWidth;
            binGrid.bins[i][j].yCoord = (i + 0.5) * binGrid.binHeight;
        }
    }

    binGrid.nBins = binGrid.nBinCols * binGrid.nBinRows;

    context.sprData.totalCellArea = 0.0;
    for (int i = 0; i < static_cast<int>(context.ci->clusters.size()); ++i)
    {
        if (context.ci->clusters[i].isFake == false)
        {
            context.sprData.totalCellArea += context.ci->clusters[i].area;
        }
    }
    context.sprData.desiredCellsAreaAtEveryBin = context.sprData.totalCellArea / binGrid.nBins;
}

void BinGrid::SetBinGrid(BinGrid& binGrid)
{
    if (bins)
        FreeMemory();

    binHeight = binGrid.binHeight;
    binWidth = binGrid.binWidth;
    nBinCols = binGrid.nBinCols;
    nBinRows = binGrid.nBinRows;
    nBins = binGrid.nBins;

    bins = new Bin*[nBinRows];
    for (int i = 0; i < nBinRows; ++i)
    {
        bins[i] = new Bin[nBinCols];
    }

    for (int i = 0; i < nBinRows; ++i)
        for (int j = 0; j < nBinCols; ++j)
            bins[i][j] = bins[i][j];

    for (int i = 0; i < nBinRows; ++i)
    {
        for (int j = 0; j < nBinCols; ++j)
        {
            bins[i][j].xCoord = (j + 0.5) * binWidth;
            bins[i][j].yCoord = (i + 0.5) * binHeight;
        }
    }
}
void BinGrid::FreeMemory()
{
    for (int i = 0; i < nBinRows; ++i)
    {
        delete [] bins[i];
    }
    delete [] bins;
}
