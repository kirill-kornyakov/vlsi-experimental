#include "LRSizing.h"
#include <math.h>
#include "LinearRegression.h"
#include "TimingUtils.h"
#include "AdaptiveRoute.h"
#include <algorithm>
#include <limits>

#define accuracyForLRS_Mu 0.001
#define DOUBLE_ACCURACY 1e-4
#define errorBoundForLDP 0.01

#define MODE_TEST 1

//TODO: function for cell family name
//TODO: class CellFamily 
//TODO: GetCellFamilyRC()
//TODO ����� ���� ����� � ��� ������� �������� ������ ���
void LRSizer::PrintMacroTypeCharacteristics()
{
  FILE* macroCharacteristics = fopen("./log/macroCharacteristics.csv", "w");
  fprintf (macroCharacteristics, "cellName;cellSize;pinName;pinDirection;C;signalDirection;R;T;\n");
  for (HMacroTypes::EnumeratorW macroTypeE = design.MacroTypes.GetEnumeratorW(); macroTypeE.MoveNext(); )
  {
    double cellSize = GetMacroTypeSize(macroTypeE);
    std::string cellName = macroTypeE.Name();
    for(HMacroType::PinsEnumeratorW pinE = macroTypeE.GetEnumeratorW(); pinE.MoveNext();)
    {
      std::string pinName = pinE.Name();
      if (pinE.Direction() == PinDirection_INPUT)
      {
        Utils::DriverPhisics driverPhisics = Utils::GetDriverWorstPhisics(design, pinE, SignalDirection_None);
        fprintf(macroCharacteristics, "%s;%lf;%s;Input;%lf;-; ; \n", 
          cellName.data(), cellSize, pinName.data(), driverPhisics.C);
      }
      else if(pinE.Direction() == PinDirection_OUTPUT)
      {
        for(HPinType::ArcsEnumeratorW arcE = pinE.GetArcsEnumeratorW(); arcE.MoveNext();)
        {
          HPinType pinWhichConnectsToPinE = arcE.GetStartPinType(pinE);
          std::string signalDirection = design[pinWhichConnectsToPinE].Name() + "->" + pinName;
          fprintf(macroCharacteristics, "%s;%lf;%s;Output; ;%s;%lf;%lf\n", 
            cellName.data(), cellSize, pinName.data(), signalDirection.data(), arcE.ResistanceRise(), 
            arcE.TIntrinsicRise());
        }
      }
    }
  }
  fclose(macroCharacteristics);
}

void LRSizer::PrintPinsCharacteristics()
{
  FILE* pinsCharacteristics = fopen("./log/pinsCharacteristics.csv","w");
  fprintf (pinsCharacteristics, "cellName;pinName;id;pinDirection;ArrivalTime;C;R;T\n");
  HPin pin;
  string pinName, cellName, direction="null";
  HCell cell;
  Utils::DriverPhisics DPhisics;
  for (HTimingPointWrapper tp = design[design.TimingPoints.TopologicalOrderRoot()].GoNext(); !::IsNull(tp);
    tp.GoNext())
  {
    if(!::IsNull(tp))
    {
      pin = tp.Pin(); 
      if (design[(design,tp).Pin()].IsPrimaryInput())
      {

        fprintf (pinsCharacteristics, "%s;;;;%lf\n", "PrimaryInput", tp.ArrivalTime());
        continue;
      }
      if (design[(design,tp).Pin()].IsPrimaryOutput())
      {
        fprintf (pinsCharacteristics, "%s;;;;%lf\n", "PrimaryOutput", tp.ArrivalTime());
        continue;
      }
      DPhisics = Utils::GetDriverWorstPhisics(design, pin, SignalDirection_None);
      pinName = (pin,design).Name();
      cell = (design,pin).Cell();
      cellName = design.MacroTypes.GetString<HMacroType::Name>((cell,design).Type());
      if ((pin,design).Direction() == PinDirection_INPUT) 
        direction="INPUT";
      else 
      {
        if ((pin,design).Direction() == PinDirection_OUTPUT) direction = "OUTPUT";
      }
      fprintf (pinsCharacteristics, "%s;%s;%d;%s;%lf;%lf;%lf;%lf\n", cellName.data(), pinName.data(), 
        ::ToID(tp),direction.data(), tp.ArrivalTime(), DPhisics.C, DPhisics.R, DPhisics.T);
    }
  }
  fclose(pinsCharacteristics);
}

void LRSizer::ApplySizing(std::vector<double>& X)
{
  ASSERT(cells->size() == X.size());
  std::vector<HCell>::iterator cellFrom = cells->begin();
  std::vector<double>::iterator currentX = X.begin();
  int i = 0;//DEBUG
  int numOfChanges = 0;
  while(cellFrom != cells->end() && currentX != X.end())
  {
    HMacroType cellToType = RoundCellToTypeFromLib(*cellFrom, *currentX);
    //DEBUG
    double roundingToSize = GetMacroTypeSize(cellToType);
    //ALERT("Rounding from %lf to %lf", *currentX, roundingToSize);
    HMacroType cellFromType = design[*cellFrom].Type();
    i++;
    if (cellFromType != cellToType)
    {
      numOfChanges++;

      double fromWidth = GetMacroTypeSize(cellFromType);
      double toWidth = GetMacroTypeSize(cellToType);
      ALERT("%d: We are changing it from %s (%lf) to %s (%lf)", i, 
        (design.MacroTypes.GetString<HMacroType::Name>(cellFromType)).data(), fromWidth, 
        (design.MacroTypes.GetString<HMacroType::Name>(cellToType)).data(), toWidth);
    }
    //END OF DEBUG

    (*cellFrom, design).SetType(cellToType);
    (*cellFrom, design).SetHeight(design.GetDouble<HMacroType::SizeY>(cellToType));
    (*cellFrom, design).SetWidth(design.GetDouble<HMacroType::SizeX>(cellToType));
    cellFrom++;
    currentX++;
  }
  ALERT("Changed types of %d elements", numOfChanges);
}

