#include <math.h>
#include <float.h>
#include "HippocratePlacement.h"
#include "Legalization.h"
#include "Parser.h"
#include <time.h>
#include "Timing.h"

bool CheckHippocrateConstraint(HDesign& design, HNet net, HPin sink)
{
  ASSERT(((sink,design).Net() == net));
  ASSERT(((sink,design).Direction() == PinDirection_INPUT));

  HPin source = (net,design).Source();

  double driverResistance = Utils::GetDriverTimingPhisics(design, source, SignalDirection_None).R;
  double hpwl = Utils::CalcNetHPWL(design, net);
  double l = abs((source,design).X() - (sink,design).X())
    + abs((source,design).Y() - (sink,design).Y());
  double Cs = Utils::GetSinkCapacitance(design, sink, SignalDirection_None);

  double sourceArrival = (design.TimingPoints[source],design).ArrivalTime();
  double sourceOldC = (design.SteinerPoints[source],design).ObservedC();

  double sinkArrival = DBL_MIN;

  for (HCell::PinsEnumeratorW pin = (design[sink].Cell(),design).GetPinsEnumeratorW();
    pin.MoveNext(); )
  {
    if (!::IsNull(pin.Cell()) && pin.Direction() == PinDirection_INPUT)
    {
      double AT = (design.TimingPoints[pin],design).ArrivalTime();
      if (AT > sinkArrival)
        sinkArrival = AT;
    }
  }

  double load = Utils::GetNetLoad(design, net, SignalDirection_None);

  return sourceArrival
    + driverResistance * (load + design.RoutingLayers.Physics.LinearC * hpwl - sourceOldC)
    + design.RoutingLayers.Physics.RPerDist * l * (Cs + 0.5 * l * design.RoutingLayers.Physics.LinearC)
    <= sinkArrival;
}

bool CheckHippocrateConstraint(HDesign& design, HCell cell)
{
  for (HCell::PinsEnumeratorW pin = (cell,design).GetPinsEnumeratorW(); pin.MoveNext(); )
    if (!::IsNull(pin))
    {
      HNetWrapper net = design[pin.Net()];
      for (HNet::SinksEnumeratorW sink = net.GetSinksEnumeratorW(); sink.MoveNext(); )
        if (!CheckHippocrateConstraint(design, net, sink))
          return false;
    }
  return true;
}

void HippocratePlacementInit()
{
	printf("\nH I P P O C R A T E   I N I T\n");
}

void HippocratePlacementFinalize()
{
	printf("\nH I P P O C R A T E  D E L E T E\n");
}

void HippocratePlacementMOVE(HDPGrid& hdpp, HDesign& hd, HCell& curCell, std::vector<HCell>& currentWnd, StatisticsAnalyser& stat)
{

	Utils::CalculateHPWL(hd, true);
	std::vector<HNet> curNets;//соединения, в которых участвует текущий элемент
	for(HCell::PinsEnumeratorW pin=hd.Get<HCell::Pins, HCell::PinsEnumeratorW>(curCell); pin.MoveNext(); )
	{
		HNet net=hd.Get<HPin::Net,HNet>(pin);
		if(!::IsNull(net)) curNets.push_back(net);
	}
	double maxIncrOfWTWL = 0;  //максимальное уменьшение WTWL
	int k = 0;  //номер эл-та из окна, для которого достиг. максимальное уменьшение WTWL
	int PosInGap=0;
	int oldCurRow = hdpp.CellRow(curCell);
	int oldCurCol = hdpp.CellColumn(curCell);
	std::vector<PseudoCell> VHole=getPseudoCellsFromWindow(hdpp, hd, currentWnd);

	for (unsigned int i = 0; i < VHole.size(); i++)
	{
		if(hdpp.CellSitesNum(curCell) <= VHole[i].GapSize) //меняем два элемента местами
		{
			for(int counter=0;counter<=(VHole[i].GapSize-hdpp.CellSitesNum(curCell));counter++)
			{
				hdpp.PutCell(curCell, VHole[i].row,VHole[i].col+counter);

				//для каждого изменённого соединения проверяем выполнение ограничений
				bool constraintsOK = true;
 				double deltaWTWL = 0;

				//for(int j = 0; j < curNets.size(); j++) {
					//if(!ControlOfConstraints(hdpp, hd, curNets[j], oldCurRow, oldCurCol, curCell))
						//constraintsOK = false;

					if(!CheckHippocrateConstraint(hd, curCell))
					{constraintsOK = false;}

					if(constraintsOK) { //ALERT("constraintsOK");//если выполнены ограничения
					//находим изменение WTWL (deltaWTWL)
					deltaWTWL = CalculateDiffWTWL(hd, curNets, false);
					if (deltaWTWL < maxIncrOfWTWL){
						maxIncrOfWTWL = deltaWTWL;
						k = i;
						PosInGap=counter;
					}
				}

				hdpp.PutCell(curCell, oldCurRow, oldCurCol);
				stat.incrementAttempts(MOVE);

			}
		}
	}
	if (maxIncrOfWTWL < 0) { //если длина проводов уменьшилась
		//меняем эл-ты
		hd.Plotter->SaveMilestoneImage("MOVE_B");
		hd.Plotter->PlotCell(curCell, Color_Brown);
		hd.Plotter->Refresh(HPlotter::WAIT_3_SECONDS);

		hdpp.PutCell(curCell, VHole[k].row,VHole[k].col+PosInGap);

		//ALERT("!!!k=%d pos=%d",k,PosInGap);
		hd.Plotter->PlotCell(curCell, Color_Brown);
		hd.Plotter->Refresh(HPlotter::WAIT_3_SECONDS);
		hd.Plotter->SaveMilestoneImage("MOVE_A");
		stat.incrementAttemptsSuccessful(MOVE);
		for(int j = 0; j < curNets.size(); j++)
		{
			HPinWrapper source = hd[hd.Get<HNet::Source, HPin>(curNets[j])];
			//if (source.Cell() == curCell) {ALERT("IS SOURCE!"); }
		}
	}

}

void HippocratePlacementCOMPACT(HDPGrid& hdpp, HDesign& hd, HCell& curCell, BBox curBBox, StatisticsAnalyser& stat)
{
	double maxIncrOfWTWL = 0;  //максимальное уменьшение WTWL
	int k = 0;  //номер эл-та из окна, для которого достиг. максимальное уменьшение WTWL
	int PosInGap=0;
	int oldCurRow;
	int oldCurCol;
	std::vector<HCell> gotSpecialWindow;//
	std::vector<PseudoCell> VHole;

	for(std::vector<HCell>::iterator IterCellInNet=curBBox.cells.begin();IterCellInNet!=curBBox.cells.end();IterCellInNet++)
	{
		Utils::CalculateHPWL(hd, true);
		std::vector<HNet> curNets;//соединения, в которых участвует текущий элемент
		for(HCell::PinsEnumeratorW pin=hd.Get<HCell::Pins, HCell::PinsEnumeratorW>(*IterCellInNet); pin.MoveNext(); )
		{
			HNet net=hd.Get<HPin::Net,HNet>(pin);
			if(!::IsNull(net)) curNets.push_back(net);
		}
		maxIncrOfWTWL = 0;  //максимальное уменьшение WTWL
		k = 0;  //номер эл-та из окна, для которого достиг. максимальное уменьшение WTWL
		PosInGap=0;
		oldCurRow = hdpp.CellRow(*IterCellInNet);
		oldCurCol = hdpp.CellColumn(*IterCellInNet);
		//gotSpecialWindow=getSpecialWindow(hdpp, hd, *IterCellInNet, curBBox);
		VHole.clear();
		//VHole = getPseudoCellsFromSpecWindow(hdpp, hd, gotSpecialWindow);
		VHole=getGapsForCell(hdpp,hd,*IterCellInNet,curBBox,hd.cfg.ValueOf("HippocratePlacement.FindOnlyInsideGaps"));
		//unsigned int i = 0; if (VHole.size()<=0) continue;
		for(unsigned int i = 0; i < VHole.size(); i++)
		{
			if(hdpp.CellSitesNum(*IterCellInNet) <= VHole[i].GapSize) //меняем два элемента местами
			{
				for(int counter=0;counter<=(VHole[i].GapSize-hdpp.CellSitesNum(*IterCellInNet));counter++)
				{
					hd.Plotter->PlotCell(*IterCellInNet, Color_GrayText);
					hdpp.PutCell(*IterCellInNet, VHole[i].row,VHole[i].col+counter);
					hd.Plotter->PlotCell(*IterCellInNet, Color_Black);

					//для каждого изменённого соединения проверяем выполнение ограничений
					bool constraintsOK = true;
					double deltaWTWL = 0;
					/*for(int j = 0; j < curNets.count; j++) {
						if(!ControlOfConstraints(hdpp, hd, curNets.net[j], oldCurRow, oldCurCol, *IterCellInNet))
							constraintsOK = false;
					}*/
					if(!CheckHippocrateConstraint(hd, *IterCellInNet)) constraintsOK = false;

					if(constraintsOK) { //если выполнены ограничения
						//находим изменение WTWL (deltaWTWL)
						deltaWTWL = CalculateDiffWTWL(hd, curNets, false);
						if (deltaWTWL < maxIncrOfWTWL){
							maxIncrOfWTWL = deltaWTWL;
							k = i;
							PosInGap=counter;
						}
					}

					hdpp.PutCell(*IterCellInNet, oldCurRow, oldCurCol);
					stat.incrementAttempts(COMPACT);
				}
			}
		}
		if (maxIncrOfWTWL < 0) { //если длина проводов уменьшилась
			//меняем эл-ты
			//hd.Plotter->SaveMilestoneImage("CompactSuccess1");
			hd.Plotter->PlotCell(*IterCellInNet, Color_Brown);
			hd.Plotter->Refresh(HPlotter::WAIT_3_SECONDS);

			hdpp.PutCell(*IterCellInNet, VHole[k].row,VHole[k].col+PosInGap);

			//ALERT("!!!k=%d pos=%d",k,PosInGap);
			hd.Plotter->PlotCell(*IterCellInNet, Color_Red);
			hd.Plotter->Refresh(HPlotter::WAIT_3_SECONDS);
			hd.Plotter->SaveMilestoneImage("Compact_success2");
			stat.incrementAttemptsSuccessful(COMPACT);
		hd.Plotter->ShowPlacement();
		}
	}
}

