#pragma once 

#pragma warning(disable:4355)

#include "HSteinerPoint.h"
#include "HCircuit.h"
#include "HMacroType.h"
#include "HPinType.h"
#include "HRoutingLayer.h"
#include "HTimingArcType.h"
#include "HCell.h"
#include "HPin.h"
#include "HNet.h"
#include "HWire.h"
#include "HSite.h"
#include "HPlacementRow.h"
#include "HTimingPoint.h"
#include "HCriticalPath.h"
#include "HCriticalPathPoint.h"

#include "HPlotter.h"
#include "OpenCVPlotter.h"

#define HDESIGNCOLLECTION(field_name, col_type) \
public: \
  col_type field_name; \
  col_type::WrapperType operator[](col_type::ItemType obj) \
    { return field_name.MakeWrapper(obj); } \
  template<int prop, class RetType> \
  RetType Get(col_type::ItemType item) const { return field_name.Get<prop, RetType>(item); } \
  template<int prop, class ArgType> \
  void Set(col_type::ItemType item, const ArgType& value) { field_name.Set<prop>(item, value); } \
  template<int prop> \
  double GetDouble(col_type::ItemType item) const { return field_name.Get<prop, double>(item); } \
  template<int prop> \
  int GetInt(col_type::ItemType item) const { return field_name.Get<prop, int>(item); } \
  template<int prop> \
  bool GetBool(col_type::ItemType item) const { return field_name.Get<prop, bool>(item); } \
  template<int prop> \
  string GetString(col_type::ItemType item) const { return field_name.Get<prop, string>(item); }

/* Attention!!!
 *  distance units - nm   (nanometers)
 *  capacitance    - pf   (picofarads)
 *  resistance     - kohm (kiloohms)
 *  time           - ns   (nanoseconds)
 */
class HDesign
{
public:
  LDesign _Design;

  HDesign(): _Design(),            Circuit(&_Design),    MacroTypes(this),
             PinTypes(this),       RoutingLayers(this),  TimingArcTypes(this),
             Cells(this),          Pins(this),           Nets(this),
             SteinerPoints(this),  Wires(this),          Sites(this),
             PlacementRows(this),  TimingPoints(this),   CriticalPaths(this),
             CriticalPathPoints(this)
  {
    _Design.Tech = 0;
    //Plotter = new HPlotter(*this);
    Plotter = new OpenCVPlotter(*this);
  }

  HCircuit Circuit;
  HPlotter *Plotter;
  libconfig::ConfigExt cfg;

  HDESIGNCOLLECTION(Nets, HNets)
  HDESIGNCOLLECTION(MacroTypes, HMacroTypes)
  HDESIGNCOLLECTION(PinTypes, HPinTypes)
  HDESIGNCOLLECTION(RoutingLayers, HRoutingLayers)
  HDESIGNCOLLECTION(TimingArcTypes, HTimingArcTypes)
  HDESIGNCOLLECTION(Cells, HCells)
  HDESIGNCOLLECTION(Pins, HPins)
  HDESIGNCOLLECTION(SteinerPoints, HSteinerPoints)
  HDESIGNCOLLECTION(Wires, HWires)
  HDESIGNCOLLECTION(Sites, HSites)
  HDESIGNCOLLECTION(PlacementRows, HPlacementRows)
  HDESIGNCOLLECTION(TimingPoints, HTimingPoints)
  HDESIGNCOLLECTION(CriticalPaths, HCriticalPaths)
  HDESIGNCOLLECTION(CriticalPathPoints, HCriticalPathPoints)

  void Initialize();
  bool HasTechInfo() const {return _Design.Tech != 0;}
  void SetTechInfo(HDesign* design = 0);
  bool CanDoTiming() const;
};

inline void HDesign::SetTechInfo(HDesign* design)
{
  if(!design)
    _Design.Tech = new LTech();
  else
    _Design.Tech = design->_Design.Tech;
}

inline bool HDesign::CanDoTiming() const
{
  return cfg.ValueOf("benchmark.CanDoTiming", false)
    && TimingArcTypes.IsInitialized();
}

template<typename H>
inline typename H::WrapperType operator, (H obj, HDesign& design) { return design[obj]; }

template<typename H>
inline typename H::WrapperType operator, (HDesign& design, H obj) { return design[obj]; }

#include "HExternalMethods.h"
