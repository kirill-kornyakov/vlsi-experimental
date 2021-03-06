#include "DetailedPlacement.h"
#include "DetailedPlacementSwappers.h"
#include "Utils.h"
#include "Auxiliary.h"
#include <stdlib.h>
//#include <conio.h>

GlobalSwapper::GlobalSwapper(HDPGrid& g, int optimalRegionHalfWidth, int optimalRegionHalfHeight,
  ConstraintsController* _checker): m_Grid(g), m_OptimalRegionHalfWidth(optimalRegionHalfWidth),
  m_OptimalRegionHalfHeight(optimalRegionHalfHeight), checker(_checker)
{
  //NOTE: maximum possible number of considered cells = 1 + OptimalRegionWidth()
  //(current cell + number of sites along optimal region)
  m_ConsideredGroupOfCells = new HCell[1 + OptimalRegionWidth()];
  BestPositions = new int[1 + OptimalRegionWidth()];
  CurrentPositions = new int[1 + OptimalRegionWidth()];
}

GlobalSwapper::~GlobalSwapper()
{
  delete[] m_ConsideredGroupOfCells;
  delete[] BestPositions;
  delete[] CurrentPositions;
}

int GlobalSwapper::OptimalRegionWidth()
{
  return 2 * m_OptimalRegionHalfWidth + 1;
}

int GlobalSwapper::OptimalRegionHeight()
{
  return 2 * m_OptimalRegionHalfHeight + 1;
}

void GlobalSwapper::NullConsideredGroupOfCells()
{
  for (int i = 0; i < 1 + OptimalRegionWidth(); i++)
  {
    m_ConsideredGroupOfCells[i] = m_Grid.Design().Cells.Null();
  }
  m_GroupSize = 0;
}

bool GlobalSwapper::IsAlreadyInOptimalRegion()
{
  int currSiteIdx = m_Grid.CellColumn(m_Cell);
  int currRowIdx  = m_Grid.CellRow(m_Cell);

  return currSiteIdx >= m_OriginColIdx && currSiteIdx <= m_OriginColIdx + OptimalRegionWidth() 
      && currRowIdx  >= m_OriginRowIdx && currRowIdx  <= m_OriginRowIdx + OptimalRegionHeight();
}

int GlobalSwapper::GetNextRowIdx(int& rowIdx)
{
  int optimalRowIdx = OptimalRow();
  int rowShift = rowIdx - optimalRowIdx;
  rowShift = -rowShift;
  rowIdx = optimalRowIdx + rowShift;
  if (rowIdx >= optimalRowIdx)
  {
    rowIdx++;
  }

  return rowIdx;
}

bool GlobalSwapper::DoesRowBelongToOptimalRegion(int rowIdx)
{
  return rowIdx >= m_OriginRowIdx && rowIdx < m_OriginRowIdx + OptimalRegionHeight() 
      && rowIdx >= 0 && rowIdx < m_Grid.NumRows();
}

bool GlobalSwapper::IsColWithinBorders(int colIdx)
{
  return colIdx >= 0 && colIdx < m_Grid.NumCols();
}

bool GlobalSwapper::DoesColBelongToOptimalRegion(int colIdx)
{
  return colIdx >= m_OriginColIdx 
    && colIdx < m_OriginColIdx + OptimalRegionWidth()
    && IsColWithinBorders(colIdx);
}


void GlobalSwapper::RecheckBestResult(int siteIdx, int currFreeSpace, int &maxFreeSpace, int &maxFreeSpaceOrigin)
{
  if (currFreeSpace > maxFreeSpace)
  {
    maxFreeSpace = currFreeSpace;
    maxFreeSpaceOrigin = siteIdx - currFreeSpace;
  }
}