void LRSizer::GetCellFamily(HCell cell, std::vector<HMacroType>& macroTypesInFamily)
{
  string cellFamily = GetMacroTypeFamilyName((cell,design).Type());
  for (HMacroTypes::EnumeratorW macroTypeE = design.MacroTypes.GetEnumeratorW(); macroTypeE.MoveNext(); )
  {
    string macroTypeFamily = GetMacroTypeFamilyName(macroTypeE);
    if (macroTypeFamily == cellFamily)
      macroTypesInFamily.push_back(macroTypeE);
  }
  std::sort(macroTypesInFamily.begin(), macroTypesInFamily.end(), MacroTypeSizesComparator(design));
}

void LRSizer::GetCellFamily(HCell cell, std::vector<double>& cellSizes)
{
  cellSizes.clear();
  std::vector<HMacroType> macroTypesInFamily;
  GetCellFamily(cell, macroTypesInFamily);

  for(std::vector<HMacroType>::iterator macroType = macroTypesInFamily.begin(); 
    macroType != macroTypesInFamily.end(); macroType++)
  {
    double size = GetMacroTypeSize(*macroType);
    if (std::find(cellSizes.begin(), cellSizes.end(), size) == cellSizes.end())
      cellSizes.push_back(size);
  }
  std::sort(cellSizes.begin(), cellSizes.end());
}

void LRSizer::GetPinFamilyC(HPin pin, std::vector<double>& C, std::vector<double>& X)
{
  string cellFamily = GetMacroTypeFamilyName(design[design[pin].Cell()].Type());
  string pinName = design[pin].Name();
  std::vector<double> cellSizes;
  std::vector<HMacroType> macroTypesInFamily;
  GetCellFamily(design[pin].Cell(), macroTypesInFamily);
  for (std::vector<HMacroType>::iterator macroType = macroTypesInFamily.begin(); 
    macroType != macroTypesInFamily.end(); macroType++)
  {
    double size = GetMacroTypeSize(*macroType);
    if (std::find(cellSizes.begin(), cellSizes.end(), size) == cellSizes.end())
    {
      cellSizes.push_back(size);
      for (HMacroType::PinsEnumeratorW pinE = design[*macroType].GetEnumeratorW(); pinE.MoveNext(); )
      {
        if ((pinE.Direction() == PinDirection_INPUT) || (pinE.Direction() == PinDirection_OUTPUT))
        {
          if(pinE == design[pin].Type())
          {
            C.push_back(Utils::GetSinkCapacitance(design, pinE, SignalDirection_None));
            X.push_back(GetMacroTypeSize(*macroType)); 
            break;
          }
        }
      }
    }
  }
  ASSERT(C.size() == X.size());
}

void LRSizer::GetArcFamilyR(HPin startPin, HPin endPin, std::vector<double>& R, std::vector<double>& invX)
{
  ASSERT((startPin, design).Direction() == PinDirection_INPUT);
  ASSERT((endPin, design).Direction() == PinDirection_OUTPUT);
  ASSERT((endPin, design).Cell() == (startPin, design).Cell());//Pins are on the same cell
  std::vector<HMacroType> macroTypesInFamily;
  GetCellFamily(design[endPin].Cell(), macroTypesInFamily);
  for (std::vector<HMacroType>::iterator macroType = macroTypesInFamily.begin(); 
    macroType != macroTypesInFamily.end(); macroType++)
  {
    for(HMacroType::PinsEnumeratorW pinE = design[*macroType].GetEnumeratorW(); pinE.MoveNext();)
    {
      if(pinE.Direction() == PinDirection_OUTPUT && pinE == design[endPin].Type())
      {
        for(HPinType::ArcsEnumeratorW arcE = pinE.GetArcsEnumeratorW(); arcE.MoveNext();)
        {
          HPinType possibleStartPinConnectedToPinE = arcE.GetStartPinType(pinE);
          if (possibleStartPinConnectedToPinE == (startPin, design).Type())
          {
            R.push_back(arcE.ResistanceRise());
            invX.push_back(1.0 / GetMacroTypeSize(*macroType));
          }
        }
      }
    }
  }
  ASSERT(R.size() == invX.size());
}

std::string LRSizer::GetMacroTypeFamilyName(HMacroType macroType)
{
  std::string macroTypeName = design.MacroTypes.GetString<HMacroType::Name>(macroType);
  return macroTypeName.substr(0, macroTypeName.length()-2);
}

int LRSizer::CountLeftArcs(HDesign& design, HNet net)
{
  return CountLeftArcs(design, design[net].Source());
}

