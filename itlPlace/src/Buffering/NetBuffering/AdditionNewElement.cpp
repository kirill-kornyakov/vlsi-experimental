#include "AdditionNewElement.h"
#include "NetBufferingAlgorithm.h"
#include "VanGinnekenTree.h"
#include "BufferPositions.h"

AbstractAdditionNewElement::AbstractAdditionNewElement(NetBufferingAlgorithm* vGA)
{
    vGAlgorithm = vGA;    
}

StandartAdditionNewElement::StandartAdditionNewElement(NetBufferingAlgorithm* vGA): AbstractAdditionNewElement(vGA)
{
}

void StandartAdditionNewElement::InsertsBuffer(TemplateTypes<NewBuffer>::list& newBuffer, 
                                               VGVariantsListElement* best)
{
    for (TemplateTypes<BufferPositions>::list::iterator pos = best->GetBufferPosition()->begin(); 
        pos != best->GetBufferPosition()->end(); ++pos)
    {
        InsertBuffer(newBuffer, *pos);
    }
}

void  StandartAdditionNewElement::InsertBuffer(TemplateTypes<NewBuffer>::list& newBuffer, 
                                               BufferPositions& position)
{
    if (position.GetBufferInfo() == 0) return;

    HCellWrapper buffer = vGAlgorithm->data->design[vGAlgorithm->data->design.Cells.AllocateCell()];
    buffer.SetType(position.GetBufferInfo()->Type());
    buffer.SetPlacementStatus(PlacementStatus_Movable);
    buffer.SetHeight(vGAlgorithm->data->design.GetDouble<HMacroType::SizeY>(position.GetBufferInfo()->Type()));
    buffer.SetWidth(vGAlgorithm->data->design.GetDouble<HMacroType::SizeX>(position.GetBufferInfo()->Type()));

    char bufferName[32];
    sprintf(bufferName, "%s%d", vGAlgorithm->data->textIdentifierBuffer.c_str(), ::ToID(buffer));//TODO: create unique name
    buffer.SetName(bufferName);
    if ((vGAlgorithm->data->typePartition == LEGAL_POSITIONS_ONLY) 
        && position.GetPosition()->isCandidate() && 
        !position.GetPosition()->isCandidateAndRealPoint() && !position.GetPosition()->isInternal())
        buffer.SetX(position.GetPosition()->x);
    else
        buffer.SetX(position.GetPosition()->x - buffer.Width() * 0.5);

    if ((vGAlgorithm->data->typePartition == LEGAL_POSITIONS_ONLY) && position.GetPosition()->isCandidate() && 
        !position.GetPosition()->isCandidateAndRealPoint() && !position.GetPosition()->isInternal())
        buffer.SetY(position.GetPosition()->y);
    else
        buffer.SetY(position.GetPosition()->y - buffer.Height() * 0.5);

    buffer.SetOrientation(Orientation_N);
    vGAlgorithm->data->design.Pins.AllocatePins(buffer);
    //position.GetPosition()->GetTree()->pGrid->SetCell(buffer);
    NewBuffer buf;
    buf.Positions = position;
    buf.cell = buffer;
    buf.sink = Utils::FindCellPinByName(vGAlgorithm->data->design, buffer, (position.GetBufferInfo()->InPin(), 
        vGAlgorithm->data->design).Name());
    buf.source = Utils::FindCellPinByName(vGAlgorithm->data->design, buffer, 
        (position.GetBufferInfo()->OutPin(), vGAlgorithm->data->design).Name());
    newBuffer.push_back(buf);
}