void GlobalSwapper::AnalyzeFreeSpace(int rowIdx, int& rowFreeSpace, int& maxFreeSpace, int& maxFreeSpaceOrigin)
{
  int currFreeSpace = 0;

  //2.1.1 calculate free space in optimal region
  int siteIdx = m_OriginColIdx;
  for (; DoesColBelongToOptimalRegion(siteIdx); siteIdx++)
  {
    if (::IsNull(m_Grid(rowIdx, siteIdx)))
    {
      rowFreeSpace++;
      currFreeSpace++;
    }
    else
    {
      RecheckBestResult(siteIdx, currFreeSpace, maxFreeSpace, maxFreeSpaceOrigin);
      currFreeSpace = 0;
    }
  }
  //following call needed if the last site in current row is free
  RecheckBestResult(siteIdx, currFreeSpace, maxFreeSpace, maxFreeSpaceOrigin);
}

bool GlobalSwapper::TryToMoveWithoutShifting(int rowIdx, int siteIdx)
{
  //place the cell into optimal region and look at the benefit
  m_Grid.PutCellFast(m_Cell, rowIdx, siteIdx);
  NullConsideredGroupOfCells();
  m_ConsideredGroupOfCells[0] = m_Cell;
  m_GroupSize = 1;

  if (checker->CheckCriteria(m_ConsideredGroupOfCells, m_GroupSize))
  {//accept move
    m_Grid.PutCell(m_Cell, rowIdx, siteIdx);
    Utils::CalculateHPWLDiff(m_Grid.Design(), m_ConsideredGroupOfCells, m_GroupSize, true);
    return true;
  }
  else
  {//rollback
    m_Grid.PutCellFast(m_Cell, m_Grid.CellRow(m_Cell), m_Grid.CellColumn(m_Cell));
    return false;
  }
}

void GlobalSwapper::ReleaseSpaceForCell(int rowIdx, int freeSiteIdx, unsigned int nCandidatesForShifting)
{
  HCell currentCell = m_Grid.Design().Cells.Null();
  HCell nextCell = m_Grid.Design().Cells.Null();
  
  //WARNING: do not place increment of m_GroupSize into FOR statement
  //there is tricky dependence of lines
  for (m_GroupSize = 0; m_GroupSize < nCandidatesForShifting; )
  {
    //shift one more cell
    //NOTE: first cell is our current cell, we simply place it
    currentCell = m_ConsideredGroupOfCells[m_GroupSize];
    m_Grid.PutCellFast(currentCell, rowIdx, freeSiteIdx);
    BestPositions[m_GroupSize] = freeSiteIdx;
    m_GroupSize++;

    //check if space enough
    int cellWidth = (int)m_Grid.CellSitesNum(currentCell);
    freeSiteIdx += cellWidth;
    int forbiddenSiteIdx;
    nextCell = m_ConsideredGroupOfCells[m_GroupSize];
    if (!::IsNull(nextCell))
    {
      forbiddenSiteIdx = m_Grid.CellColumn(nextCell);
      if (freeSiteIdx <= forbiddenSiteIdx)
        break; //escape because space released
    }
    else
    {
      forbiddenSiteIdx = m_OriginColIdx + OptimalRegionWidth(); 
      ASSERT(freeSiteIdx <= forbiddenSiteIdx);
      break;
    }
  }
}

