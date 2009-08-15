#ifndef __REPORTING_H__
#define __REPORTING_H__

#include "HDesign.h"

//macro types reports
void ReportCellsByMacroFunction(HDesign& design);
//���������� cell ������� ��� MacroType
void ReportCellByMacroType(HDesign& design);
//����� ������� ��� ������� MacroType: ���, ������, ���, ���������� �����, 
//��� ������� ����: ���, �����������, �������������� ����������, �������� ������������ 
//�������� �������� �����������
void ReportMacroTypeInfo(HDesign& design);

//circuit reports
void ReportPlacementArea(HDesign& design);
void ReportWithTheCharacteristicsOfCircuits(HDesign& design);
void ReportBenchmarkStatistics(HDesign& design);

//nets reports
//���������� ���������� ����������, ���������� � �.�. �����.
void ReportNetsInfo(HDesign& design);
//for converting placement to Fast Router input format
void PrintToFastRouterFormat(HDesign& hd, char* fileName);
void PrintToBoxRouterFormat(HDesign& hd, char* fileName);
//���������� � ���������� ����� ������� ���� (all, skipped, buffered, removed, Active).
void ReportCountNetsWithTypes(HDesign& design);
void ReportNetPins(HDesign& hd, HNet net);
void ReportNetTiming(HDesign& design, HNet net);

//timing reports
struct CriticalPathsCriticality
{
  double criticality;
  HCriticalPath path;
};

struct NameSortCPC
{  
  bool operator()
    (const CriticalPathsCriticality& a, 
    const CriticalPathsCriticality& b) 
    const { return a.criticality  < b.criticality;  }
};
void ReportNegativeSlacks(HDesign&);
void ReportCriticalPathsToLogFile(HDesign&, int n = -1);
void PlotCriticalPathsAndSaveImages(HDesign&);
void PlotTopCriticalPathsAndSaveImages(HDesign& design, int n = -1);

#endif //__REPORTING_H__