PossiblePos GetCenterNearestPos(HDPGrid& hdpp, HCell& NetCell, std::vector<PseudoCell>& VGap, BBox& curBBox)
{
	int targetRow=curBBox.rowCenter, targetCol=curBBox.colCenter;
	PossiblePos CentPos;
	CentPos.col=-1;
	CentPos.row=-1;
	int dist = -1;
	for(unsigned int i = 0; i < VGap.size(); i++)
	{
		if(hdpp.CellSitesNum(NetCell) <= VGap[i].GapSize)
		{
			for(int j=0; j<VGap[i].GapSize-hdpp.CellSitesNum(NetCell)+1;j++)
			{
				int temp = abs(VGap[i].col+j-targetCol)+abs(VGap[i].row-targetRow);
				if ((dist>temp)||(dist == -1))
				{
					dist=temp;
					CentPos.col=VGap[i].col+j;
					CentPos.row=VGap[i].row;
				}
			}
		}
	}
	return CentPos;
}

int GetLeftFreeSpace(HCell cell, HDPGrid& hdpp) {
	int m = 1;
	int row = hdpp.CellRow(cell);
	int col = hdpp.CellColumn(cell);
	if (col >= 1) {

		//while ((col - m >= 0)&&(::IsNull(hdpp(row, col - m)))) {
		while((col - m >= 0)&&!isCellExists(hdpp, row, col-m)){
			m++;
		}
	}
	return m-1;
}

int GetRightFreeSpace(HCell cell, HDPGrid& hdpp) {
	int row = hdpp.CellRow(cell);
	int col = hdpp.CellColumn(cell);
	int nSites = hdpp.CellSitesNum(cell);
	int m = 0;
	if (col + nSites < hdpp.NumCols()) {
		//while((col + nSites + m < hdpp.NumCols())&&::IsNull(hdpp(row, col + nSites + m))) {
		while(col + nSites + m < hdpp.NumCols() && !isCellExists(hdpp, row, col + nSites + m)){
				m++;
		}
	}
	return m;
}