int LRSizer::CountLeftArcs(HDesign& design, HPin pin)
{
  if (design.Get<HPin::Direction, PinDirection>(pin) == PinDirection_INPUT)
  {
    if (!::IsNull(design.Get<HPin::Net, HNet>(pin)))
      return 1;
    return 0;
  }
  HPinType pinType = design.Get<HPin::Type, HPinType>(pin);
  int i = 0;
  for (HPinType::ArcsEnumeratorW arc = design[pinType].GetArcsEnumeratorW(); arc.MoveNext(); )
    if (arc.TimingType() == TimingType_Combinational)
      i++;
  return i;
}

LRSizer::LRSizer(HDesign& hd): design(hd), LambdaIn(0)
{
  size = design._Design.NetList.nPinsLimit;
  ::Grow(&LambdaIn, 0, size);
  //InitLambda(design, 1);
  cells = new std::vector<HCell>;
  InitCells(cells);

  //DEBUG
  PrintPinsCharacteristics();
  //GetStPEdges();
  PrintMacroTypeCharacteristics();
  //END OF DEBUG
}

LRSizer::~LRSizer()
{
  ::Grow(&LambdaIn, size, 0);
  delete cells;
}

void LRSizer::InitLambda(double defaultLambda)
{
  for (HTimingPointWrapper tp = design[design.TimingPoints.FirstInternalPoint()]; !::IsNull(tp); tp.GoNext())
  {
    int arcsCount = CountLeftArcs(design, tp.Pin());
    if (arcsCount > 0)
      GetPointLambdas(tp).resize(arcsCount, defaultLambda);
  }
}

void LRSizer::InitCells(std::vector<HCell>* cells)
{
  int index = 1;
  HCell tempCell;
  //Going through whole circuit in reverse topological order
  for (HTimingPointWrapper tp = design[design.TimingPoints.TopologicalOrderRoot()]; 
    !::IsNull(tp.GoPrevious());)
  {
    CRITICAL_ASSERT(!::IsNull(tp));
    HPin pin = tp.Pin();
    tempCell = design[pin].Cell();
    if (::IsNull(tempCell)) continue;
    bool isCellNotInCells = true;
    for (std::vector<HCell>::iterator cell = cells->begin(); cell != cells->end(); cell++)
      if (*cell == tempCell){isCellNotInCells = false; break;}
      if (isCellNotInCells)
        cells->push_back(tempCell);
  }
}

void LRSizer::SolveLrsMu(std::vector<double>& NewVX)
{
  std::vector<double> vMu;
  vMu.resize(size, -1);
  FillVMu(vMu);
  std::vector<double> vX;
  InitVxByLowerBound(vX); //TODO Place for optimization //for all x_i=L_i
  NewVX.resize(vX.size());
  NewVX.assign(vX.begin(), vX.end());
  int stepCounter = 1;
  do
  {
    ///for (HTimingPointWrapper tp = design[design.TimingPoints.TopologicalOrderRoot()]; 
    /// !::IsNull(tp.GoNext()); )
    ///{
    ///  double Ui = 2.0;//TODO: replace these constants with correct bounds
    ///  double Li = 0.5;
    ///  double ri = 1;
    ///  double ci = 1;

    ///  double mu = vMu[::ToID(tp)];
    ///  //NewVX[] = min(
    ///}
    //ALERT("Size of vX before before =%d", vX.size());

    vX.clear();
    vX=NewVX;
    UpdateVX(NewVX, vMu);
    ALERT("Criterion on %d-th iteration of LRS_MU is %lf", stepCounter, CalcCriterion(NewVX));
    stepCounter++;
  } while(!CheckStopConditionForLRS_Mu(vX, NewVX, accuracyForLRS_Mu));

  ALERT("Number of iterations of LRS_MU is %d", stepCounter - 1);
}

double LRSizer::CalcB(HCell& cell,std::vector<double>& vMu,std::vector<double>& vX )
{
  double B = 0;
  /*TODO: Rewrite method according to the fact that R belongs to ARC
  Maths::Regression::Linear* regressionR;
  HTimingPoint tp;
  for (HCell::PinsEnumeratorW pin = (design, cell).GetPinsEnumeratorW(); pin.MoveNext(); )
  {
    if (pin.Direction() == PinDirection_OUTPUT && !::IsNull(pin.Net()))
    {
      tp = design.TimingPoints[pin];
      regressionR = GetRegressionR((design.TimingPoints[pin], design));
      int index = (int)distance(cells->begin(), find(cells->begin(), cells->end(), cell));
      B += vMu[::ToID(tp)] * GetObservedC(tp,vX) * (regressionR->getValue(1 / vX[index])) * vX[index];
    }
  }*/
  return B;
}

double LRSizer::CalcA(HCell& cell, std::vector<double>& vMu, std::vector<double>& vX)
{
  double A = 0;
  /*TODO: Rewrite method according to the fact that R belongs to ARC
  Maths::Regression::Linear* regressionC;
  HTimingPoint tp;
  for (HCell::PinsEnumeratorW pin = (design, cell).GetPinsEnumeratorW(); pin.MoveNext(); )
  {
    if (pin.Direction() == PinDirection_INPUT && !::IsNull(pin.Net()))
    {
      tp = design.TimingPoints[pin];
      regressionC = GetRegressionR((design.TimingPoints[pin], design));
      int index = (int)distance(cells->begin(), find(cells->begin(), cells->end(), cell));
      A += (GetWeightedResistance(tp, vX, vMu) + 1) * (regressionC->getValue(vX[index])) / vX[index];
    }
  }*/
  return A;
}