void StandartAdditionNewElement::AddSinksToNet(HNet& subNet, VanGinnekenTreeNode* node, 
                                               HNetWrapper::PinsEnumeratorW& subNetPinEnumW,
                                               TemplateTypes<NewBuffer>::list& newBuffer)
{
    NewBuffer* bufferNumber = FindBufferNumberByIndex(node, newBuffer);

    if (bufferNumber != NULL)
    {
        HPin bufferInput = bufferNumber->sink;

        subNetPinEnumW.MoveNext();

        vGAlgorithm->data->design.Nets.AssignPin(subNet, subNetPinEnumW, bufferInput);

        HTimingPoint source = vGAlgorithm->data->design.TimingPoints[
            vGAlgorithm->data->design.Get<HNet::Source, HPin>(subNet)];

            HPin bufferOutput = bufferNumber->source;
            Utils::InsertNextPoint(vGAlgorithm->data->design, vGAlgorithm->data->design.TimingPoints[bufferInput], 
                source);
            Utils::InsertNextPoint(vGAlgorithm->data->design, vGAlgorithm->data->design.TimingPoints[bufferOutput], 
                vGAlgorithm->data->design.TimingPoints[bufferInput]);

            /*if (vGAlgorithm->data->typeNetListBuffering == PATH_BASED)
            {
            vGAlgorithm->data->design.CriticalPathPoints.SetPoints()
            }*/

            return;
    }

    if (node->isSteinerTreeLeaf())
    {		
        subNetPinEnumW.MoveNext();
        vGAlgorithm->data->design.Nets.AssignPin(subNet, subNetPinEnumW, 
            vGAlgorithm->data->design.SteinerPoints.Get<HSteinerPoint::Pin, HPin>(node->GetSteinerPoint()));
        return;
    }

    if (node->HasLeft())
    {
        AddSinksToNet(subNet, node->GetLeft(), subNetPinEnumW, newBuffer);
    }

    if (node->HasRight())
    {
        AddSinksToNet(subNet, node->GetRight(), subNetPinEnumW, newBuffer);
    }
}

NewBuffer* StandartAdditionNewElement::FindBufferNumberByIndex(VanGinnekenTreeNode* node, 
                                                               TemplateTypes<NewBuffer>::list& newBuffer)
{
    int i = 0;
    for (TemplateTypes<NewBuffer>::list::iterator pos = newBuffer.begin(); pos != newBuffer.end(); ++pos)
    {
        i++;
        if(pos->Positions.GetPosition() == node)
            return &*pos;
    }
    return NULL;
}

void StandartAdditionNewElement::PinsCountCalculation(VanGinnekenTreeNode* node, int& nPins, 
                                                      TemplateTypes<NewBuffer>::list& newBuffer)
{
    if ((FindBufferNumberByIndex(node, newBuffer) != NULL))
    {
        nPins++;
        return;
    }
    if (node->isSteinerTreeLeaf())
    {		
        nPins++;
        return;
    }
    if (node->HasLeft())
    {
        PinsCountCalculation(node->GetLeft(), nPins, newBuffer);
    }
    if (node->HasRight())
    {
        PinsCountCalculation(node->GetRight(), nPins, newBuffer);
    }
    return;
}

HSteinerPoint StandartAdditionNewElement::FindStartPoindInEdge(NewBuffer& buffer, bool& isRight, TemplateTypes<NewBuffer>::list& newBuffer)
{
    int i = buffer.Positions.GetPosition()->index; 
    bool isFindStartPoint = false;
    while ((i > buffer.Positions.GetPosition()->GetRoot()->index) && (!isFindStartPoint))
    {
        i--;
        if (buffer.Positions.GetPosition()->GetTree()->vGTreeNodeList[i]->isCandidate())
        {
            NewBuffer* nb = FindBufferNumberByIndex(buffer.Positions.GetPosition()->GetTree()->vGTreeNodeList[i], newBuffer);
            if (nb != NULL)
                return vGAlgorithm->data->design.SteinerPoints[nb->source];
        }
        else 
            isFindStartPoint = true;
    }
    int lastCandidateInEdge = i + 1;
    int j = i;
    while (j >= buffer.Positions.GetPosition()->GetRoot()->index) 
    {
        if (buffer.Positions.GetPosition()->GetTree()->vGTreeNodeList[j]->HasLeft())
            if (buffer.Positions.GetPosition()->GetTree()->vGTreeNodeList[j]->GetLeft()->index == lastCandidateInEdge) 
            {
                isRight = false;
                return buffer.Positions.GetPosition()->GetTree()->vGTreeNodeList[j]->GetSteinerPoint(buffer.Positions.GetPosition()->GetTree()->vGTreeNodeList[j]->type != SOURCE_AND_SINK);
            }
            if (buffer.Positions.GetPosition()->GetTree()->vGTreeNodeList[j]->HasRight())
                if (buffer.Positions.GetPosition()->GetTree()->vGTreeNodeList[j]->GetRight()->index == lastCandidateInEdge)
                {
                    isRight = true;
                    return buffer.Positions.GetPosition()->GetTree()->vGTreeNodeList[j]->GetSteinerPoint(buffer.Positions.GetPosition()->GetTree()->vGTreeNodeList[j]->type != SOURCE_AND_SINK);;
                }
                j--;
    }
    return vGAlgorithm->data->design.SteinerPoints.Null();
}