void HippocratePlacementCENTER(HDPGrid& hdpp, HDesign& hd, HCell& curCell, std::vector<HCell>& currentWnd, StatisticsAnalyser& stat)
{
	Utils::CalculateHPWL(hd, true);
	std::vector<HNet> curNets;//соединения, в которых участвует текущий элемент
	for(HCell::PinsEnumeratorW pin=hd.Get<HCell::Pins, HCell::PinsEnumeratorW>(curCell); pin.MoveNext(); )
	{
		HNet net=hd.Get<HPin::Net,HNet>(pin);
		if(!::IsNull(net)) curNets.push_back(net);
	}

	double maxIncrOfWTWL = 0;  //максимальное уменьшение WTWL
	int k = 0;
	int oldCurRow = hdpp.CellRow(curCell);
	int oldCurCol = hdpp.CellColumn(curCell);

	hd.Plotter->PlotCell(curCell, Color_Black);
	hd.Plotter->Refresh(HPlotter::WAIT_3_SECONDS);

	std::vector<HCell> ConnectedCells; //эл-ты, соединённые с текущим
	int connectedCellsCount = 0;   //к-во эл-тов, соединённых с текущим

	ConnectedCells = GetCellsConnectedWithCurrent(hdpp, hd, curCell);
	connectedCellsCount = ConnectedCells.size();

	std::vector<HPinWrapper> primaryPins;
	for(int i=0; i<curNets.size(); i++) {
		for (HNet::PinsEnumeratorW pin = hd[curNets[i]].GetPinsEnumeratorW(); pin.MoveNext(); ) {
			if(pin.IsPrimary()) {
				primaryPins.push_back(pin);
			}
		}
	}
	//определяем огранич. прямоугольник
	double minX, minY, maxX, maxY;
	minX = maxX = hd.GetDouble<HCell::X>(ConnectedCells[0]);
	minY = maxY = hd.GetDouble<HCell::Y>(ConnectedCells[0]);

	for(int i=0; i<connectedCellsCount; i++) {
		if(hd.GetDouble<HCell::X>(ConnectedCells[i]) > maxX)
			maxX = hd.GetDouble<HCell::X>(ConnectedCells[i]);
		if(hd.GetDouble<HCell::X>(ConnectedCells[i]) < minX)
			minX = hd.GetDouble<HCell::X>(ConnectedCells[i]);

		if(hd.GetDouble<HCell::Y>(ConnectedCells[i]) > maxY)
			maxY = hd.GetDouble<HCell::Y>(ConnectedCells[i]);
		if(hd.GetDouble<HCell::Y>(ConnectedCells[i]) < minY)
			minY = hd.GetDouble<HCell::Y>(ConnectedCells[i]);
	}
	if(primaryPins.size()) {
		for(unsigned int i=0; i<primaryPins.size(); i++) {
			if(maxX < (primaryPins[i].X()))
				maxX = (primaryPins[i].X());
			if(maxY < (primaryPins[i].Y()))
				maxY = (primaryPins[i].Y());
			if(minX > (primaryPins[i].X()))
				minX = (primaryPins[i].X());
			if(minY > (primaryPins[i].Y()))
				minY = (primaryPins[i].Y());
		}
	}
	double centerX = (maxX + minX)/2;
	double centerY = (maxY + minY)/2;
	int centerCol = hdpp.FindColumn(centerX);
	int centerRow = hdpp.FindRow(centerY);

	hd.Plotter->DrawCircle(centerX,centerY,3,Color_Yellow);
	hd.Plotter->Refresh(HPlotter::WAIT_3_SECONDS);

	bool putCurCell = false;
	int left = 0, right = 0; //к-во свободных сайтов слева и справа от центра
	//находим центральный элемент
	//if(!::IsNull(hdpp(centerRow, centerCol))) { //если в центре есть элемент
	if(isCellExists(hdpp, centerRow, centerCol)){ //если в центре есть элемент
		curCell = hdpp(centerRow, centerCol);
	}
	else { //если в центре нет элемента
		if (centerCol > 0) {
			//while((centerCol - left - 1 >= 0)&&(::IsNull(hdpp(centerRow, centerCol - left - 1))))
			while((centerCol - left - 1 >= 0) && !isCellExists(hdpp, centerRow, centerCol - left - 1))
				left++;
			/////
			//TODO: WHY without "!" ???
			/////

		}
		if (centerCol < hdpp.NumCols()) {
			//while ((centerCol + right < hdpp.NumCols())&&(::IsNull(hdpp(centerRow, centerCol + right))))
			while((centerCol + right < hdpp.NumCols())&&!isCellExists(hdpp, centerRow, centerCol + right))
				right++;
				/////
				//TODO: WHY without "!" ???
				/////


		}
		if (left + right < hdpp.CellSitesNum(curCell)) { //если нет места
			if(left <= right)
				curCell = hdpp(centerRow, centerCol - left - 1);
			else
				curCell = hdpp(centerRow, centerCol + right);
		}
		else if(left + right == hdpp.CellSitesNum(curCell)  ) { //если есть место
			hdpp.PutCell(curCell, centerRow, centerCol - left);
			putCurCell = true;
		}
		else {
			if(right <= hdpp.CellSitesNum(curCell))
				hdpp.PutCell(curCell, centerRow, centerCol + right - hdpp.CellSitesNum(curCell));
			else if(left <= hdpp.CellSitesNum(curCell))
				hdpp.PutCell(curCell, centerRow, centerCol - left);
			else
				hdpp.PutCell(curCell, centerRow, centerCol);
			putCurCell = true;
		}
	}
	hd.Plotter->PlotCell(curCell, Color_Green);
	hd.Plotter->Refresh(HPlotter::WAIT_3_SECONDS);

	currentWnd = GetCurrentWindow(hdpp, RADIUS_OF_WINDOW, curCell);

	std::vector<PseudoCell> VHole;
	VHole = getPseudoCellsFromWindow(hdpp, hd, currentWnd);
	PseudoCell cntHole;

	std::vector<PseudoCell> VHoles;
	int numHoles = 0;
	int PosInGap = 0;

	if(!putCurCell){ //если не перекладывали текущий элемент
		curCell = hdpp(oldCurRow, oldCurCol);
	}
	else { //кладём текущий обратно
		hdpp.PutCell(curCell, oldCurRow, oldCurCol);
		cntHole.row = centerRow;
		cntHole.col = centerCol - left;
		cntHole.GapSize = left + right;
	}
	for(unsigned int i = 0; i < VHole.size(); i++) {
		VHoles.push_back(VHole[i]);
	}
	if(putCurCell){
		VHoles.push_back(cntHole);
	}
	numHoles = VHoles.size();

	hd.Plotter->PlotCell(curCell, Color_Red);
	hd.Plotter->Refresh(HPlotter::WAIT_3_SECONDS);
	stat.incrementAttempts(CENTER);

	//ищем для текущего лучшее место в центральном окне
	for(int i = 0; i < numHoles; i++) {
		if(hdpp.CellSitesNum(curCell) <= VHoles[i].GapSize) {
			for(int counter=0;counter<=(VHoles[i].GapSize-hdpp.CellSitesNum(curCell));counter++)	{
				hdpp.PutCell(curCell, VHoles[i].row,VHoles[i].col+counter);

				//для каждого изменённого соединения проверяем выполнение ограничений
				bool constraintsOK = true;
				double deltaWTWL = 0;

				if(!CheckHippocrateConstraint(hd, curCell)) constraintsOK = false;
				/*for(int j = 0; j < curNets.count; j++) {
					if(!ControlOfConstraints(hdpp, hd, curNets.net[j], oldCurRow, oldCurCol, curCell))
						constraintsOK = false;
				}*/
				if(constraintsOK) { //если выполнены ограничения
					//находим изменение WTWL (deltaWTWL)
					deltaWTWL = CalculateDiffWTWL(hd, curNets, false);
					if (deltaWTWL < maxIncrOfWTWL){
						maxIncrOfWTWL = deltaWTWL;
						k = i;
						PosInGap=counter;
					}
				}
				hdpp.PutCell(curCell, oldCurRow, oldCurCol);
			}
		}
	}
	if (maxIncrOfWTWL < 0) { //если длина проводов уменьшилась
		hdpp.PutCell(curCell, VHoles[k].row,VHoles[k].col+PosInGap);
		stat.incrementAttemptsSuccessful(CENTER);
	}
	hd.Plotter->Clear();
	hd.Plotter->PlotPlacement();
	hd.Plotter->Refresh(HPlotter::WAIT_1_SECOND);
}


void HippocratePlacementSWAP(HDPGrid& hdpp, HDesign& hd, HCell& curCell, std::vector<HCell>& currentWnd, StatisticsAnalyser& stat)
{
	if ((curCell,hd).IsSequential()) return;
	Utils::CalculateHPWL(hd, true);

	std::vector<HNet> curNets;//соединения, в которых участвует текущий элемент
	for(HCell::PinsEnumeratorW pin=hd.Get<HCell::Pins, HCell::PinsEnumeratorW>(curCell); pin.MoveNext(); )
	{
		HNet net=hd.Get<HPin::Net,HNet>(pin);
		if(!::IsNull(net)) curNets.push_back(net);
	}
	double maxIncrOfWTWL = 0;  //максимальное уменьшение WTWL
	int BestCurCol=-1;
	int BestCurRow=-1;
	int BestIterCol=-1;
	int BestIterRow=-1;
	bool BestFound=false;
	HCell BestSwappingCell;
	HCell BestCurCell;

	int oldCurRow = hdpp.CellRow(curCell);
	int oldCurCol = hdpp.CellColumn(curCell);

	for(std::vector<HCell>::iterator iterSwappingCells=currentWnd.begin();iterSwappingCells!=currentWnd.end();iterSwappingCells++)
	{
		if ((*iterSwappingCells,hd).IsSequential()) continue;
		std::vector<HNet> Nets; //соединения, в которых участвует элемент из окна
		for(HCell::PinsEnumeratorW pin=hd.Get<HCell::Pins, HCell::PinsEnumeratorW>(*iterSwappingCells); pin.MoveNext(); )
		{
			HNet net=hd.Get<HPin::Net,HNet>(pin);
			if(!::IsNull(net)) Nets.push_back(net);
		}

		HNet dubNet; //нет, в котором участвуют текущий эл-т и эл-т из окна (если есть)
		for(int m=0; m<Nets.size(); m++)
			for(int j=0; j<curNets.size(); j++)
				if(curNets[j]==Nets[m]&&(!::IsNull(curNets[j]))) {
					dubNet = curNets[j];
				}

				int oldRow = hdpp.CellRow(*iterSwappingCells);
				int oldCol = hdpp.CellColumn(*iterSwappingCells);

				int row = oldCurRow, //текущий эл-т меньше
					col = oldCurCol;
				int minNS = hdpp.CellSitesNum(curCell);
				int maxNS = hdpp.CellSitesNum(*iterSwappingCells);
				if( minNS > maxNS ) {
					minNS = hdpp.CellSitesNum(*iterSwappingCells);
					maxNS = hdpp.CellSitesNum(curCell);
					row = oldRow; //эл-т из окна меньше
					col = oldCol;
				}
				//ищем свободное место слева и справа от меньшего эл-та
				int LeftFreeSpace = GetLeftFreeSpace(hdpp(row, col), hdpp);
				int RightFreeSpace = GetRightFreeSpace(hdpp(row, col), hdpp);
				if( (LeftFreeSpace + RightFreeSpace + minNS) >= maxNS ) //меняем два элемента местами
				{
					int delta1 = 0;
					int delta2 = 0;
					if( (RightFreeSpace + minNS) < maxNS ) { //меняем со сдвигом влево
						if(curCell==hdpp(row, col)) { //текущий меньше
							delta1 = maxNS - RightFreeSpace - minNS;
						}
						else { //текущий больше
							delta2 = maxNS - RightFreeSpace - minNS;
						}
					}

					HCell swappingCell=*iterSwappingCells;
					hdpp.PutCell(curCell, oldRow, oldCol-delta2);
					hdpp.PutCell(swappingCell, oldCurRow, oldCurCol-delta1);

					double deltaWTWL = CalculateDiffWTWL(hd, curNets, false) + CalculateDiffWTWL(hd, Nets, false) - GetNetWeight(hd, dubNet)*Utils::CalculateHPWLDiff(hd, &dubNet, 1, false);
					if(CheckHippocrateConstraint(hd, curCell)&&CheckHippocrateConstraint(hd, swappingCell)&&(deltaWTWL < maxIncrOfWTWL)){

						maxIncrOfWTWL = deltaWTWL;
						BestSwappingCell=swappingCell;
						BestCurCell=curCell;
						BestCurRow=oldRow;
						BestCurCol=oldCol-delta2;
						BestIterRow=oldCurRow;
						BestIterCol=oldCurCol-delta1;
						BestFound=true;
					}
					//меняем обратно
					hdpp.PutCell(swappingCell, oldRow, oldCol);
					hdpp.PutCell(curCell, oldCurRow, oldCurCol);

					stat.incrementAttempts(SWAP);
				}
	}

	if ((maxIncrOfWTWL < 0)&&(BestFound)) { //если длина проводов уменьшилась
		//if(BestIterRow>=0&&BestIterCol>=0&&BestCurCol>=0&&BestCurRow>=0){
		hdpp.PutCell(BestCurCell, BestCurRow,BestCurCol);
		hdpp.PutCell(BestSwappingCell, BestIterRow, BestIterCol);

		stat.incrementAttemptsSuccessful(SWAP);

		HippocratePlacementLOCALMOVE(hdpp, hd, BestCurCell, stat);
		HippocratePlacementLOCALMOVE(hdpp, hd, BestSwappingCell, stat);
		//}
	}
}