double LRSizer::CalcNewX(HCell& cell,std::vector<double>& vMu,std::vector<double>& vX)
{
  double root = sqrt(CalcB(cell,vMu,vX) / CalcA(cell, vMu, vX)); 
  std::vector<double> cellSizes;
  GetCellFamily(cell, cellSizes);
  return min(cellSizes.back(), max(cellSizes.front(), root));
}

void LRSizer::UpdateVX(std::vector<double>& newVX, std::vector<double>& vMu)
{
  //newVX.reserve(cells.size());
  std::vector<double> tempNewVX;
  double newX = 0;
  for(unsigned int i = 0; i < cells->size(); i++)
  {
    newX = CalcNewX((*cells)[i], vMu, newVX);
    tempNewVX.push_back(newX);
  }
  newVX.clear();
  newVX.assign(tempNewVX.begin(), tempNewVX.end());
}

void LRSizer::FillVMu(std::vector<double>& vMu)
{
  //Sum of all Lambda inputs of the point
  for (HTimingPointWrapper tp = design[design.TimingPoints.FirstInternalPoint()]; !::IsNull(tp); tp.GoNext() )
  {
    double lambdaSum = 0;
    for (std::vector<double>::iterator lambda = GetPointLambdas(tp).begin();
      lambda != GetPointLambdas(tp).end(); lambda++)
    {
      lambdaSum += *lambda;
    }
    vMu[::ToID(tp)] = lambdaSum;
  }
}

void LRSizer::InitVxByLowerBound(std::vector<double>& vX)
{
  vX.reserve(cells->size());
  std::vector<double> cellSizes;
  for (std::vector<HCell>::iterator cellIter = cells->begin(); cellIter != cells->end(); cellIter++)
  {
    GetCellFamily(*cellIter, cellSizes);
    vX.push_back(*cellSizes.begin());
  }
}

void LRSizer::initVX(std::vector<double>& vX)
{
  vX.clear();
  for(std::vector<HCell>::iterator iter = cells->begin(); iter != cells->end(); iter++)
    vX.push_back(GetMacroTypeSize((*iter, design).Type()));
}

double LRSizer::GetObservedC( HTimingPoint tp,std::vector<double>& vX )
{
  HPinWrapper pin = design[(design,tp).Pin()];
  Maths::Regression::Linear* regressionC;
  if (pin.Direction() == PinDirection_INPUT)
  { 
    regressionC = GetRegressionC((tp, design));
    HCell cell = pin.Cell();
    int index = (int)distance(cells->begin(), find(cells->begin(), cells->end(), cell));
    return (regressionC->getValue(vX[index]));
  }
  HNet net = pin.Net();
  if (pin.Direction() == PinDirection_OUTPUT && !::IsNull(net))
  {
    double sum = (design.SteinerPoints[pin],design).ObservedC();
    for(HNet::SinksEnumeratorW sink = (design,net).GetSinksEnumeratorW(); sink.MoveNext(); )
    {
      //sink can be PO!! ERROR TODO
      if(sink.IsPrimaryOutput()) continue;//This is a DIRTY patch!
      regressionC = GetRegressionC((design.TimingPoints[sink], design));
      HCell cell = sink.Cell();
      int index = (int)distance(cells->begin(), find(cells->begin(), cells->end(), cell));
      sum += (regressionC->getValue(vX[index])) - (sink.Type(), design).Capacitance();
    }
    return sum;
  }
  return 0;
}

double LRSizer::GetWeightedResistance(HTimingPoint tp, std::vector<double>& vX, std::vector<double>& vMu)
{
  /*TODO: Rewrite method according to the fact that R belongs to ARC
  HPinWrapper pin = design[(design,tp).Pin()];
  Maths::Regression::Linear* regressionR;
  if (pin.Direction() == PinDirection_INPUT)
  {
    if (pin.IsPrimaryInput())
      return 0; // u_pin * R_pin_from_EVR;
    HPin src = (design, pin.Net()).Source();
    if ((src, design).IsPrimary()) return 0; //TODO: It is a DIRTY HACK
    regressionR = GetRegressionR((design.TimingPoints[src], design));
    HCell cell = (src, design).Cell();
    int index = (int)distance(cells->begin(), find(cells->begin(), cells->end(), cell));
    return (vMu[::ToID(design.TimingPoints[src])] * (regressionR->getValue(1 / vX[index]))); // + R_wire(from srs to pin);
    //return 0;// u_src * r_src / x_src + R_wire(from srs to pin);
  }

  HNet net = pin.Net();
  if (pin.Direction() == PinDirection_OUTPUT && !::IsNull(net))
  {
    HCellWrapper cell = design[pin.Cell()];
    double sum = 0;
    for(HCell::PinsEnumeratorW inputPin = cell.GetPinsEnumeratorW(); inputPin.MoveNext(); )
      if (inputPin.Direction() == PinDirection_INPUT && !::IsNull(inputPin.Net()))
      {
        regressionR = GetRegressionR((design.TimingPoints[inputPin], design));
        HCell cell = (inputPin,design).Cell();
        int index = (int)distance(cells->begin(), find(cells->begin(), cells->end(), cell));
        sum += vMu[::ToID(design.TimingPoints[inputPin])] * (regressionR->getValue(1 / vX[index]));
        //sum += 0; //u_inputPin * r_inputPin / x_inputPin;
      }
  }
  */
  return 0;
}

