#ifndef __LOW_NETLIST_H__
#define __LOW_NETLIST_H__

#include "stdTypes.h"
#include "LTech.h"

enum CellOrientation
{
  Orientation_N  = 0,
  Orientation_W  = 1,
  Orientation_S  = 2,
  Orientation_E  = 3,
  Orientation_FN = 4,
  Orientation_FW = 5,
  Orientation_FS = 6,
  Orientation_FE = 7,
  Orientation_Default = 0
};

enum PlacementStatus
{
  PlacementStatus_Movable = 0,
  PlacementStatus_Fixed   = 1,
  PlacementStatus_Fictive = 2,
  PlacementStatus_Default = 0
};

enum NetKind
{
  NetKind_First    = 0,

  NetKind_Default  = 0,
  NetKind_Active   = 0,
  NetKind_Skipped  = 1,
  NetKind_Buffered = 2,
  NetKind_Removed  = 3,

  NetKind_Last     = 3
};

struct LCellTypeBound
{
  IDType startCellID;
  int nTerminals;
  int nFictive;
};

struct LNetList
{
  double *cellX;//nm  x of left down cell corner
  double *cellY;//nm  y of left down cell corner
  double *cellWidth; //nm
  double *cellHeight;//nm
  IDType *cellPinStartIdx;
  IDType *cellPinEndIdx;
  string *cellName;
  IDType *cellType;//from Tech
  CellOrientation* cellOrient;
  PlacementStatus* cellPlStatus;

  LCellTypeBound cellTypeBounds[MacroType_Last + 2]; //each element is start position
  IDType& nCellsEnd;
  IDType nCellsLimit;

  //pins
  //--------------------------------------------------------------------------
  //#|PI & PO|########|Cell pins|############################################|
  //--------------------------------------------------------------------------
  double *pinOffsetX; //nm  used as X coordinate for PI and PO
  double *pinOffsetY; //nm  used as Y coordinate for PI and PO
  IDType *pinType;//from tech
  string *pinName; //only for primaries
  PinDirection *pinDirection;
  IDType *pinCellIdx;
  IDType *pinNetIdx;
  IDType *pinOriginalNetIdx;

  //primaries
  IDType nPinsLimit;
  IDType nPrimariesEnd;

  //cell's pins
  IDType nPinsStart;
  IDType nPinsEnd;

  //nets
  //--------------------------------------------------------------------------
  //|#|Nets from benchmark|Nets added in runtime|############################|
  //--------------------------------------------------------------------------
  string*  netName;
  NetKind* netKind;
  double*  netHPWL;
  double*  netLnet;
  double*  netWeight;
  ///!!! SomeType **netBufferingInfo;

  IDType nNetsLimit;
  IDType nNetsEnd;
  IDType nNetsBaseEnd;//temporary bound; probably will be removed
  IDType nNetsByKind[NetKind_Last + 1];

  //Net pins
  //pin with number netPins[netPinStart[i]] - is net source!
  IDType *netPins; //pins indexes
  IDType *netPinStart;
  IDType *netPinEnd;

  IDType nNetPinEnd;
  IDType nNetPinLimit;

  LNetList(): nCellsEnd(cellTypeBounds[MacroType_Last + 1].startCellID)
  {
    cellHeight = 0;
    cellName = 0;
    cellOrient = 0;
    cellPinEndIdx = 0;
    cellPinStartIdx = 0;
    cellX = 0;
    cellY = 0;
    cellType = 0;
    cellPlStatus = 0;
    cellWidth = 0;
    pinCellIdx = 0;
    pinNetIdx = 0;
    pinOffsetX = 0;
    pinOffsetY = 0;
    pinOriginalNetIdx = 0;
    pinType = 0;
    pinDirection = 0;
    pinName = 0;
    netName = 0;
    netKind = 0;
    netPins = 0;
    nCellsLimit = 0;
    nPinsEnd = 0;
    nPinsLimit = 0;
    nPinsStart = 0;
    nNetsBaseEnd = 0;
    nNetsEnd = 0;
    nNetsLimit = 0;
    nNetPinEnd = 0;
    nNetPinLimit = 0;
    netPinStart = 0;
    netPinEnd = 0;
    nPrimariesEnd = 0;
    netHPWL = 0;
    netWeight = 0;
    netLnet = 0;

    for (int i = 0; i < MacroType_Last + 2; i++)
    {
      cellTypeBounds[i].startCellID = 0;
      cellTypeBounds[i].nTerminals = 0;
      cellTypeBounds[i].nFictive = 0;
    }

    for (int j = 0; j < NetKind_Last + 1; j++)
      nNetsByKind[j] = 0;
  }
};

#endif //__LOW_NETLIST_H__