void HippocratePlacementLOCALMOVE(HDPGrid& hdpp, HDesign& hd, HCell& curCell, StatisticsAnalyser& stat)
{
  int basecol = hdpp.CellColumn(curCell);
  int baserow = hdpp.CellRow(curCell);
  double hpwl_diff = Utils::CalculateHPWLDiff(hd, &curCell, 1, false);//= 0.0;//but I don't trust input.
  int bestcol = basecol;

  //go left
	for (int lcol = basecol - 1; lcol >= 0 && !isCellExists(hdpp,baserow, lcol); --lcol)
  //for (int lcol = basecol - 1; lcol >= 0 && ::IsNull(hdpp(baserow, lcol)); --lcol)
  {
    hdpp.PutCellIntoSiteFast(curCell, lcol);
    double diff = Utils::CalculateHPWLDiff(hd, &curCell, 1, false);
    if (diff < hpwl_diff)
      if (CheckHippocrateConstraint(hd, curCell))
      {
        bestcol = lcol;
        hpwl_diff = diff;
      }
  }

  //go right
  int len = hdpp.CellSitesNum(curCell);
  int maxPos = hdpp.NumCols() - len;
  //for (int rcol = basecol + 1; rcol < maxPos && ::IsNull(hdpp(baserow, rcol + len - 1)); ++rcol)
	/////
	//TODO: WHY without "!" ???
	/////
	for (int rcol = basecol + 1; rcol < maxPos && !isCellExists(hdpp,baserow, rcol + len - 1); ++rcol)
  {
    hdpp.PutCellIntoSiteFast(curCell, rcol);
    double diff = Utils::CalculateHPWLDiff(hd, &curCell, 1, false);
    if (diff < hpwl_diff)
      if (CheckHippocrateConstraint(hd, curCell))
      {
        bestcol = rcol;
        hpwl_diff = diff;
      }
  }

  //choose best position
  if (bestcol == basecol)
  {
    hdpp.PutCellIntoSiteFast(curCell, basecol);
    stat.incrementAttempts(LOCALMOVE);
  }
  else
  {
    hdpp.PutCellIntoSite(curCell, bestcol);
    Utils::CalculateHPWLDiff(hd, &curCell, 1, true);
    stat.incrementAttemptsSuccessful(LOCALMOVE);
  }

  return;
}

double CalculateDeltaArrivalTime(HDesign& hd, HPinWrapper sink)
{
	double deltaArrivalTime; //измен-е AT для sink
	HCell cell = sink.Cell();
	double cellAT; //AT для элемента
	double sinkAT; //AT для sink
	HTimingPointWrapper tpsink = hd[hd.TimingPoints[sink]];
	sinkAT = tpsink.ArrivalTime();
	cellAT = sinkAT;
	//обходим входные пины элемента
	for (HCell::PinsEnumeratorW pin = hd.Get<HCell::Pins, HCell::PinsEnumeratorW>(cell); pin.MoveNext(); ) {
		if (pin.Direction() == PinDirection_INPUT) {
			HTimingPointWrapper tp = hd[hd.TimingPoints[pin]];
			if(cellAT < tp.ArrivalTime()) {
				cellAT = tp.ArrivalTime();
			}
		}
	}
	deltaArrivalTime = cellAT - sinkAT;
	return deltaArrivalTime;
}

double CalculateAlpha(HDesign& hd, HNet changedNet) {
	HPin source = hd.Get<HNet::Source, HPin>(changedNet);
	double Rdriver = Utils::GetDriverWorstPhisics(hd, source, SignalDirection_None).R;
	double c = hd.RoutingLayers.Physics.LinearC;  //удельная электроёмкость
	//double alpha = Rdriver*c;
	//return alpha;
	return Rdriver*c;
}

double CalculateBetta(HDesign& hd, HPinWrapper sink, double oldMDist)
{
	double betta;
	double r = hd.RoutingLayers.Physics.RPerDist; //удельное сопротивление
	double c = hd.RoutingLayers.Physics.LinearC;  //удельная электроёмкость
	double csink = Utils::GetSinkCapacitance(hd, sink, SignalDirection_None);
	betta = CONST_Kd*r*(c*oldMDist*0.5 + csink);
	return betta;
}

inline double CalculateGamma(HDesign& hd)
{
	static double r = hd.RoutingLayers.Physics.RPerDist; //удельное сопротивление
    static double c = hd.RoutingLayers.Physics.LinearC;  //удельная электроёмкость
    static double gamma=CONST_Kd*r*c*0.5;
	return gamma;
}

double GetNetSlack(HDesign& hd, HNet& net)
{
	HPin source = hd.Get<HNet::Source, HPin>(net);
	HTimingPoint sPoint = hd.TimingPoints[source];
	return hd.GetDouble<HTimingPoint::Slack>(sPoint);
}

__inline double GetNetWeight(HDesign& hd, HNet& net)
{
	double slack = GetNetSlack(hd, net);
	if(slack < 0)
		return (CONST_A - CONST_B*slack);
	return CONST_A;
}

double CalculateDiffWTWL(HDesign& hd, std::vector<HNet>& netvector, bool updateCachedValues)
{
	double result = 0.0;
	for(int i = 0; i < netvector.size(); i++)
		result += GetNetWeight(hd, netvector[i]) * Utils::CalculateHPWLDiff(hd, (HNet*)&netvector[i], 1, updateCachedValues);
	return result;
}

BBox GetCurrentBBox3(HDPGrid& hdpp, HDesign& hd, HPinWrapper& myPin)//GetCurrentBBox по пину
{
	//HPinWrapper pin  = hd[hd.Get<HTimingPoint::Pin, HPin>(pointsEnumW.TimingPoint())];
	HNetWrapper netW = hd[myPin.Net()];
	//hd.Plotter->PlotNet(netW);
	//hd.Plotter->Refresh(HPlotter::NO_WAIT);

	HNetWrapper::PinsEnumeratorW pinsEnumW = netW.GetPinsEnumeratorW();
	HCell cell=hd[pinsEnumW].Cell();
	int c=hdpp.CellColumn(cell);
	int r=hdpp.CellRow(cell);
	int rmax=r, rmin=r, cmax=c, cmin=c;

	BBox tBBox;

	while(pinsEnumW.MoveNext())
	{

		HCell cell=hd[pinsEnumW].Cell();

//		if((tBBox.cells.size()!=0)&&(tBBox.cells.back()==cell)){

			//ALERT("AAAAAAA CELL IS ALREADY HEREEEE!!!!!"));
	//	}else
		if((tBBox.cells.size()==0)||(tBBox.cells.back()!=cell)){
			if (!::IsNull(cell))
				tBBox.cells.push_back(cell);
			//else ALERT("Cell is null in CurBBox");
			int c=hdpp.CellColumn(cell);
			int r=hdpp.CellRow(cell);
			if (c>cmax) cmax=c;
			if (c<cmin) cmin=c;
			if (r>rmax) rmax=r;
			if (r<rmin) rmin=r;
		}
	}

	tBBox.col=cmin;
	tBBox.row=rmin;
	tBBox.width=cmax-cmin;
	tBBox.height=rmax-rmin;
	tBBox.colCenter=tBBox.col+(tBBox.width-tBBox.width%2)/2;
	tBBox.rowCenter=tBBox.row+(tBBox.height-tBBox.height%2)/2;

	double x1, y1, x2, y2, xc, yc;
	x1=hdpp.ColumnX(tBBox.col);
	y1=hdpp.RowY(tBBox.row);

	x2=hdpp.ColumnX(tBBox.col+tBBox.width);
	y2=hdpp.RowY(tBBox.row+tBBox.height);

	xc=hdpp.ColumnX(tBBox.colCenter);
	yc=hdpp.RowY(tBBox.rowCenter);

	hd.Plotter->DrawRectangle(x1,y1,x2,y2,Color_Red);
	hd.Plotter->DrawCircle(xc, yc, 6, Color_Red);
	hd.Plotter->Refresh(HPlotter::WAIT_1_SECOND);
	hd.Plotter->SaveMilestoneImage("BBox");
	//hd.Plotter->ShowPlacement();

	return tBBox;
}