bool LRSizer::CheckStopConditionForLRS_Mu(std::vector<double> prevVX, std::vector<double> nextVX, 
                                          double accuracy)
{
  double max = 0;
  if (prevVX.size() != nextVX.size())
  {
    ALERT("delta size!!! = %d", nextVX.size() - prevVX.size());
    return false;		
  }
  for(unsigned int i = 0; i < prevVX.size(); i++)
  {
    if (fabs(prevVX[i] - nextVX[i]) > max) 
      max = fabs(prevVX[i] - nextVX[i]);
  }

  if (max < accuracy) return true;
  else return false;
}

void LRSizer::CheckKuhn_Tucker()
{
#ifdef DEBUG
  HTimingPointWrapper tp = design[design.TimingPoints.TopologicalOrderRoot()];
  for (; !::IsNull(tp); tp.GoNext())
  {
    if (!CheckKuhn_Tucker(tp)) ALERT("Check KuhnTucker=false on ::toID(tp)=%d", ::ToID(tp));
  }
#endif
}


void LRSizer::SolveLDP()
{
  int stepCounter = 1;
  //1. Set initial lambdas
  InitLambda(1);
  ProjectLambdaMatrix();
  CheckKuhn_Tucker();

  std::vector<double> vX;

  do{
    SolveLrsMu(vX);

    //ALERT("Criterion on %d-th iteration is %lf", stepCounter, CalcCriterion(vX));

    std::vector<double> arrivalTimes = GetArrivalTimes(vX);

    AdjustLambda(stepCounter, arrivalTimes, vX);
    ProjectLambdaMatrix();

    stepCounter++;
  } while (!CheckStopConditionForLDP(vX, errorBoundForLDP));

  ALERT("Number of iterations in SOLVE LDP: %d", stepCounter - 1);
}

double LRSizer::FindOutputLambdaSum(HTimingPoint point)
{
  double sumOfLambda = 0;
  //�������� ����
  HPin pin = design.Get<HTimingPoint::Pin, HPin>(point);
  if (design.Get<HPin::Direction, PinDirection>(pin) == PinDirection_OUTPUT)
  {
    HNet net = (design,pin).Net();
    ASSERT(design[net].Source() == pin);
    for(HNet::SinksEnumeratorW sink = (design,net).GetSinksEnumeratorW(); sink.MoveNext(); )
    {
      std::vector<double>& lambdas = GetPointLambdas(design.TimingPoints[sink]);
      ASSERT(lambdas.size() == 1);
      sumOfLambda += lambdas[0];
    }
    return sumOfLambda;
  }
  if (design.Get<HPin::Direction, PinDirection>(pin) == PinDirection_INPUT)
  {
    //cell input pin, iterate cell arcs
    HPin inputPin = pin;
    HCell cell = (design, inputPin).Cell();
    //���������
    // (design,cell) \
    // (cell,design) --> HCellWrapper
    // design[cell]  /
    // design.Cells.MakeWrapper(cell)

    for (HCell::PinsEnumeratorW outputPin = (design, cell).GetPinsEnumeratorW(); outputPin.MoveNext(); )
    {
      if (outputPin.Direction() != PinDirection_OUTPUT || ::IsNull(outputPin.Net())) 
        continue;
      int i=0;
      HTimingPoint timingPointOfOutputPin = design.TimingPoints[outputPin];
      for (HPinType::ArcsEnumeratorW arc = (design,outputPin.Type()).GetArcsEnumeratorW(); arc.MoveNext(); )
      {
        if (arc.TimingType() != TimingType_Combinational) continue;
        if (arc.GetStartPin(outputPin) == inputPin)
          sumOfLambda += GetPointLambdas(timingPointOfOutputPin)[i];
        i++;
      }
    }
  }
  return 0;
}

double LRSizer::FindInputLambdaSum(HTimingPoint point)
{
  double lambdaSum = 0;
  for (std::vector<double>::iterator lambda = GetPointLambdas(point).begin();
    lambda != GetPointLambdas(point).end(); lambda++)
    lambdaSum += *lambda;
  return lambdaSum;
}

bool LRSizer::CheckKuhn_Tucker( HTimingPoint point)
{
  return (fabs(FindOutputLambdaSum(point) - FindInputLambdaSum(point)) < DOUBLE_ACCURACY);
}