HSteinerPoint StandartAdditionNewElement::FindEndPoindInEdge(NewBuffer& buffer, TemplateTypes<NewBuffer>::list& newBuffer)
{
    int i = buffer.Positions.GetPosition()->index + 1;
    bool isFindEndPoint = false;
    while (!isFindEndPoint) 
    {
        if (!buffer.Positions.GetPosition()->GetTree()->vGTreeNodeList[i]->isCandidate()) 
            return buffer.Positions.GetPosition()->GetTree()->vGTreeNodeList[i]->GetSteinerPoint();
        NewBuffer* nb = FindBufferNumberByIndex(buffer.Positions.GetPosition()->GetTree()->vGTreeNodeList[i], newBuffer);
        if (nb != NULL)
            return vGAlgorithm->data->design.SteinerPoints[nb->sink];
        i++;
    }
    return vGAlgorithm->data->design.SteinerPoints.Null();
}

void StandartAdditionNewElement::AddPointToSteinerTree(NewBuffer& buffer, TemplateTypes<NewBuffer>::list& newBuffer)
{
    bool isRight = false;
    HSteinerPoint start = FindStartPoindInEdge(buffer, isRight, newBuffer);
    HSteinerPoint end = FindEndPoindInEdge(buffer, newBuffer); 

    if (isRight)
        vGAlgorithm->data->design.Set<HSteinerPoint::Right>(start, 
        vGAlgorithm->data->design.SteinerPoints[buffer.sink]);
    else
        vGAlgorithm->data->design.Set<HSteinerPoint::Left>(start, 
        vGAlgorithm->data->design.SteinerPoints[buffer.sink]);
    vGAlgorithm->data->design.SteinerPoints.InheritPinCoordinates(vGAlgorithm->data->design.SteinerPoints[buffer.sink]);

    vGAlgorithm->data->design.Set<HSteinerPoint::Left>(vGAlgorithm->data->design.SteinerPoints[buffer.source], end);
    vGAlgorithm->data->design.SteinerPoints.InheritPinCoordinates(vGAlgorithm->data->design.SteinerPoints[buffer.source]);
}

