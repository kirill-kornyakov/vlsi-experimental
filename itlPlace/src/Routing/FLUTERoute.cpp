#include "FLUTERoute.h"
#include "Configuration.h"
#include "Grow.h"
#include "Logging.h"
#include "Math.h"

#define DTYPE double //for FLUTE
#define MAXD 90 //for FLUTE
#define MAX_TREE_NODES(n) ((n) * 2 - 2) //max number of points in Steiner Tree
#define DEFAULT_ACCURACY 3 //default gAccuracy for FLUTE
#define STEINER_TREE_MAX_NEIGHBOURS_COUNT 4

namespace FLUTE
{
#pragma warning(push)
#pragma warning(disable: 4244 4005 4101 4305)

#include "FLUTE\\flute.cpp"

#pragma warning(pop)
}

struct PointInfo
{
  int Count;//count of neighbours
  int Idxs[STEINER_TREE_MAX_NEIGHBOURS_COUNT];//positions of neighbours
};

struct EdgeInfo
{
  int Id;   //Steiner point id for dest point
  int SrcIdx;  //position of the edge start point
  int DestIdx; //position of the edge end point
};


//NOTE: these data should be thread private for openmp parallelization
static int gAccuracy         = DEFAULT_ACCURACY;
static double gPinX[MAXD]    = {0};//x coordinates for pins
static double gPinY[MAXD]    = {0};//y coordinates for pins
static int gPinIndexes[MAXD] = {0};//position index for pins
static int gPinIds[MAXD]     = {0};//pin's IDs
static PointInfo gTreePoints[MAX_TREE_NODES(MAXD)];

void InitFLUTE()
{
  FLUTE::readLUT();
  ::gAccuracy = ::gCfg.ValueOf("FLUTE.Accuracy", DEFAULT_ACCURACY);
}