/*void ProcessPath(HDPGrid &hdpp, HDesign &hd, HCriticalPaths::EnumeratorW &critPathEnumW, StatisticsAnalyser& stat, TimingHPWLWatcher& thpwlWatcher)
{
	HCell curCell;
	LPoint curPoint;
	BBox curBBox;

	HCell prevCell;
	bool f = false;
	bool isFirst = true;

	int numOfPoint=0;
	for (HCriticalPath::PointsEnumeratorW pointsEnumW = critPathEnumW.GetEnumeratorW(); pointsEnumW.MoveNext();)
	{
		numOfPoint++;
		f = false;
		HPinWrapper pin  = hd[hd.Get<HTimingPoint::Pin, HPin>(pointsEnumW.TimingPoint())];

		if (isFirst) { //если первый элемент критического пути
			prevCell = pin.Cell();
		}
		else prevCell = curCell;

		curCell = pin.Cell();

		if(curCell == prevCell) { //элементы совпадают
			f = true;
		}
		curPoint.col = hdpp.FindColumn(hd.GetDouble<HCell::X>(curCell));
		curPoint.row = hdpp.FindRow(hd.GetDouble<HCell::Y>(curCell));

		if((!f)||isFirst) {  //текущий и предыдущий элементы разные
			if(!::IsNull<HCell>(curCell)) {
				//ALERT("--Taking next point # %d", numOfPoint++));

				if(hd.cfg.ValueOf("HippocratePlacement.SWAP", false)){
					LWindow curWnd;
					GetCurrentWindow(hdpp, RADIUS_OF_WINDOW, curCell, curWnd);
					HippocratePlacementSWAP(hdpp, hd, curCell, curWnd, stat);
				}
				if(hd.cfg.ValueOf("HippocratePlacement.COMPACT", false)){
					curBBox = GetCurrentBBox(hdpp, hd, pointsEnumW);
					HippocratePlacementCOMPACT(hdpp, hd, curCell, curBBox, stat);
				}
				if(hd.cfg.ValueOf("HippocratePlacement.MOVE", false)){
					LWindow curWnd;
					GetCurrentWindow(hdpp, RADIUS_OF_WINDOW, curCell, curWnd);
					HippocratePlacementMOVE(hdpp, hd, curCell, curWnd, stat);
				}
				if(hd.cfg.ValueOf("HippocratePlacement.CENTER", false)){
					LWindow curWnd;
					GetCurrentWindow(hdpp, RADIUS_OF_WINDOW, curCell, curWnd);
					HippocratePlacementCENTER(hdpp, hd, curCell, curWnd, stat);
				}
				if(hd.cfg.ValueOf("HippocratePlacement.LOCALMOVE", false)){
					LWindow curWnd;
					GetCurrentWindow(hdpp, RADIUS_OF_WINDOW, curCell, curWnd);
					HippocratePlacementLOCALMOVE(hdpp, hd, curCell, stat);
				}

				thpwlWatcher.doReport();
			}
		}
		isFirst = false;
	}
}*/

void DoHippocratePlacement( HDPGrid& hdpp, HDesign& hd, StatisticsAnalyser& stat, TimingHPWLWatcher& thpwlWatcher )
{

	if(hd.cfg.ValueOf("HippocratePlacement.SWAP", false)){
		ALERT("-------SWAP ENABLED");
	}
	if(hd.cfg.ValueOf("HippocratePlacement.COMPACT", false)){
		ALERT("-------COMPACT ENABLED");
	}
	if(hd.cfg.ValueOf("HippocratePlacement.MOVE", false)){
		ALERT("-------MOVE ENABLED");
	}
	if(hd.cfg.ValueOf("HippocratePlacement.CENTER", false)){
		ALERT("-------CENTER ENABLED");
	}
	if(hd.cfg.ValueOf("HippocratePlacement.LOCALMOVE", false)){
		ALERT("-------LOCALMOVE ENABLED");
	}

	/*if(hd.cfg.ValueOf("HippocratePlacement.KamaevCheckConstraints", false)){
		ALERT("-------KamaevCheck ENABLED");
	}else{
		ALERT("-------OurCheck ENABLED");
	}

	if(hd.cfg.ValueOf("HippocratePlacement.PathsInCriticalOrder", true)){
		ALERT("-------PathInCriticalOrder ENABLED");
	}else{
		ALERT("-------PathInRandomOrder ENABLED");
	}*/


	//int pp = 0;
	STA(hd, true, true);
	FindCriticalPaths(hd);
	PathCallback callback(hdpp, stat, thpwlWatcher);

	if (hd.cfg.ValueOf("HippocratePlacement.PathsInCriticalOrder", true))
	{
		Utils::IterateMostCriticalPaths(hd, -1, Utils::CriticalPathHandler(&callback, &PathCallback::ProcessPath2));
	}
	else{
		hd.Plotter->ShowPlacement();
		hd.Plotter->SaveMilestoneImage("Before_Hippocrate");

		Utils::CalculateHPWL(hd, true);

		int numOfPath=0;

//		char temp[100];

		for (HCriticalPaths::EnumeratorW critPathEnumW = hd.CriticalPaths.GetEnumeratorW(); critPathEnumW.MoveNext();)
		{
			if(!(numOfPath%10)) ALERT("--Taking next path # %d", numOfPath);
			numOfPath++;

			hd.Plotter->PlotCriticalPath(critPathEnumW);
			hd.Plotter->Refresh(HPlotter::WAIT_3_SECONDS);

			callback.ProcessPath2(hd, critPathEnumW, -1);

			hd.Plotter->ShowPlacement();
		}
	}
}

int RightCounting(int row, int column, int R, int RL, HCell* hcell, HDPGrid& hdpp)	// Поиск вправо
{
	ASSERT(row >= 0 && column >= 0 && row < hdpp.NumRows() && column < hdpp.NumCols());

	int counter = 0;
	while(counter < R + RL + 1 && column < hdpp.NumCols())
	{
		if(isCellExists(hdpp, row, column))
		{
				hcell[counter] = hdpp(row,column);
				counter++;
				if(isCellExists(hdpp, row, column-1))
						if(hdpp(row,column) == hdpp(row,column-1))
								counter--;
		}
		column++;
	}
	return counter;
}

int FindFirstLeftCell(int row, int column, int R, HDPGrid& hdpp)	// Найти первый элемент слева
{
	ASSERT(row >= 0 && column >= 0 && row < hdpp.NumRows() && column < hdpp.NumCols());

	int col = column;
	int counter = 0;
	col--;
	while(counter < R+1 && col >= 0)
	{
			if(isCellExists(hdpp, row, col))
			{
					counter++;
					if(isCellExists(hdpp, row, col+1))
							if(hdpp(row,col) == hdpp(row,col+1))
									counter--;
			}
			col--;
	}
	if(col < 0)
			col = 0;
	while(((!isCellExists(hdpp, row, col))&&(col<hdpp.NumCols())))
	{
		col++;
	}
	if (col==hdpp.NumCols()) col=-1;
	return col;
}