bool GlobalSwapper::TryToMoveWithShifting(int rowIdx)
{
  //find the first free site from the left,
  //we will place cell there and shift needed number of following cells
  int firstFreeSiteIdx = m_OriginColIdx;
  while (!::IsNull(m_Grid(rowIdx, firstFreeSiteIdx)))
    ++firstFreeSiteIdx;
  
  //calculate length of found free region and find first occupied site
  int currFreeSpace = 0;
  int firstOccupiedSiteIdx = firstFreeSiteIdx;
  for (; DoesColBelongToOptimalRegion(firstOccupiedSiteIdx); ++firstOccupiedSiteIdx)
  {
    if (::IsNull(m_Grid(rowIdx, firstOccupiedSiteIdx)))
    {
      currFreeSpace++;
    }
    else
      break;
  }

  //fill m_ConsideredGroupOfCells
  NullConsideredGroupOfCells();
  unsigned int nCandidatesForShifting = 0;
  //put current cell
  HCell currentCell = m_Cell;
  m_ConsideredGroupOfCells[nCandidatesForShifting] = currentCell;
  CurrentPositions[nCandidatesForShifting] = m_Grid.CellColumn(currentCell); 
  //TODO: we actually don't need coordinates above (CurrentPositions), because we do PutCellFast,
  //so we can get information from grid. Think about this moment.
  nCandidatesForShifting++;
  //put cells from optimal region
  HCell lastCellInGroup = m_Grid.Design().Cells.Null();
  for (int i = firstOccupiedSiteIdx; DoesColBelongToOptimalRegion(i); ++i)
  {
    HCell currentCell = m_Grid(rowIdx, i);
    if (!::IsNull(currentCell))
    {
      if (currentCell != lastCellInGroup)
      {//add new cell to the group
        lastCellInGroup = currentCell;
        m_ConsideredGroupOfCells[nCandidatesForShifting] = currentCell;
        CurrentPositions[nCandidatesForShifting] = m_Grid.CellColumn(currentCell);
        nCandidatesForShifting++;
      }
    }
  }

  ReleaseSpaceForCell(rowIdx, firstFreeSiteIdx, nCandidatesForShifting);

  if (checker->CheckCriteria(m_ConsideredGroupOfCells, m_GroupSize))
  {//put cells to the better position found (probably initial position)
    for (unsigned int j = 0; j < m_GroupSize; ++j)
      m_Grid.PutCell(m_ConsideredGroupOfCells[j], rowIdx, BestPositions[j]);
    Utils::CalculateHPWLDiff(m_Grid.Design(), m_ConsideredGroupOfCells, m_GroupSize, true);
    return true;
  }
  else
  {//rollback
    for (unsigned int j = 0; j < m_GroupSize; ++j)
      m_Grid.PutCellFast(m_ConsideredGroupOfCells[j], m_Grid.CellRow(m_ConsideredGroupOfCells[j]), CurrentPositions[j]);
    return false;
  }
  return false;
}

bool GlobalSwapper::TryToMoveCellToOptimalRegion()
{
  string cellName = m_Grid.Design().GetString<HCell::Name>(m_Cell).c_str();

  //try to place into every row starting from optimal row
  for (int rowIdx = OptimalRow(); DoesRowBelongToOptimalRegion(rowIdx); GetNextRowIdx(rowIdx))
  {
    //2.1 try to find free space for the cell
    int availableFreeSpace = 0; //entire free space in current row within the optimal region
    int maxFreeSpace = 0;       //length of maximum continual free space region
    int maxFreeSpaceOrigin = 0; //index of site from which starts maximum continual free space region
    AnalyzeFreeSpace(rowIdx, availableFreeSpace, maxFreeSpace, maxFreeSpaceOrigin);

    //2.2.2 if there is enough space, place the cell there and BREAK
    if (availableFreeSpace >= m_CellWidthInSites)
    {
      if (maxFreeSpace >= m_CellWidthInSites) 
      {//if there is enough space in a continual free space region
        if (TryToMoveWithoutShifting(rowIdx, maxFreeSpaceOrigin))
        {
          //m_Grid.Design().Plotter->PlotCell(m_Cell);
          //ALERT("- %s\t", cellName.c_str()));
          return true;
        }
      }
      else
      {//if there is still enough space but shifting of other cells is needed
        if (TryToMoveWithShifting(rowIdx))
        {
          //m_Grid.Design().Plotter->PlotCell(m_Cell);
          //ALERT("+ %s\t", cellName.c_str()));
          return true;
        }
      }
    }
  }
  return false;
}

int GlobalSwapper::OptimalRow()
{
  return m_OriginRowIdx + m_OptimalRegionHalfHeight;
}

int GlobalSwapper::OptimalColumn()
{
  return m_OriginColIdx + m_OptimalRegionHalfWidth;
}