double LRSizer::GetInputTimingPointAT(std::vector<double>& vX, HTimingPoint& tp, 
                                       std::vector<double>& arrivalTimes)
{
  HPinWrapper src = design[(design[design.Get<HPin::Net, HNet>(design.Get<HTimingPoint::Pin, HPin>(tp))]).Source()];
  HSteinerPointWrapper point = design[design.SteinerPoints[design[design.Get<HTimingPoint::Pin, HPin>(tp)]]];
  HSteinerPointWrapper srcPoint = design[design.SteinerPoints[src]];
  HSteinerPointWrapper nextPoint = srcPoint;  
  HSteinerPointWrapper nextRoot = point;
  HSteinerPointWrapper previousRoot = point;
  HSteinerPointWrapper srcP = srcPoint;
  TemplateTypes<HSteinerPoint>::stack points;
  double obsWireC, srcWireR,sinkCap;
  double aT = 0;
  while (GetParentStPoint(nextRoot, srcPoint) != nextRoot)
  {
    obsWireC = 0;
    srcWireR = 0;
    sinkCap = 0;
    nextRoot = GetParentStPoint(nextRoot, srcPoint);
    if (nextRoot.Left() == previousRoot)
      nextPoint = nextRoot.Left();
    else
      nextPoint = nextRoot.Right();
    obsWireC += design.GetDouble<HSteinerPoint::ExtractedC>(nextPoint) / 2;
    HPin nextPointPin = design.Get<HSteinerPoint::Pin, HPin>(nextPoint);
    if (!::IsNull(nextPointPin) && design.Get<HPin::Direction, PinDirection>(nextPointPin) == PinDirection_INPUT &&
      !::IsNull(design[nextPointPin].Cell()))
    {
      Maths::Regression::Linear* regressionC = GetRegressionC((design.TimingPoints[nextPointPin],design));
      int index = (int)distance(cells->begin(), find(cells->begin(), cells->end(), design[nextPointPin].Cell()));
      obsWireC += (regressionC->getValue(vX[index]));
    }
    srcWireR = design.GetDouble<HSteinerPoint::ExtractedR>(nextPoint);
    points.push(nextPoint);
    srcP = nextRoot;
    previousRoot = nextRoot;
    
    while (!points.empty()) points.pop(); //Clear stack
    points.push(nextPoint);   
    
    while (!points.empty())
    {
      srcP = points.top();
      points.pop();
      
      if (srcP.HasLeft())
      {
        nextPoint = srcP.Left();
        obsWireC += design.GetDouble<HSteinerPoint::ExtractedC>(nextPoint);
        HPin leftPointPin = design.Get<HSteinerPoint::Pin, HPin>(nextPoint);
        
        if (!::IsNull(leftPointPin) && design.Get<HPin::Direction, PinDirection>(leftPointPin) == PinDirection_INPUT &&
          !::IsNull(design[leftPointPin].Cell()))
        {
          Maths::Regression::Linear* regressionC = GetRegressionC((design.TimingPoints[leftPointPin],design));
          int index = (int)distance(cells->begin(), find(cells->begin(), cells->end(), design[leftPointPin].Cell()));
          obsWireC += (regressionC->getValue(vX[index]));
        }
        points.push(nextPoint);
        
        if (srcP.HasRight())
        {
          nextPoint = srcP.Right();
          obsWireC += design.GetDouble<HSteinerPoint::ExtractedC>(nextPoint);
          HPin rightPointPin = design.Get<HSteinerPoint::Pin, HPin>(nextPoint);
          if (!::IsNull(rightPointPin) && design.Get<HPin::Direction, PinDirection>(rightPointPin) == PinDirection_INPUT &&
            !::IsNull(design[rightPointPin].Cell()))
          {
            Maths::Regression::Linear* regressionC = GetRegressionC((design.TimingPoints[rightPointPin],design));
            int index = (int)distance(cells->begin(), find(cells->begin(), cells->end(), design[rightPointPin].Cell()));
            obsWireC += (regressionC->getValue(vX[index]));
          }
          points.push(nextPoint);
        }
      }
    }
    aT += srcWireR * obsWireC;
  }
  aT += arrivalTimes[::ToID(design.TimingPoints[src])];
  return aT;
}

HSteinerPointWrapper LRSizer::GetParentStPoint(HSteinerPointWrapper& point,HSteinerPointWrapper& srcPoint)
{
  TemplateTypes<HSteinerPoint>::stack points;
  HSteinerPointWrapper nextPoint = srcPoint;
  HSteinerPointWrapper parentPoint = srcPoint;
  points.push(srcPoint);
  while (!points.empty())
  {
    parentPoint = points.top();
    points.pop();
    if (parentPoint.HasLeft())
    {
      nextPoint = parentPoint.Left();
      if (nextPoint == point)
        return parentPoint;
      points.push(nextPoint);
      if (parentPoint.HasRight())
      {
        nextPoint = parentPoint.Right();
        if (nextPoint == point)
          return parentPoint;
        points.push(nextPoint);
      }
    }
  }
  return point;
}

double LRSizer::GetOutputTimingPointAT(std::vector<double>& vX, HTimingPoint& tp, 
                                           std::vector<double>& arrivalTimes)
{
  Maths::Regression::Linear* regressionR;
  HPin pin = design.Get<HTimingPoint::Pin, HPin>(tp);
  HCell cell = design[(design, tp).Pin()].Cell();
  int index = (int)distance(cells->begin(), find(cells->begin(), cells->end(), cell));
  double x = vX[index];
  double R = 0;
  double T = 0, aT = 0, maxDelay = 0;
  HPin startPin;

  HPinType sourceType = design.Get<HPin::Type, HPinType>(pin);

  for (HPinType::ArcsEnumeratorW arc = design[sourceType].GetArcsEnumeratorW(); arc.MoveNext(); )
  {
    if (arc.TimingType() == TimingType_Combinational)
    {
      startPin = arc.GetStartPin(pin);
      aT = arrivalTimes[::ToID(design.TimingPoints[startPin])];
      T = arc.TIntrinsicRise();
      regressionR = GetRegressionR(startPin, pin);
      R = regressionR->getValue(1 / x);
      if (maxDelay < (T + aT + R * GetObservedC(tp, vX))) 
        maxDelay = T + aT + R * GetObservedC(tp, vX);
    }
  }
  return (maxDelay);
}