std::vector<HCell> GetCurrentWindow(HDPGrid& hdpp, int R, HCell curCell)	// Вернуть окно элемента // moded
{
	std::vector<HCell> New;
	hdpp.Design().Plotter->PlotCell(curCell, Color_Indigo);
	hdpp.Design().Plotter->Refresh(HPlotter::WAIT_1_SECOND);

	int RL = 0; //кол-во cell'ов слева от текущего cell'а (в окне)
	int row = hdpp.CellRow(curCell);        // номер ряда текущего cell'а
	int column = hdpp.CellColumn(curCell);  // номер сайта текущего cell'а

	int x_min = FindFirstLeftCell(row, column, R, hdpp);

	while(x_min < column)
	{
			if(isCellExists(hdpp, row, x_min)) {
					RL++;
					if(isCellExists(hdpp, row, x_min-1)) {
							if(hdpp(row,x_min) == hdpp(row,x_min-1))
									RL--;
					}
			}
			x_min++;
	}

  int maxcount = (R + RL + 1)*(2*R + 1);  // максимальное кол-во cell'ов в окне
	HCell* hcell = new HCell[maxcount];	    // выделяем память под массив cell'ов

	int y_min = row - R;
	int y_max = row + R;

	if(y_min < 0)
		y_min = 0;

	if(y_max > hdpp.NumRows() - 1)
		y_max = hdpp.NumRows() - 1;

	int temp = 0;
	for(int i = y_min; i <= y_max; i++)	{
			//ALERT("-- before FindFirstLeftCell %d",cellCounter);
			//if ((pathNumber==5)&&(cellCounter==10))
				//int qweerrrr=0;
		x_min = FindFirstLeftCell(i, column, R, hdpp);///////////HERE
		if (x_min==-1) continue;
			//ALERT("-- after FindFirstLeftCell");
		temp += RightCounting(i, x_min, R, RL, hcell + temp, hdpp);

	}
	for(int j = 0; j < temp; j++)	{
		New.push_back(hcell[j]);
	}
	delete[] hcell;
	return New;
}
SpecWindow getSpecWindForCell(HDPGrid& hdpp, HDesign& hd, HCell& CellFromNet, BBox& curBBox )
{
	SpecWindow SpWind;
		if ((hdpp.CellColumn(CellFromNet)<=curBBox.colCenter)&&((hdpp.CellRow(CellFromNet)>=curBBox.rowCenter)))
	{
		SpWind.StartRow=hdpp.CellRow(CellFromNet);
		SpWind.StartCol=hdpp.CellColumn(CellFromNet);
		SpWind.FinishRow=curBBox.rowCenter;
		SpWind.FinishCol=curBBox.colCenter;
	}else
		if ((hdpp.CellColumn(CellFromNet)>curBBox.colCenter)&&((hdpp.CellRow(CellFromNet)>=curBBox.rowCenter)))
		{
			SpWind.StartRow=hdpp.CellRow(CellFromNet);
			SpWind.StartCol=curBBox.colCenter;
			SpWind.FinishRow=curBBox.rowCenter;
			SpWind.FinishCol=hdpp.CellColumn(CellFromNet);
		}else
			if ((hdpp.CellColumn(CellFromNet)<=curBBox.colCenter)&&((hdpp.CellRow(CellFromNet)<curBBox.rowCenter)))
			{
				SpWind.StartRow=curBBox.rowCenter;
				SpWind.StartCol=hdpp.CellColumn(CellFromNet);
				SpWind.FinishRow=hdpp.CellRow(CellFromNet);
				SpWind.FinishCol=curBBox.colCenter;
			}else
				if ((hdpp.CellColumn(CellFromNet)>curBBox.colCenter)&&((hdpp.CellRow(CellFromNet)<curBBox.rowCenter)))
				{
					SpWind.StartRow=curBBox.rowCenter;
					SpWind.StartCol=curBBox.colCenter;
					SpWind.FinishRow=hdpp.CellRow(CellFromNet);
					SpWind.FinishCol=hdpp.CellColumn(CellFromNet);
				}
	return(SpWind);
}
std::vector<PseudoCell> getGapsForCell(HDPGrid& hdpp, HDesign& hd, HCell& CellFromNet, BBox& curBBox, bool InsideGaps )
{
	std::vector<PseudoCell> VGaps;
	PseudoCell newGap;
	SpecWindow SpWind = getSpecWindForCell(hdpp,hd,CellFromNet,curBBox);
	int extraSites=-1;
	for(int TempRow=SpWind.StartRow;TempRow>=SpWind.FinishRow;TempRow--)
	{
		int TempCol=SpWind.StartCol;
		if (hdpp.NumCols()-SpWind.FinishCol-1 < 10) extraSites=hdpp.NumCols()-SpWind.FinishCol-1;
		else extraSites = 10;

		while ((TempCol<=SpWind.FinishCol)||((TempCol<=SpWind.FinishCol+extraSites)&&(!isCellExists(hdpp, TempRow, TempCol))))
		{
			if (!isCellExists(hdpp, TempRow, TempCol))
			{
				newGap.col=TempCol;
				newGap.row=TempRow;
				newGap.GapSize=0;
				while ((!isCellExists(hdpp, TempRow, TempCol))&&(TempCol<=SpWind.FinishCol+extraSites))
				{
					TempCol++;
					newGap.GapSize++;
				}
				if (((InsideGaps)&&((newGap.col!=SpWind.StartCol)||(newGap.col+newGap.GapSize<SpWind.FinishCol)))||(!InsideGaps ))
				VGaps.push_back(newGap);
			}
			else TempCol++;
		}

	}

	return VGaps;
}
std::vector<PossiblePos> getVPossibleHorizontalPos(HDPGrid& hdpp, HCell curCell)
{
	PseudoCell LocalGap;
	LocalGap.row=hdpp.CellRow(curCell);
	LocalGap.col=hdpp.CellColumn(curCell)-GetLeftFreeSpace(curCell, hdpp);
	LocalGap.GapSize=hdpp.CellSitesNum(curCell)+GetRightFreeSpace(curCell, hdpp)+GetLeftFreeSpace(curCell, hdpp);

	std::vector<PossiblePos> VPossibleHorizontalPos;
	for (int i=0; i<LocalGap.GapSize-hdpp.CellSitesNum(curCell)+1;i++)
	{
		PossiblePos tmp;
		tmp.row=LocalGap.row;
		tmp.col=LocalGap.col + i;
		VPossibleHorizontalPos.push_back(tmp);
	}


	return VPossibleHorizontalPos;
}

std::vector<PseudoCell> getPseudoCellsFromSpecWindow(HDPGrid& hdpp, HDesign& hd, std::vector<HCell>& VCellInSpWind )
{
	std::vector<PseudoCell> VPsCellInSpWindow;

	int imax=VCellInSpWind.size()-1;
	for (int i=0;i<imax;i++)
	{
		HCell currentCell=VCellInSpWind[i];

		HCell nextcell=VCellInSpWind[i+1];
		if (hdpp.CellRow(nextcell) == hdpp.CellRow(currentCell))
		{
			int holeSize=hdpp.CellColumn(VCellInSpWind[i+1])-
				(hdpp.CellColumn(VCellInSpWind[i])+
				hdpp.CellSitesNum(VCellInSpWind[i]));
			if (holeSize>0)
			{
				PseudoCell temp;
				temp.col=hdpp.CellColumn(VCellInSpWind[i])+hdpp.CellSitesNum(VCellInSpWind[i]);
				temp.row=hdpp.CellRow(VCellInSpWind[i]);
				temp.GapSize=holeSize;
				VPsCellInSpWindow.push_back(temp);
			}
		}
	}
	return (VPsCellInSpWindow);
}