bool GlobalSwapper::TryToExchangeWithCell()
{
  int nAvailableSitesNearTrialCell;
  int nAvailableSitesNearCurrentCell;

  NullConsideredGroupOfCells();
  m_GroupSize = 0;
  m_ConsideredGroupOfCells[m_GroupSize] = m_Cell;
  m_GroupSize = 1;

  int currCellColIdx = m_Grid.CellColumn(m_Cell);
  int currCellRowIdx = m_Grid.CellRow(m_Cell);

  //calculate free space from the left side of current cell
  //REFACTOR: extract method CalcFreeSpaceNearCell
  int leftFreeSpace = 0;
  for (int j = currCellColIdx - 1; IsColWithinBorders(j); j--)
  {
    if (::IsNull(m_Grid(currCellRowIdx, j)))
    {
      leftFreeSpace++;
    }
    else
    {
      break;
    }
  }

  //calculate free space from the right side of current cell
  //REFACTOR: extract method CalcFreeSpaceNearCell
  int rightFreeSpace = 0;
  for (int j = currCellColIdx + m_CellWidthInSites; IsColWithinBorders(j); j++)
  { 
    if (::IsNull(m_Grid(currCellRowIdx, j)))
    {
      rightFreeSpace++;
    }
    else
    {
      break;
    }
  }

  //calculate currNumOfAvailableSites - available space after current cell removing
  nAvailableSitesNearCurrentCell = leftFreeSpace + m_CellWidthInSites + rightFreeSpace;

  //try to exchange with every cell from optimal region,
  //escape after first successive try
  for (int rowIdx = OptimalRow(); DoesRowBelongToOptimalRegion(rowIdx); GetNextRowIdx(rowIdx))
  {
    HCell lastCell = m_Grid.Design().Cells.Null();
    for (int siteIdx = m_OriginColIdx; DoesColBelongToOptimalRegion(siteIdx); siteIdx++)
    {
      int trialCellWidthInSites = 0;

      HCell trialCell = m_Grid(rowIdx, siteIdx);
      //try to exchange with this cell
      if (!IsNull(trialCell) && trialCell != lastCell)
      {
        lastCell = trialCell;

        m_GroupSize = 1;
        m_ConsideredGroupOfCells[m_GroupSize] = trialCell;
        CurrentPositions[m_GroupSize] = m_Grid.CellColumn(trialCell);
        m_GroupSize = 2;
        
        trialCellWidthInSites = (int)m_Grid.CellSitesNum(trialCell);
        siteIdx += trialCellWidthInSites-1; //NOTE: optimization, -1 because FOR increment this counter
        int trialCellColIdx = m_Grid.CellColumn(trialCell);

        //REFACTOR: extract method CalcFreeSpaceNearCell
        int leftTrialFreeSpace  = 0;
        for (int k = trialCellColIdx - 1; IsColWithinBorders(k); k--)
        {
          if (::IsNull(m_Grid(rowIdx, k)))
          { 
            leftTrialFreeSpace++; 
          }
          else
          {
            break;
          }
        }

        //REFACTOR: extract method CalcFreeSpaceNearCell
        int rightTrialFreeSpace = 0;
        for (int k = trialCellColIdx + trialCellWidthInSites; IsColWithinBorders(k); k++)
        {
          if (::IsNull(m_Grid(rowIdx, k)))
          { 
            rightTrialFreeSpace++;
          }
          else
          {
            break;
          }
        }

        nAvailableSitesNearTrialCell = leftTrialFreeSpace + trialCellWidthInSites + rightTrialFreeSpace;

        //if enough space available, we perform exchange (we don't make shift)
        int neededSpace4Trial = 0;
        int neededSpace4Current = 0;
        int site4Current = 0;
        int site4Trial = 0;
        if (nAvailableSitesNearTrialCell >= m_CellWidthInSites && nAvailableSitesNearCurrentCell >= trialCellWidthInSites)
        {
          site4Current = trialCellColIdx;
          neededSpace4Current = m_CellWidthInSites - (trialCellWidthInSites + rightTrialFreeSpace);
          if (neededSpace4Current > 0)
          {
            site4Current -= neededSpace4Current;
            site4Current = std::max(0, site4Current);
          }
          site4Trial = currCellColIdx;
          neededSpace4Trial = trialCellWidthInSites - (m_CellWidthInSites + rightFreeSpace);
          if (neededSpace4Trial > 0)
          {
            site4Trial -= neededSpace4Trial;
            site4Trial = std::max(0, site4Trial);
          }
          //exchange cells
          m_Grid.PutCellFast(m_Cell, m_Grid.CellRow(trialCell), site4Current);
          m_Grid.PutCellFast(trialCell, m_Grid.CellRow(m_Cell), site4Trial);

          if (checker->CheckCriteria(m_ConsideredGroupOfCells, m_GroupSize))
          {
            //put cells to the better position found
            m_Grid.PutCell(trialCell, m_Grid.CellRow(m_Cell), site4Trial);
            m_Grid.PutCell(m_Cell, rowIdx, site4Current); //WARINING: do not use m_Grid.CellRow(trialCell) here, because it was rewritten
            Utils::CalculateHPWLDiff(m_Grid.Design(), m_ConsideredGroupOfCells, m_GroupSize, true);
            return true;
          }
          else
          {//rollback
            m_Grid.PutCellFast(m_Cell, m_Grid.CellRow(m_Cell), m_Grid.CellColumn(m_Cell));
            m_Grid.PutCellFast(trialCell, m_Grid.CellRow(trialCell), m_Grid.CellColumn(trialCell));
          }
        }
      }
    }
  }
  return false;
}