void StandartAdditionNewElement::CreateNets(HNet& net, TemplateTypes<NewBuffer>::list& newBuffer, 
                                            HNet* newNet, VanGinnekenTreeNode* node, int newNetCount)
{
    HNet currentNet = net;
    int ind = 0;
    TemplateTypes<NewBuffer>::list::iterator j = newBuffer.begin();
    int mustPinCount = int(newBuffer.size()) * 2;
    int newPinCount = 0;
    while (j != newBuffer.end())
    {
        int newNetNumb = 0;
        HNet cNet = j->Positions.GetPosition()->GetNet();
        int rootIndex =  j->Positions.GetPosition()->GetRoot()->index;
        if (cNet != currentNet)
        {
            currentNet = cNet;
            net = currentNet;
            node = j->Positions.GetPosition()->GetTree()->vGTreeNodeList[rootIndex]->GetLeft();
        }

        int pinCountInCurrentNet = vGAlgorithm->data->design.Nets.GetInt<HNet::PinsCount>(net);
        int bufferInInCurrentNet = 0;
        mustPinCount += pinCountInCurrentNet;


        int nNewNetPin = 0;
        int nPins = 0;
        char cellIdx[10];

        HNet subNet = vGAlgorithm->data->design.Nets.AllocateNet(false);
        newNet[ind] = subNet;
        vGAlgorithm->data->design.Nets.Set<HNet::Kind, NetKind>(subNet, NetKind_Active);
        sprintf(cellIdx, "%d", newNetNumb);
        vGAlgorithm->data->design.Nets.Set<HNet::Name>(subNet, vGAlgorithm->data->design[net].Name() + 
            vGAlgorithm->data->textIdentifierBufferedNet + string(cellIdx));

        string ssss = vGAlgorithm->data->design[net].Name() + 
            vGAlgorithm->data->textIdentifierBufferedNet + string(cellIdx);

        PinsCountCalculation(node, nPins, newBuffer);
        nPins++;
        nNewNetPin += nPins;
        vGAlgorithm->data->design.Nets.AllocatePins(subNet, nPins);  
        newPinCount += nPins;
        vGAlgorithm->data->design.Nets.Set<HNet::Source, HPin>(subNet, vGAlgorithm->data->design[net].Source());

        AddPointToSteinerTree(*j, newBuffer);
        HNetWrapper::PinsEnumeratorW enumerator = vGAlgorithm->data->design[subNet].GetSinksEnumeratorW();
        AddSinksToNet(subNet, node, enumerator, newBuffer);

        if (vGAlgorithm->data->plotNets)
        {
            vGAlgorithm->data->design.Plotter->ShowNetSteinerTree(subNet, Color_Red, true, 
                HPlotter::WaitTime(vGAlgorithm->data->plotterWaitTime));
        }


        //for (TemplateTypes<NewBuffer>::list::iterator j = newBuffer.begin(); j != newBuffer.end(); ++j, ind++)
        bool isStop = false;
        ind++;
        while (!isStop) 
        {
            newNetNumb++;
            bufferInInCurrentNet++;
            subNet = vGAlgorithm->data->design.Nets.AllocateNet(false);
            newNet[ind] = subNet;
            vGAlgorithm->data->design.Nets.Set<HNet::Kind, NetKind>(subNet, NetKind_Active);
            sprintf(cellIdx, "%d", newNetNumb);
            vGAlgorithm->data->design.Nets.Set<HNet::Name>(subNet, vGAlgorithm->data->design[net].Name() + 
                vGAlgorithm->data->textIdentifierBufferedNet + string(cellIdx));
            string sss = vGAlgorithm->data->design[net].Name() + 
                vGAlgorithm->data->textIdentifierBufferedNet + string(cellIdx);

            nPins = 0;
            NewBuffer& nodeStart = *j;
            PinsCountCalculation(nodeStart.Positions.GetPosition()->GetLeft(), nPins, newBuffer);
            nPins++;
            nNewNetPin += nPins;
            newPinCount += nPins;
            vGAlgorithm->data->design.Nets.AllocatePins(subNet, nPins);
            vGAlgorithm->data->design.Nets.Set<HNet::Source, HPin>(subNet, j->source);

            NewBuffer& nodeStart2 = *j;
            AddPointToSteinerTree(*j, newBuffer);
            HNetWrapper::PinsEnumeratorW enumerator = vGAlgorithm->data->design[subNet].GetSinksEnumeratorW();
            AddSinksToNet(subNet, nodeStart2.Positions.GetPosition()->GetLeft(), enumerator, newBuffer);

            if (vGAlgorithm->data->plotNets)
            {
                vGAlgorithm->data->design.Plotter->ShowNetSteinerTree(subNet, Color_Red, false, 
                    HPlotter::WaitTime(vGAlgorithm->data->plotterWaitTime));
            }
            j++;
            if (j == newBuffer.end())
            {
                isStop = true;
                vGAlgorithm->data->design.Nets.Set<HNet::Kind, NetKind>(net, NetKind_Buffered);
                ASSERT(nNewNetPin == (pinCountInCurrentNet + bufferInInCurrentNet * 2));
            }
            else
                if (currentNet != j->Positions.GetPosition()->GetNet())
                {
                    isStop = true; 
                    vGAlgorithm->data->design.Nets.Set<HNet::Kind, NetKind>(net, NetKind_Buffered);
                    ASSERT(nNewNetPin == (pinCountInCurrentNet + bufferInInCurrentNet * 2));
                }
                ind++;
        }
    }
    ASSERT(mustPinCount == newPinCount);
}