std::vector<double> LRSizer::GetArrivalTimes(std::vector<double>& vX)
{
  std::vector<double> arrivalTimes;
  arrivalTimes.resize(size, 0);
  Maths::Regression::Linear* regressionR = NULL;
  for (HTimingPointWrapper tp = design[design.TimingPoints.TopologicalOrderRoot()].GoNext(); !::IsNull(tp); 
    tp.GoNext())
  {
    if(::IsNull(tp)) continue; 
    HPinWrapper pin = design[(design, tp).Pin()];
    double aT;
    if (pin.IsPrimaryInput()) 
      aT = 0;  
    else
    {
      if (pin.Direction() == PinDirection_INPUT || pin.IsPrimaryOutput())
         aT=GetInputTimingPointAT(vX, tp, arrivalTimes);
      else
        if (pin.Direction() == PinDirection_OUTPUT)
          aT=GetOutputTimingPointAT(vX, tp, arrivalTimes);
    }
    arrivalTimes[::ToID(tp)] = aT;
    ALERT("id: %d, arrivalTime: %lf", ::ToID(tp), aT);
  }
  return arrivalTimes;
}

double LRSizer::CalcDelay(HTimingPointWrapper& tp, std::vector<double>& vX)
{
  /*TODO: Rewrite method according to the fact that R belongs to ARC  
  Maths::Regression::Linear* regressionR = GetRegressionR(tp);
  HPinWrapper pin = design[(design, tp).Pin()];
  HCell cell = pin.Cell();
  int index = (int)distance(cells->begin(), find(cells->begin(), cells->end(), cell));
  return (regressionR->getValue(1 / vX[index]) * GetObservedC(tp, vX));  
  */
  return 1;
}

void LRSizer::AdjustLambda( int step, std::vector<double>& arrivalTimes, std::vector<double>& vX)
{
  HTimingPointWrapper tp = design[design.TimingPoints.TopologicalOrderRoot()];
  for (tp.GoPrevious(); tp != design.TimingPoints.LastInternalPoint(); tp.GoPrevious())
  {
    HNet net = design.Get<HPin::Net, HNet>(design.Get<HTimingPoint::Pin, HPin>(tp));
    HPin source = design[net].Source();
    HTimingPointWrapper tpSrc = design[design.TimingPoints[source]];
    GetPointLambdas(tp)[0] = GetPointLambdas(tp)[0] + (1 / step) * 
      (arrivalTimes[::ToID(tpSrc)]/*-A_0*/);// ?? EndPoint always has the only input point ??
  }
  HTimingPointWrapper tmpTp = design[design.TimingPoints.FirstInternalPoint()].GoPrevious();
  for (tp.GoPrevious(); tp != tmpTp; tp.GoPrevious())
  {
    if (((design.Get<HTimingPoint::Pin, HPin>(tp)), design).Direction() == PinDirection_INPUT)
    {
      HNet net = design.Get<HPin::Net, HNet>(design.Get<HTimingPoint::Pin, HPin>(tp));
      HPin source = design[net].Source();
      HTimingPointWrapper tpSrc = design[design.TimingPoints[source]];
      GetPointLambdas(tp)[0] = GetPointLambdas(tp)[0] + (1 / step) * 
        (arrivalTimes[::ToID(tpSrc)] + CalcDelay(tp, vX) - arrivalTimes[::ToID(tp)]);
    }
    int i=0;
    if (((design.Get<HTimingPoint::Pin, HPin>(tp)), design).Direction() == PinDirection_OUTPUT)
    {
      HPin outputPin = design.Get<HTimingPoint::Pin, HPin>(tp);
      HCell cell = (design,outputPin).Cell();
      for (HCell::PinsEnumeratorW inputPin = (design, cell).GetPinsEnumeratorW(); inputPin.MoveNext(); )
      {
        HTimingPointWrapper tpInput = design[design.TimingPoints[inputPin]];
        if (inputPin.Direction() != PinDirection_INPUT || ::IsNull(inputPin.Net())) 
          continue;
        for (HPinType::ArcsEnumeratorW arc = (design,(design,outputPin).Type()).GetArcsEnumeratorW(); 
          arc.MoveNext();)
        {
          if (arc.TimingType() != TimingType_Combinational) 
            continue;
          if (arc.GetStartPin(inputPin) == outputPin)
          {
            GetPointLambdas(tp)[i] = GetPointLambdas(tp)[i] + (1 / step) * 
              (arrivalTimes[::ToID(tpInput)] + CalcDelay(tp, vX) - arrivalTimes[::ToID(tp)]);
            i++;
          }
        }
      }   
    }
  }
}

void LRSizer::ProjectLambdaMatrix()
{
  double MuIn, MuOut; 
  std::vector<double> percentage;
  HTimingPointWrapper tp = design[design.TimingPoints.TopologicalOrderRoot()];
  for (tp.GoPrevious(); !::IsNull(tp); tp.GoPrevious())
  {
    percentage.reserve(GetPointLambdas(tp).size());
    MuIn = FindInputLambdaSum(tp);
    MuOut = FindOutputLambdaSum(tp);
    for (std::vector<double>::iterator lambdaIter = GetPointLambdas(tp).begin();
      lambdaIter != GetPointLambdas(tp).end(); lambdaIter++)
      percentage.push_back(*lambdaIter / MuIn);
    for(unsigned int i = 0;i < GetPointLambdas(tp).size(); i++)
    {
      GetPointLambdas(tp)[i] = percentage[i] * MuOut;
    }
  }
}