void FLUTERoute(HDesign& aDesign, HNet& aNet)
{
  HNetWrapper net = aDesign[aNet];
  int numOfPins = net.PinsCount();
  DEBUG_ASSERT(numOfPins > 1);

  HWireWrapper wire = aDesign[aDesign.Wires[aNet]];
  //2-pins nets can be simply routed
  if (numOfPins == 2)
  {
    HSteinerPoint source = wire.Root();
    HSteinerPoint sink = aDesign.SteinerPoints[net.LastSink()];
    aDesign.SteinerPoints.InheritPinCoordinates(source);
    aDesign.SteinerPoints.InheritPinCoordinates(sink);
    aDesign.Set<HSteinerPoint::Left>(source, sink);
    wire.SetRoutingType(RoutingType_Flute);
    wire.SetLength(fabs(aDesign.GetDouble<HSteinerPoint::X>(source)
      - aDesign.GetDouble<HSteinerPoint::X>(sink))
      + fabs(aDesign.GetDouble<HSteinerPoint::Y>(source) 
      - aDesign.GetDouble<HSteinerPoint::Y>(sink)));
    return;
  }

  //remove old tree if presents
  if (wire.RoutingType() != RoutingType_Unrouted)
    aDesign.SteinerPoints.RemoveSteinerTree(wire.Root());

  //check if flute can route this
  if (numOfPins > MAXD)
  {
    wire.SetRoutingType(RoutingType_Unrouted);
    wire.SetLength(0.0);
    LOGINFOFORMAT(("FLUTE unable to route net %s", aDesign.GetString<HNet::Name>(aNet).c_str()));
    return;
  }

  //initialize input arrays for FLUTE
  //fill indexes to restore pins order after FLUTE
  int pointsCount = MAX_TREE_NODES(numOfPins);
  int pos = 0;
  for (HNet::PinsEnumeratorW i = net.GetPinsEnumeratorW(); i.MoveNext(); pos++)
  {
    ::gPinX[pos] = i.X();
    ::gPinY[pos] = i.Y();
    ::gPinIds[pos] = ::ToID(i);
    ::gPinIndexes[pos] = pos;
  }

  //HACK: sort pins to get flute internal order
  //do not modify
  //sort indexes: y - first, x - second
  for (int q = 0; q < numOfPins - 1; q++)
  {
    int minIdx = q;
    for (int j = q + 1; j < numOfPins; j++)
    {
      if (  ::gPinY[::gPinIndexes[minIdx]] >  ::gPinY[::gPinIndexes[j]]
      || (::gPinY[::gPinIndexes[minIdx]] == ::gPinY[::gPinIndexes[j]]
      &&  ::gPinX[::gPinIndexes[minIdx]] >  ::gPinX[::gPinIndexes[j]]))
      {
        minIdx = j;
      }
    } //for (int j = q + 1; j < numOfPins; j++)
    ::Swap(::gPinIndexes + q, ::gPinIndexes + minIdx);
  }// for (int q = 0; q < numOfPins - 1; q++)


  FLUTE::Tree flTree = FLUTE::flute(numOfPins, ::gPinX, ::gPinY, ::gAccuracy);

  wire.SetLength(flTree.length);
  wire.SetRoutingType(RoutingType_Flute);

  int sourcePosition = -1;
  int sourceID = ::ToID(net.Source());

  //find all neighbours for each point
  memset(::gTreePoints, 0, sizeof(PointInfo) * pointsCount);
  for (int k = 0; k < pointsCount; k++)
  {
    if (k < numOfPins && ::gPinIds[::gPinIndexes[k]] == sourceID)
      sourcePosition = k;
    if (k != flTree.branch[k].n)
    {
      //first edge point
      ::gTreePoints[k].Idxs[::gTreePoints[k].Count] = flTree.branch[k].n;
      ::gTreePoints[k].Count++;

      //second edge point
      ::gTreePoints[flTree.branch[k].n].Idxs[::gTreePoints[flTree.branch[k].n].Count] = k;
      ::gTreePoints[flTree.branch[k].n].Count++;
    }
  }// for (int k = 0; k < pointsCount; k++)

  aDesign.SteinerPoints.InheritPinCoordinates(wire.Root());
  TemplateTypes<EdgeInfo>::stack edgesStack;
  
  EdgeInfo root;
  root.Id = ::ToID(wire.Root());
  root.SrcIdx = root.DestIdx = sourcePosition;
  edgesStack.push(root);

  //create A-tree from flat arrays
  while (!edgesStack.empty())
  {
    EdgeInfo parent = edgesStack.top();
    edgesStack.pop();
    HSteinerPoint point = ::__ConstructPtr<HSteinerPoint>(parent.Id);
  
    //cereate new edge for each point neighbour except parent point
    int edgesNum = 0;
    for (int k = 0; k < ::gTreePoints[parent.DestIdx].Count; k++)
      if (::gTreePoints[parent.DestIdx].Idxs[k] != parent.SrcIdx)
      {
        EdgeInfo nextEdge;
        nextEdge.SrcIdx = parent.DestIdx;
        nextEdge.DestIdx = ::gTreePoints[parent.DestIdx].Idxs[k];

        //get pin or create new Steiner point
        HSteinerPoint newPoint = nextEdge.DestIdx < numOfPins
          ? ::__ConstructPtr<HSteinerPoint>(::gPinIds[::gPinIndexes[nextEdge.DestIdx]])
          : aDesign.SteinerPoints.AllocateSteinerPoint();
        nextEdge.Id = ::ToID(newPoint);
        aDesign.Set<HSteinerPoint::X>(newPoint, flTree.branch[nextEdge.DestIdx].x);
        aDesign.Set<HSteinerPoint::Y>(newPoint, flTree.branch[nextEdge.DestIdx].y);
        edgesStack.push(nextEdge);

        if (edgesNum == 0)
          aDesign.Set<HSteinerPoint::Left>(point, newPoint);
        else if (edgesNum == 1)
          aDesign.Set<HSteinerPoint::Right>(point, newPoint);
        else
        {
          aDesign.SteinerPoints.RemoveSteinerTree(wire.Root());
          wire.SetRoutingType(RoutingType_Unrouted);
          wire.SetLength(0.0);
          LOGERRORFORMAT(("Not A-tree from FLUTE. Net: %s", net.Name().c_str()));
          return;
        }
        edgesNum++;
      }
  }// while (!edgesStack.empty())

  //FLUTE uses malloc to allocate this array. So we must delete it using free()
  free(flTree.branch);
}

void FLUTERoute(HDesign& aDesign)
{
  for (HNets::ActiveNetsEnumeratorW niter = aDesign.Nets.GetNetsEnumeratorW(); niter.MoveNext(); )
    FLUTERoute(aDesign, niter);
}