LegalAdditionNewElement::LegalAdditionNewElement(NetBufferingAlgorithm* vGA): StandartAdditionNewElement(vGA)
{
}
void LegalAdditionNewElement::InsertBuffer(TemplateTypes<NewBuffer>::list& newBuffer, 
                                           BufferPositions& position)
{
    if (position.GetBufferInfo() == 0) return;

    HCellWrapper buffer = vGAlgorithm->data->design[vGAlgorithm->data->design.Cells.AllocateCell()];
    buffer.SetType(position.GetBufferInfo()->Type());
    buffer.SetPlacementStatus(PlacementStatus_Movable);
    buffer.SetHeight(vGAlgorithm->data->design.GetDouble<HMacroType::SizeY>(position.GetBufferInfo()->Type()));
    buffer.SetWidth(vGAlgorithm->data->design.GetDouble<HMacroType::SizeX>(position.GetBufferInfo()->Type()));

    char bufferName[32];
    sprintf(bufferName, "buf_%d", ::ToID(buffer));//TODO: create unique name
    buffer.SetName(bufferName);

    double x = 0;
    if ((vGAlgorithm->data->typePartition == LEGAL_POSITIONS_ONLY) && position.GetPosition()->isCandidate() && 
        !position.GetPosition()->isCandidateAndRealPoint() && !position.GetPosition()->isInternal())
        x = position.GetPosition()->x;
    else
        x = position.GetPosition()->x - buffer.Width() * 0.5;
    int column = position.GetPosition()->GetTree()->pGrid->GetColumn(x);

    double y = 0;
    if ((vGAlgorithm->data->typePartition == LEGAL_POSITIONS_ONLY) && position.GetPosition()->isCandidate() && 
        !position.GetPosition()->isCandidateAndRealPoint() && !position.GetPosition()->isInternal())
        y = position.GetPosition()->y;
    else
        y = position.GetPosition()->y - buffer.Height() * 0.5;
    int row = position.GetPosition()->GetTree()->pGrid->GetRow(y);

    double newX = position.GetPosition()->GetTree()->pGrid->GetNode(row, column)->GetX();
    double newY = position.GetPosition()->GetTree()->pGrid->GetNode(row, column)->GetY();
    buffer.SetX(newX);
    buffer.SetY(newY);

    buffer.SetOrientation(Orientation_N);
    vGAlgorithm->data->design.Pins.AllocatePins(buffer);

    int interseptCount = 0;
    double x1 = newX, y1 = newY, 
        x2 = newX + buffer.Width(), y2 =  newY,
        x3 = newX, y3 = newY + buffer.Height(),
        x4 = newX + buffer.Width(), y4 = newY + buffer.Height();
    double  bufferWidth =  buffer.Width();

    int rowBegin = position.GetPosition()->GetTree()->pGrid->GetRow(y1);
    int rowEnd = position.GetPosition()->GetTree()->pGrid->GetRow(y3);
    int columnBegin = position.GetPosition()->GetTree()->pGrid->GetColumn(x1);
    int columnEnd = position.GetPosition()->GetTree()->pGrid->GetColumn(x4);
    HCell* intersept = new HCell [abs(rowEnd - rowBegin) * abs(columnEnd - columnBegin)];
    position.GetPosition()->GetTree()->pGrid->CellInZone(newX, newY, buffer.Width(), buffer.Height(), interseptCount, intersept);
    double leftShift = 0;
    double leftRight = 0;
    for (int i = 0; i < interseptCount; i++)
    {
        HCellWrapper cw = vGAlgorithm->data->design[intersept[i]];
        position.GetPosition()->GetTree()->pGrid->ExtractCell(cw);
        double cwx = cw.X(), cwy = cw.Y();
        double cwWidth = cw.Width();
        if (cw.X() < (newX + buffer.Width() / 2.0))
        {
            if (cw.X() > newX) 
                cw.SetX(newX - cw.Width());
        }
        else
        {
            cw.SetX(newX + buffer.Width());
        }
        position.GetPosition()->GetTree()->pGrid->SetCell(cw);
    }
    if (interseptCount > 0)
        delete [] intersept;

    position.GetPosition()->GetTree()->pGrid->SetCell(buffer);
    NewBuffer buf;
    buf.Positions = position;
    buf.cell = buffer;
    buf.sink = Utils::FindCellPinByName(vGAlgorithm->data->design, buffer, (position.GetBufferInfo()->InPin(), 
        vGAlgorithm->data->design).Name());
    buf.source = Utils::FindCellPinByName(vGAlgorithm->data->design, buffer, 
        (position.GetBufferInfo()->OutPin(), vGAlgorithm->data->design).Name());
    newBuffer.push_back(buf);
}