std::vector<PseudoCell> getPseudoCellsFromWindow(HDPGrid& hdpp, HDesign& hd, std::vector<HCell>& VCellsWindow){
	static int windownum=0;

	//VCellsWindow.reserve(curWnd.count);
	//ALERTFORMAT(("count: %d", curWnd.count));

	std::vector<HCell>::iterator iterVCellsWindow=VCellsWindow.begin();

	//hd.Plotter->PlotCell(curCell, Color_Red);
	//hd.Plotter->Refresh(HPlotter::NO_WAIT);
	//hd.Plotter->SaveMilestoneImage("CELLS_IN_WINDOW");


	//ALERTFORMAT(("SORT1"));
	sort(VCellsWindow.begin(), VCellsWindow.end(), CellRowComparator(hdpp));

	//ALERTFORMAT(("SORT2"));
	int numOfLastRow=hdpp.CellRow(*(VCellsWindow.end()-1));
	iterVCellsWindow=VCellsWindow.begin();
	std::vector<HCell>::iterator tmp=iterVCellsWindow;
	for(;iterVCellsWindow!=VCellsWindow.end();iterVCellsWindow++){
		//ALERTFORMAT(("PROCESSING ELEMENT %d",hdpp.CellRow(*(iterVCellsWindow))));
		if (iterVCellsWindow+1 == VCellsWindow.end())
		{
			sort(tmp,VCellsWindow.end(),CellColComparator(hdpp));
			break;
		}else{
			if (hdpp.CellRow(*(iterVCellsWindow+1)) != hdpp.CellRow(*(iterVCellsWindow))){
				//ALERTFORMAT(("PROCESSING ROW %d",hdpp.CellRow(*(iterVCellsWindow))));
				sort(tmp,iterVCellsWindow+1,CellColComparator(hdpp));
				tmp=iterVCellsWindow+1;
			}
		}
	}

	//-----------------filling std::vector<PseudoCell>-------------------

	iterVCellsWindow=VCellsWindow.begin();
	std::vector<PseudoCell> VPsCellWindow;
	for(;iterVCellsWindow!=VCellsWindow.end()-1;iterVCellsWindow++)
	{

		if (hdpp.CellRow(*(iterVCellsWindow+1)) == hdpp.CellRow(*(iterVCellsWindow)))
		{
			int holeSize=hdpp.CellColumn(*(iterVCellsWindow+1))-
				(hdpp.CellColumn(*iterVCellsWindow)+
				hdpp.CellSitesNum(*iterVCellsWindow));
			if (holeSize>0)
			{
				PseudoCell temp;
				temp.col=hdpp.CellColumn(*iterVCellsWindow)+hdpp.CellSitesNum(*iterVCellsWindow);
				temp.row=hdpp.CellRow(*(iterVCellsWindow));
				temp.GapSize=holeSize;
				VPsCellWindow.push_back(temp);
			}
		}
	}
	//ALERTFORMAT(("Num of Holes: %d", VPsCellWindow.size()));

	hd.Plotter->ShowPlacement();

	VCellsWindow.clear();
	return (VPsCellWindow);
}


std::vector<HCell> GetCellsConnectedWithCurrent(HDPGrid& hdpp, HDesign& hd, HCell curCell)
{
	std::vector<HCell> ConnectedCells; //эл-ты, соединённые с текущим
	int connectedCellsCount = 0, j = 0;

	//hd.Plotter->PlotCell(curCell, Color_Red);
	//hd.Plotter->Refresh(HPlotter::WAIT_1_SECOND);

	//заполняем вектор эл-тов, соединённых с текущим
	for(HCell::PinsEnumeratorW pin = hd.Get<HCell::Pins, HCell::PinsEnumeratorW>(curCell); pin.MoveNext(); ) {
		HNet net = hd.Get<HPin::Net,HNet>(pin);
		HNetWrapper netw = hd[hd.Get<HPin::Net,HNet>(pin)];
		if(!::IsNull(net)) {
			//hd.Plotter->PlotNet(netw);
			//hd.Plotter->Refresh(HPlotter::WAIT_1_SECOND);
			if(pin == hd[hd.Get<HNet::Source, HPin>(netw)]) { //если source
				for (HNet::PinsEnumeratorW npin = netw.GetPinsEnumeratorW(); npin.MoveNext(); ) {
					if(npin!=hd[hd.Get<HNet::Source, HPin>(netw)])
						if(!::IsNull(npin.Cell())){
							ConnectedCells.push_back(npin.Cell());
						}
				}
			}
			else {//если sink
				HCell cell = hd[hd.Get<HNet::Source, HPin>(hd.Get<HPin::Net,HNet>(pin))].Cell();
				if(!::IsNull(cell))
					ConnectedCells.push_back(cell);
			}
		}
	}
	/*for(int i=0; i<ConnectedCells.size(); i++) {
	hd.Plotter->Refresh(HPlotter::WAIT_1_SECOND);
	hd.Plotter->PlotCell(ConnectedCells[i], Color_Black);
	hd.Plotter->Refresh(HPlotter::WAIT_1_SECOND);
	}*/
	return ConnectedCells;
}


void StatisticsAnalyser::incrementAttempts( HippocrateMethods method )
{
	switch (method)
	{
	case MOVE:
		numOfMOVEAttepmts++;
		break;
	case COMPACT:
		numOfCOMPACTAttepmts++;
		break;
	case CENTER:
		numOfCENTERAttepmts++;
		break;
	case LOCALMOVE:
		numOfLOCALMOVEAttepmts++;
		break;
	case SWAP:
		numOfSWAPAttepmts++;
		break;
	}
}

void StatisticsAnalyser::incrementAttemptsSuccessful( HippocrateMethods method )
{
	switch (method)
	{
	case MOVE:
		if (reportSuccessful)	ALERT("!!!ELEMENTS MOVED!!!");
		numOfMOVESuccessfulAttempts++;
		break;
	case COMPACT:
		if (reportSuccessful)	ALERT("!!!ELEMENTS MOVED (COMPACT)!!!");
		numOfCOMPACTSuccessfulAttempts++;
		break;
	case CENTER:
		if (reportSuccessful)	ALERT("!!!ELEMENTS CENTERED!!!");
		numOfCENTERSuccessfulAttempts++;
		break;
	case LOCALMOVE:
		if (reportSuccessful)	ALERT("!!!ELEMENTS localMOVED!!!");
		numOfLOCALMOVESuccessfulAttempts++;
		break;
	case SWAP:
		if (reportSuccessful)	ALERT("!!!ELEMENTS SWAPPED!!!");
		numOfSWAPSuccessfulAttempts++;
		break;
	}
}

StatisticsAnalyser::StatisticsAnalyser(){
	reportSuccessful=true;
	numOfSWAPAttepmts=0;
	numOfSWAPSuccessfulAttempts=0;
	numOfMOVEAttepmts=0;
	numOfMOVESuccessfulAttempts=0;
	numOfLOCALMOVEAttepmts=0;
	numOfLOCALMOVESuccessfulAttempts=0;
	numOfCOMPACTAttepmts=0;
	numOfCOMPACTSuccessfulAttempts=0;
	numOfCENTERAttepmts=0;
	numOfCENTERSuccessfulAttempts=0;
}

void StatisticsAnalyser::doReport(){
	ALERT("SWAP attempts: %d", numOfSWAPAttepmts);
	ALERT("SWAP successful attempts: %d", numOfSWAPSuccessfulAttempts);
	ALERT("MOVE attempts: %d", numOfMOVEAttepmts);
	ALERT("MOVE successful attempts: %d", numOfMOVESuccessfulAttempts);
	ALERT("LOCALMOVE attempts: %d", numOfLOCALMOVEAttepmts);
	ALERT("LOCALMOVE successful attempts: %d", numOfLOCALMOVESuccessfulAttempts);
	ALERT("COMPACT attempts: %d", numOfCOMPACTAttepmts);
	ALERT("COMPACT successful attempts: %d", numOfCOMPACTSuccessfulAttempts);
	ALERT("CENTER attempts: %d", numOfCENTERAttepmts);
	ALERT("CENTER successful attempts: %d", numOfCENTERSuccessfulAttempts);
}


TimingHPWLWatcher::TimingHPWLWatcher(HDesign& _hd, bool _watch):hd(_hd),watch(_watch)
{
	STA(hd,false);
	oldTNS=Utils::TNS(hd);
	oldWNS=Utils::WNS(hd);
	oldHPWL=Utils::CalculateHPWL(hd, true);
	callCounter=0;

	// KNOTE: I find it useless, while it creates a lot of logs
	// srand( (unsigned)time( NULL ) );
	// int rnd = int(rand()*1000);
	// sprintf(filename, "TimingHPWLWatcher%d.log", rnd);
	// FILE* fOUT;
	// fOUT = fopen(filename, "a");
	// if (fOUT) {
	// 	fprintf(fOUT, "\n---------(%s)------------\n",(const char*)hd.cfg.ValueOf("benchmark.def"));
	// 	fclose(fOUT);
	// }
	// else
	// 	ALERT("Can't open %s in constructor", filename);
}