double LRSizer::CalculateQ(unsigned int size)
{
  double SumInputLambdaForPoint; 
  double SumAllInputLambda = 0;
  double QFunction = 0;
  /*TODO: Rewrite method according to the fact that R belongs to ARC
  std::vector<double> XLower;
  XLower.reserve(size);
  InitVxByLowerBound(XLower);
  HTimingPointWrapper tp = design[design.TimingPoints.TopologicalOrderRoot()];
  for (; tp != design.TimingPoints.FirstInternalPoint(); tp.GoPrevious())
  {
    SumInputLambdaForPoint=FindInputLambdaSum(tp);
    HPinWrapper pin = design[(design, tp).Pin()];
    HCell cell = pin.Cell();
    if (::IsNull(cell)) continue;
    Maths::Regression::Linear* regressionR = GetRegressionR(tp);
    int index = (int)distance(cells->begin(), find(cells->begin(), cells->end(), cell));
    SumAllInputLambda += SumInputLambdaForPoint*(regressionR->getValue(1 / XLower[index]) * 
      GetObservedC(tp, XLower));
  }
  double SumMinX=0;
  for(unsigned int i = 0; i < XLower.size(); i++)
    SumMinX += XLower[i];
  QFunction = SumMinX + SumAllInputLambda;
  */
  return QFunction;
}

bool LRSizer::CheckStopConditionForLDP(std::vector<double>& vX, double errorBound)
{
  double Sum = 0;
  for(unsigned int i = 0; i < vX.size(); i++)
    Sum += vX[i];
  if ((Sum - CalculateQ((int)vX.size())) < errorBound) 
    return true;
  else return false;
} 

double LRSizer::CalcCriterion(std::vector<double>& vCurrentX)
{
  double criterion = 0, Sum = 0, SumInputLambdaForPoint = 0, SumAllInputLambda = 0;
  /*TODO: Rewrite method according to the fact that R belongs to ARC
  for(unsigned int i = 0; i < vCurrentX.size(); i++)
    Sum+=vCurrentX[i];
  HTimingPointWrapper tp = design[design.TimingPoints.TopologicalOrderRoot()];
  for (; tp != design.TimingPoints.FirstInternalPoint(); tp.GoPrevious())
  {
    SumInputLambdaForPoint = FindInputLambdaSum(tp);
    HPinWrapper pin = design[(design, tp).Pin()];
    HCell cell = pin.Cell();
    if (::IsNull(cell)) continue;
    Maths::Regression::Linear* regressionR = GetRegressionR(tp);
    int index = (int)distance(cells->begin(), find(cells->begin(), cells->end(), cell));
    SumAllInputLambda += SumInputLambdaForPoint * (regressionR->getValue(1 / vCurrentX[index]) *
      GetObservedC(tp, vCurrentX));
  }
  criterion = Sum + SumAllInputLambda;
  */
  return criterion;
}

Maths::Regression::Linear* LRSizer::GetRegressionC( HTimingPointWrapper tp )
{
  HPin pin = tp.Pin();
  std::vector<double> C, X;
  GetPinFamilyC(pin, C, X);
  int numOfAlternatives = (int)C.size();
  double* arrC = new double[numOfAlternatives];
  copy(C.begin(), C.end(), arrC);
  double* arrX = new double[numOfAlternatives];
  copy(X.begin(), X.end(), arrX);
  ASSERT(C.size() > 0);
  return new Maths::Regression::Linear((int)C.size(), arrX, arrC);
}

Maths::Regression::Linear* LRSizer::GetRegressionR(HPin inputPin, HPin outputPin)
{
  std::vector<double> R, invX;
  GetArcFamilyR(inputPin, outputPin, R, invX);
  int numOfAlternatives = (int)R.size();
  double* arrR = new double[numOfAlternatives];
  copy(R.begin(), R.end(), arrR);
  double* arrInvX = new double[numOfAlternatives];
  copy(invX.begin(), invX.end(), arrInvX);
  ASSERT(R.size() > 0);
  return new Maths::Regression::Linear((int)R.size(), arrInvX, arrR);
}

HMacroType LRSizer::RoundCellToTypeFromLib(HCell cellFrom, double currentX)
{
  std::vector<HMacroType> macroTypesInFamily;
  GetCellFamily(cellFrom, macroTypesInFamily);
  double smallestDeltaX = std::numeric_limits<double>::max();
  HMacroType bestMacroType;
  for(std::vector<HMacroType>::iterator macroType = macroTypesInFamily.begin(); 
    macroType != macroTypesInFamily.end(); macroType++)
  {
    double sizeOfMacroType = GetMacroTypeSize(*macroType);
    double deltaX = fabs(currentX - sizeOfMacroType);
    if (deltaX < smallestDeltaX)
    {
      smallestDeltaX = deltaX;
      bestMacroType = *macroType;
    }
  }
  return bestMacroType;
}

double LRSizer::GetMacroTypeSize(HMacroType macroType)
{
  std::string macroTypeName = design[macroType].Name();
  int i = (int)macroTypeName.length() - 1;
  while(i >= 0 && macroTypeName[i] != 'x' && macroTypeName[i] != 'X') i--;
  ASSERT(i > 0);
  std::string macroTypeSizeString = macroTypeName.substr(i + 1);
  return atof(macroTypeSizeString.c_str());
}

void LRSizer::DoLRSizing()
{

#ifdef MODE_TEST   
  ALERT("MODE TEST defined");
  std::vector<double> vX;
  InitVxByLowerBound(vX);
  std::vector<double> arrivalTimes = GetArrivalTimes(vX);
  return;
#endif

  ALERT("MODE TEST not defined");
  std::vector<double> newVX;
  SolveLDP();
  SolveLrsMu(newVX);
  ApplySizing(newVX);
}