void GlobalSwapper::ShiftCoordinateToLowerBorder(int& coordinate, int distanceToBorder, int minCoordinate, int maxCoordinate)
{
  if (coordinate - distanceToBorder <= minCoordinate)
    coordinate = minCoordinate;
  else if (coordinate + distanceToBorder >= maxCoordinate)
    coordinate = (maxCoordinate - 1) - 2 * distanceToBorder;
  else
    coordinate -= distanceToBorder;
}

void GlobalSwapper::FindOptimalRegion()
{
  //find center of optimal region
  GetOptimalPosition(m_Grid, m_Cell, m_OriginRowIdx, m_OriginColIdx);
  
  if (m_Grid.Design().cfg.ValueOf(".tmp_drawOptimalSites", false))
  {//TODO: delete or accept this plotting
    m_Grid.Design().Plotter->DrawCircle(m_Grid.Design().Circuit.PlacementMinX() + m_OriginColIdx*m_Grid.SiteWidth(), 
      m_Grid.Design().Circuit.PlacementMinY() + m_OriginRowIdx*m_Grid.SiteHeight(), 4, Color_Green);
  }

  //shift from center to the lower left corner
  ShiftCoordinateToLowerBorder(m_OriginRowIdx, m_OptimalRegionHalfHeight, 0, m_Grid.NumRows());
  ShiftCoordinateToLowerBorder(m_OriginColIdx, m_OptimalRegionHalfWidth, 0, m_Grid.NumCols());
}

void GlobalSwapper::ProcessCell(HCell cell)
{
  m_Cell = cell;
  m_CellWidthInSites = (int)m_Grid.CellSitesNum(m_Cell);
  
  string cellName = m_Grid.Design().GetString<HCell::Name>(m_Cell).c_str();
      
  if (m_CellWidthInSites > OptimalRegionWidth())
    return; //we can't place cell at optimal region in any case

  //find the optimal region
  FindOptimalRegion();

  //skip cell if it already belongs to its optimal region
  if (IsAlreadyInOptimalRegion())
  {
    //ALERT("! %s\t", cellName.c_str()));
    return;
  }

  //try to move cell to its optimal region
  if (TryToMoveCellToOptimalRegion())
    return;

  //try to exchange it with a cell from the optimal region
  if (TryToExchangeWithCell())
  {
    //ALERT("* %s\t", cellName.c_str()));
    return;
  }

  //ALERT("# %s\t", cellName.c_str()));

  return;
}

void GlobalSwap(HDPGrid& grid, ConstraintsController* checker)
{
  ConfigContext ctx(grid.Design().cfg.OpenContext("GlobalSwap"));

  GlobalSwapper gswapper(grid, 
    grid.Design().cfg.ValueOf(".optimalRegionHalfWidth", 14),
    grid.Design().cfg.ValueOf(".optimalRegionHalfHeight", 2),
    checker);

  HCells::PlaceableCellsEnumeratorW pCell = grid.Design().Cells.GetPlaceableCellsEnumeratorW();
  for (; pCell.MoveNext();)
  {
    gswapper.ProcessCell(pCell);
    grid.Design().Plotter->AutoShowPlacement();
  }
}
