#include "HDesign.h"
#include "Parser.h"
#include "Utils.h"
#include "Auxiliary.h"
#include "SensitivityGuidedNetWeighting.h"
#include "Reporting.h"

#include <math.h>
#include <string>


double D(double s, double T, double beta)
{
  if (s < 0)
    return pow(1 - s / T, beta);
  else
    return 1.0;
}

double FindMaxPathDelay(HDesign& hd)
{
  double maxPathDelay = 0.0;

  for (HCriticalPaths::EnumeratorW i = hd.CriticalPaths.GetEnumeratorW(); i.MoveNext();)
  {
    HTimingPointWrapper tp = hd[hd[i.EndPoint()].TimingPoint()];

    if (maxPathDelay < tp.FallArrivalTime())
      maxPathDelay = tp.FallArrivalTime();
    if (maxPathDelay < tp.RiseArrivalTime())
      maxPathDelay = tp.RiseArrivalTime();
  }

  return maxPathDelay;
}

double ComputeSumOfWeights(HDesign& hd)
{
  double sum = 0.0;
  for (HNets::NetsEnumeratorW netW = hd.Nets.GetFullEnumeratorW(); netW.MoveNext(); )
    sum += netW.Weight();

  return sum;
}

void NormalizeWeights(HDesign& hd)
{
  double sumOfWeights = ComputeSumOfWeights(hd);

  for (HNets::NetsEnumeratorW netW = hd.Nets.GetFullEnumeratorW(); netW.MoveNext(); )
    hd.Set<HNet::Weight>(netW, netW.Weight() * hd.Nets.Count() / sumOfWeights);
}

void ComputeNetWeights(HDesign& hd)
{
  string method = hd.cfg.ValueOf("NetWeighting.method", "APlace");

  if (method == "APlace")
  {
    ALERT("Performing net-weighting algorithm designed and implemented by Artem Zhivoderov");

    double maxPathDelay = FindMaxPathDelay(hd);
    double u    = hd.cfg.ValueOf("NetWeighting.APlace.u", 0.3);
    double beta = hd.cfg.ValueOf("NetWeighting.APlace.beta", 2.0);
    double T    = (1 - u) * maxPathDelay;
    double fatWeight;  // FallArrivalTime part of weight
    double ratWeight;  // RiseArrivalTime part of weight

    for (HCriticalPaths::EnumeratorW critPathEnumW = hd.CriticalPaths.GetEnumeratorW(); critPathEnumW.MoveNext();)
    {
      for (HCriticalPath::PointsEnumeratorW pointsEnumW = critPathEnumW.GetEnumeratorW(); pointsEnumW.MoveNext();)
      {
        HPinWrapper pin  = hd[hd.Get<HTimingPoint::Pin, HPin>(pointsEnumW.TimingPoint())];
        HNetWrapper netW = hd[pin.Net()];
        HTimingPointWrapper tp = hd[hd[critPathEnumW.EndPoint()].TimingPoint()];

        fatWeight = netW.Weight() + 0.5 * (D(T - tp.FallArrivalTime(), T, beta) - 1);
        ratWeight = fatWeight     + 0.5 * (D(T - tp.RiseArrivalTime(), T, beta) - 1);
        
        hd.Set<HNet::Weight>(netW, fatWeight);
        hd.Set<HNet::Weight>(netW, ratWeight);
        pointsEnumW.MoveNext();
      }
    }
  }
  else if (method == "SensitivityGuidedNetWeighting")
  {
    ALERT("Performing net-weighting algorithm designed and implemented by Alexander Belyakov");
    SensitivityGuidedNetWeighting(hd);
  }

  if (hd.cfg.ValueOf("NetWeighting.normalize", false))
  {
    NormalizeWeights(hd);
  }
}

void ExportNetWeights(HDesign& hd, const char* fileName)
{
  FILE *netWeightsFile;

  netWeightsFile = fopen(fileName, "w");
  if (netWeightsFile)
  {
    ALERT("Exporting net-weights to .\\%s", fileName);

    for (HNets::NetsEnumeratorW netW = hd.Nets.GetFullEnumeratorW(); netW.MoveNext(); )
      fputs(Aux::Format("%s\t%f\n", 
                        netW.Name().c_str(),
                        netW.Weight()).c_str(), 
            netWeightsFile);

    fclose(netWeightsFile);
  }
}

void ImportNetWeights(HDesign& hd, const char* fileName)
{
  FILE *netWeightsFile;
  char currString[32];
  char netName[32];
  float tempVal;
  int i = 0;
  int nNetsEnd = hd._Design.NetList.nNetsEnd;

  netWeightsFile = fopen(fileName, "r");
  if (netWeightsFile)
  {
    ALERT("Importing net-weights from .\\%s", fileName);

    for (i = 1; i < nNetsEnd; ++i)
    {
      fgets(currString, 32, netWeightsFile);
      sscanf(currString, "%s\t%f\n", netName, &tempVal);
      hd._Design.NetList.netWeight[i] = tempVal;
    }

    fclose(netWeightsFile);
  }
  else
    ALERT("Error opening import net-weights file %s", fileName);
}

int GetnIter(const char* nwtsFileName)
{
  char tmp[2];

  if (nwtsFileName == "")
    tmp[0] = '0';
  else
    // We assume that we make not more than 9 iterations
    tmp[0] = nwtsFileName[strlen(nwtsFileName) - 6];
  tmp[1] = 0;
  
  return atoi(tmp);
}

void GetNewCommandLine(string& newCMD, const string& nwtsFileName, int argc, char** argv)
{
  int flagNW  = 1;

  newCMD = "";
  for(int i = 0; i < argc; i++)
  {
    std::string name(argv[i]);
    size_t pos = name.find('=', 2);

    if (name.substr(2, pos - 2) == "NetWeighting.netWeightsImportFileName")
    {
      name = "--NetWeighting.netWeightsImportFileName=" + nwtsFileName;
      flagNW = 0;
    }
    newCMD += name + " ";
  }

  if (argc == 1)
    newCMD += "default.cfg ";
  if (flagNW)
    newCMD += "--NetWeighting.netWeightsImportFileName=" + nwtsFileName;
  printf("new command line is %s\n", newCMD.c_str());
}

void ComputeAndExportWeights(HDesign& hd)
{
  string nwtsFileName = Aux::CreateCoolFileName("NetWeights/", hd.Circuit.Name(), "nwts");
  ComputeNetWeights(hd);
  ExportNetWeights(hd, nwtsFileName.c_str());
}

void PrepareNextNetWeightingLoop(HDesign& hd, int& nCyclesCounter)
{
  int nLoops = hd.cfg.ValueOf("DesignFlow.nMacroIterations", 10);
  string nwtsFileName;
  string defFileName;
  static string tnsStr = "";
  static string wnsStr = "";

  if (nLoops > 9)
    ALERT("The number of net weights iterations must be less than 10");

  ALERT("Current iteration of net weighting is %d", nCyclesCounter);

  nwtsFileName = Aux::CreateCoolFileName("NetWeights/", hd.Circuit.Name(), "nwts");
  defFileName  = hd.Circuit.Name() + "_" + Aux::IntToString(nCyclesCounter) + ".def";
  ExportDEF(hd, defFileName);
  ComputeAndExportWeights(hd);
  ReportTNSWNSSequence(hd, tnsStr, wnsStr);

  if (nCyclesCounter == nLoops)
  {
    ALERT("The specified number of net weights iterations (%d) is performed", nLoops);
    return;
  }

  hd.cfg.SetCfgValue("NetWeighting.netWeightsImportFileName", nwtsFileName); 
}