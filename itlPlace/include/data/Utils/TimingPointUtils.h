#ifndef __TIMINGPOINTUTILS_H__
#define __TIMINGPOINTUTILS_H__

#include "HDesign.h"

namespace Utils
{
  double TNS(HDesign& design);
  double WNS(HDesign& design);

  void InsertNextPoint(HDesign& design, HTimingPoint pointToPlace, HTimingPoint afterThisPoint);
  void DeletePointInList(HDesign& design, HTimingPoint pointDelete);

  string MakePointFullName(HDesign& design, HTimingPoint point);
  void PrintTopologicalOrder(HDesign& design);
};

#endif //__TIMINGPOINTUTILS_H__