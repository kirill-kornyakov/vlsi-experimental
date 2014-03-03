#pragma once

#include "LCircuit.h"
#include "LNetList.h"
#include "LTech.h"
#include "LRouting.h"
#include "LRCExtraction.h"
#include "LTiming.h"
#include "LDetailedPlacement.h"

struct LDesign
{
  LTech *Tech;
  LCircuit Circuit;
  LNetList NetList;
  LRouting Routing;
  LRCExtraction RCExtraction;
  LTiming Timing;
  LDetailedPlacement DetailedPlacement;

  //clusterization
  //global placement
};