void TimingHPWLWatcher::doReport()
{
	if (watch){
		STA(hd,false);
		newTNS=Utils::TNS(hd);
		newWNS=Utils::WNS(hd);
		newHPWL=Utils::CalculateHPWL(hd, true);
		TNSImprovement=(oldTNS-newTNS)/oldTNS*100;
		WNSImprovement=(oldWNS-newWNS)/oldWNS*100;
		HPWLImprovement=(oldHPWL-newHPWL)/oldHPWL*100;
		if (WNSImprovement || TNSImprovement || HPWLImprovement){
			//sprintf(name, "move_s298_%d_%d_%d.def",iterationNumber,numOfPath,numOfPoint);
			//ExportDEF(hd, name);
			ALERT("%d:\tTNS:\t%f\tWNS:%f\tHPWL:%f",callCounter,TNSImprovement, WNSImprovement,HPWLImprovement);
			FILE* fOUT;
			fOUT=fopen(filename,"a");
			if (fOUT){
				//ALERT("Writing to file");
				fprintf(fOUT, "%d:\tTNS:\t%f\tWNS:%f\tHPWL:%f\n",callCounter,TNSImprovement, WNSImprovement,HPWLImprovement);
				fclose(fOUT);
			}else ALERT("Cant open %s",filename);
			callCounter++;
		}
		oldTNS=newTNS;
		oldWNS=newWNS;
		oldHPWL=newHPWL;
	}

}
std::vector<HPinWrapper> CellsInPath(HDesign& hd, HCriticalPath path)
{
	std::vector<HPinWrapper> VPin;
	HCell curCell;


	HCell prevCell;
	bool f = false;
	bool isFirst = true;

	int numOfPoint=0;
	for (HCriticalPath::PointsEnumeratorW pointsEnumW = hd[path].GetEnumeratorW(); pointsEnumW.MoveNext();)
	{
		numOfPoint++;

		f = false;
		HPinWrapper pin  = hd[hd.Get<HTimingPoint::Pin, HPin>(pointsEnumW.TimingPoint())];

		if (isFirst) { //если первый элемент критического пути
			prevCell = pin.Cell();
		}
		else prevCell = curCell;

		curCell = pin.Cell();

		if(curCell == prevCell) { //элементы совпадают
			f = true;
		}
		if((!f)||isFirst){ //текущий и предыдущий элементы разные
			if(!::IsNull<HCell>(curCell)) VPin.push_back(pin);
		}
		isFirst = false;
	}
	return VPin;
}

void PathCallback::ProcessPath2(HDesign& hd, HCriticalPath path, int pathNumber)// moded
{
	//ALERT("path %d:", pathNumber);
	std::vector<HPinWrapper> myPins = CellsInPath(hd, path);

	for (std::vector<HPinWrapper>::iterator Iter = myPins.begin(); Iter < myPins.end(); Iter++)
	{
		if(hd.cfg.ValueOf("HippocratePlacement.SWAP", false)){
			std::vector<HCell> curWnd = GetCurrentWindow(hdpp, RADIUS_OF_WINDOW,Iter->Cell());
            HCell curCell = Iter->Cell();
            HippocratePlacementSWAP(hdpp, hd, curCell, curWnd, stat);
		}
		if(hd.cfg.ValueOf("HippocratePlacement.COMPACT", false)){
			BBox curBBox = GetCurrentBBox3(hdpp, hd, *Iter);
            HCell curCell = Iter->Cell();
            HippocratePlacementCOMPACT(hdpp, hd, curCell, curBBox, stat);
		}
		if(hd.cfg.ValueOf("HippocratePlacement.MOVE",false)){
			std::vector<HCell> curWnd = GetCurrentWindow(hdpp, RADIUS_OF_WINDOW,Iter->Cell());
            HCell curCell = Iter->Cell();
            HippocratePlacementMOVE(hdpp, hd, curCell, curWnd, stat);
		}
		if(hd.cfg.ValueOf("HippocratePlacement.CENTER", false)){
			std::vector<HCell> curWnd = GetCurrentWindow(hdpp, RADIUS_OF_WINDOW,Iter->Cell());
            HCell curCell = Iter->Cell();
            HippocratePlacementCENTER(hdpp, hd, curCell, curWnd, stat);
		}
		if(hd.cfg.ValueOf("HippocratePlacement.LOCALMOVE", false)){
			//LWindow curWnd;
			//GetCurrentWindow(hdpp, RADIUS_OF_WINDOW, curCell, curWnd);
            HCell curCell = Iter->Cell();
            HippocratePlacementLOCALMOVE(hdpp, hd, curCell, stat);
		}

		thpwlWatcher.doReport();
	}
}

//void PathCallback::ProcessPath2(HDesign& hd, HCriticalPath path, int pathNumber)
//{
//	HCell curCell;
//	LPoint curPoint;
//	BBox curBBox;
//
//	HCell prevCell;
//	bool f = false;
//	bool isFirst = true;
//
//	int numOfPoint=0;
//	int numOfCell=0;
//	for (HCriticalPath::PointsEnumeratorW pointsEnumW = hd[path].GetEnumeratorW(); pointsEnumW.MoveNext();)
//	{
//		numOfPoint++;
//		f = false;
//		HPinWrapper pin  = hd[hd.Get<HTimingPoint::Pin, HPin>(pointsEnumW.TimingPoint())];
//
//		if (isFirst) { //если первый элемент критического пути
//			prevCell = pin.Cell();
//		}
//		else prevCell = curCell;
//
//		curCell = pin.Cell();
//
//		if(curCell == prevCell) { //элементы совпадают
//			f = true;
//		}
//		curPoint.col = hdpp.FindColumn(hd.GetDouble<HCell::X>(curCell));
//		curPoint.row = hdpp.FindRow(hd.GetDouble<HCell::Y>(curCell));
//
//		if((!f)||isFirst) {  //текущий и предыдущий элементы разные
//			if(!::IsNull<HCell>(curCell)) {
//				//ALERTFORMAT(("--Taking next point # %d", numOfPoint++));
//				numOfCell++;
//				if(hd.cfg.ValueOf("HippocratePlacement.SWAP", false)){
//					//LWindow curWnd;
//					std::vector<HCell> curWnd = GetCurrentWindow(hdpp, RADIUS_OF_WINDOW, curCell);
//					HippocratePlacementSWAP(hdpp, hd, curCell, curWnd, stat);
//				}
//				if(hd.cfg.ValueOf("HippocratePlacement.COMPACT", false)){
//					curBBox = GetCurrentBBox(hdpp, hd, pointsEnumW);
//					HippocratePlacementCOMPACT(hdpp, hd, curCell, curBBox, stat);
//				}
//				if(hd.cfg.ValueOf("HippocratePlacement.MOVE", false)){
//					//LWindow curWnd;
//					ALERTFORMAT(("---------path number------------- %d ",pathNumber ));
//					ALERTFORMAT(("---------new cell------------- %d ",numOfCell ));
//					if ((numOfCell==10)&&(pathNumber == 5))
//						int wer=0;
//					std::vector<HCell> curWnd = GetCurrentWindow(hdpp, RADIUS_OF_WINDOW, curCell);
//					HippocratePlacementMOVE(hdpp, hd, curCell, curWnd, stat);
//				}
//				if(hd.cfg.ValueOf("HippocratePlacement.CENTER", false)){
//					//LWindow curWnd;
//					std::vector<HCell> curWnd = GetCurrentWindow(hdpp, RADIUS_OF_WINDOW, curCell);
//					HippocratePlacementCENTER(hdpp, hd, curCell, curWnd, stat);
//				}
//				if(hd.cfg.ValueOf("HippocratePlacement.LOCALMOVE", false)){
//					//LWindow curWnd;
//					//GetCurrentWindow(hdpp, RADIUS_OF_WINDOW, curCell, curWnd);
//					HippocratePlacementLOCALMOVE(hdpp, hd, curCell, stat);
//				}
//
//				thpwlWatcher.doReport();
//			}
//		}
//		isFirst = false;
//	}
//}


bool isCellExists(HDPGrid& hdpp, int row, int col)
{
	if ((row>=0) && (col>=0) && (row<hdpp.NumRows()) && (col<hdpp.NumCols()) && !(::IsNull(hdpp(row, col)))) return true;
	else return false;
}
