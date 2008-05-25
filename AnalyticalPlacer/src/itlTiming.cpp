#include <stack>
#include <queue>
#include "../include/itlTiming.h"
#include "../include/RouteTrees.h"
#include <assert.h>
#include "../include/global.h"

//#define EXTRACT_RC(circuit,src,dest,R,C) ExtractRCexact(circuit,src,dest,R,C);
#define EXTRACT_RC(circuit,src,dest,R,C) ExtractRClumped(circuit,src,dest,R,C);


inline void ExtractRCexact(Circuit& c, StNodeEx* src, StNodeEx* dest, double& R, double &C)
{
    if(src->x == dest->x)
    {
        double L = src->y - dest->y;
        if(L < 0) L = -L;
        R = L * c.vLayer->RPerDist;
        C = L * c.vLayer->CPerDist + 2.0 * (L + c.vLayer->Width) * c.vLayer->EdgeCap;
    }
    else
    {
        double L = src->x - dest->x;
        if(L < 0) L = -L;
        R = L * c.hLayer->RPerDist;
        C = L * c.hLayer->CPerDist + 2.0 * (L + c.hLayer->Width) * c.hLayer->EdgeCap;
    }
}

inline void ExtractRClumped(Circuit& c, StNodeEx* src, StNodeEx* dest, double& R, double &C)
{
    double L = (src->x == dest->x) ? (src->y - dest->y) : (src->x - dest->x);
    if(L < 0) L = -L;
    R = L * c.hLayer->RPerDist;
    C = L * (c.hLayer->CPerDist + 2.0 * c.hLayer->EdgeCap);
}

void HPWLTiming(Circuit& circuit, Net& net)
{
    double R,C;
    StNodeEx first, second;
    first.x = second.x = 0.0;
    second.y = 0.0;
    first.y = net.tree->wl;
    EXTRACT_RC(circuit,&first,&second,R,C);
    int sourceInd = HASREALSOURCE(net) ? net.sourceIdx : 0;
    double totalC = 0.0;
    double Tbase = net.tree->srcRes * net.arrPins[sourceInd].routeInfo->cap
        + (net.tree->srcRes + R / 2.0) * C;
    for(int i = 0; i < net.numOfPins; i++)
        if(i != sourceInd)
        {
            net.arrPins[i].routeInfo->ExtractedC = C;
            net.arrPins[i].routeInfo->ExtractedR = R;
            totalC += net.arrPins[i].routeInfo->ObservedC = net.arrPins[i].routeInfo->cap;
            net.arrPins[i].routeInfo->ArrivalTime = Tbase
                + (net.tree->srcRes + R) * net.arrPins[i].routeInfo->ObservedC;
        }
        totalC += (net.numOfPins > 2) ? 2.0 * C : C;
    net.arrPins[sourceInd].routeInfo->ObservedC = totalC;
}

void ATreeTiming(Circuit& circuit, Net& Net)
{
    std::stack<StNodeEx*> NodeStack;
    StNodeEx* cur = Net.tree->nodes;
    cur->ObservedC = cur->cap;
    StNodeEx* next = cur;

    while(true)
    {
        while(next == cur && cur->left)
        {
            next = (StNodeEx*)cur->left;
            next->ObservedC = next->cap;
            EXTRACT_RC(circuit, cur, next, next->ExtractedR, next->ExtractedC);
            NodeStack.push(cur);
            cur = next;
        }
        if(next == cur && cur->right)
        {
            next = (StNodeEx*)cur->right;
            next->ObservedC = next->cap;
            EXTRACT_RC(circuit, cur, next, next->ExtractedR, next->ExtractedC);
            NodeStack.push(cur);
            cur = next;
            continue;
        }
        else
        {
            if(NodeStack.empty()) break;
            next = cur;
            cur = NodeStack.top();
            cur->ObservedC += next->ObservedC + next->ExtractedC;
            if(next == cur->left && cur->right)
            {
                next = (StNodeEx*)cur->right;
                next->ObservedC = next->cap;
                EXTRACT_RC(circuit, cur, next, next->ExtractedR, next->ExtractedC);
                cur = next;
                continue;
            }
            else
                NodeStack.pop();
        }
    }

    cur = Net.tree->nodes;
    cur->ArrivalTime = Net.tree->srcRes * cur->ObservedC;
    while(true)
    {
        while(cur->left)
        {
            next = (StNodeEx*)cur->left;
            if(cur->right)
                NodeStack.push(cur);
            next->ArrivalTime = cur->ArrivalTime + next->ExtractedR * (next->ExtractedC * 0.5 + next->ObservedC);
            cur = next;
        }
        if(cur->right)
        {
            next = (StNodeEx*)cur->right;
            next->ArrivalTime = cur->ArrivalTime + next->ExtractedR * (next->ExtractedC * 0.5 + next->ObservedC);
            cur = next;
            continue;
        }
        else
        {
            if(NodeStack.empty())
                break;
            cur = NodeStack.top();
            NodeStack.pop();
            next = (StNodeEx*)cur->right;
            next->ArrivalTime = cur->ArrivalTime + next->ExtractedR * (next->ExtractedC * 0.5 + next->ObservedC);
            cur = next;
        }
    }
}

void CalculateDalays(Circuit& circuit, Net& Net)
{
    if(Net.tree->RoutingType & A_TREE)
        ATreeTiming(circuit,Net);
    else
        if(Net.tree->RoutingType & HPWL_STUB)
            HPWLTiming(circuit,Net);
}

void PrintPrev(Circuit& c, int nodeID)
{
    cout << "---------" << endl;
    cout << c.tableOfNames[nodeID].name << "   " << nodeID << endl;
    cout << "---------" << endl;
    for(size_t i = 0; i < c.tableOfConnections[nodeID].size(); i++)
    {
        int netID = c.tableOfConnections[nodeID][i];
        cout << "Net " << netID << " - " << c.nets[netID].name << endl;
        for(int j = 0; j < c.nets[netID].numOfPins; j++)
            if(c.nets[netID].arrPins[j].chtype == 'O')
                cout << "Source: " << c.nets[netID].arrPins[j].cellIdx 
                << " - "  << c.tableOfNames[c.nets[netID].arrPins[j].cellIdx].name << endl
                << "---------" << endl;
    }
}

//////////////////////////////////////////////////////////////////////////////////////////////////////

void MakeTimingLists(Circuit& c)
{
    int* netDegrees = new int[c.nNets];
    int* netDegrees_2 = new int[c.nNets];
    int* nodeDegrees = new int[c.Shift_ + c.nTerminals];
    for(int q = 0; q < c.Shift_ + c.nTerminals; q++)
        nodeDegrees[q] = 0;
    for(int i = 0; i < c.nNets; i++)
    {
        netDegrees[i] = 0;
        for(int l = 0; l < c.nets[i].numOfPins; l++)
            if(c.nets[i].arrPins[l].chtype != 'O')
                netDegrees[i]++;
        if(HASREALSOURCE(c.nets[i]))
            nodeDegrees[c.nets[i].arrPins[c.nets[i].sourceIdx].cellIdx]++;
    }
    for(int p = 0; p < c.nNets; p++)
        netDegrees_2[p] = netDegrees[p];

    Net __top;
    Net* current = &__top;

    //propagate by required order
    //inverced list - is arrival order
    std::queue<int> readyNodes;
    std::queue<int> readyNets;

    for(int k = 0; k < c.nPrimaryOutputs; k++)
        readyNodes.push(c.primaryOutputs[k]);

    while(!readyNodes.empty() || !readyNets.empty())
    {
        while(!readyNodes.empty())
        {
            int nodeIndex = readyNodes.front();
            readyNodes.pop();
            //cout << c.tableOfNames[nodeIndex].name << " - " << nodeIndex << endl;
            for(size_t j = 0; j < c.tableOfConnections[nodeIndex].size(); j++)
            {
                int netID = c.tableOfConnections[nodeIndex][j];
                if(c.nets[netID].arrPins[c.nets[netID].sourceIdx].cellIdx != nodeIndex)
                    if(!(netDegrees[netID]-=1))
                        readyNets.push(netID);
            }
        }
        while(!readyNets.empty())
        {
            int netIndex = readyNets.front();
            readyNets.pop();
            if(HASREALSOURCE(c.nets[netIndex]))
            {//completely ignore nets without real source
                if(!(nodeDegrees[c.nets[netIndex].arrPins[c.nets[netIndex].sourceIdx].cellIdx]-=1))
                    readyNodes.push(c.nets[netIndex].arrPins[c.nets[netIndex].sourceIdx].cellIdx);

                current->requiredOrder = c.nets + netIndex;
                current->requiredOrder->arrivalOrder = current;
                current = current->requiredOrder;
            }
        }
        if(readyNodes.empty())
        {
            for(int j = 0; j < c.nNets; j++)
                if(netDegrees[j] > 0 && netDegrees[j] < netDegrees_2[j])
                {
                    netDegrees[j] = 0;
                    readyNets.push(j);
                    break;
                }
        }
    }

    c.firstRequired = __top.requiredOrder;
    c.firstArrival = current;
    c.firstArrival->requiredOrder = 0;
    c.firstRequired->arrivalOrder = 0;

    delete[] netDegrees;
    delete[] nodeDegrees;
    delete[] netDegrees_2;
}

void PropagateArrivalTime(Circuit& circuit, bool reanalize, bool reroute)
{
    const double minimalArrival = 0.0;

    for(int i = 0; i < circuit.nNodes; i++)
        circuit.placement[i].arrivalTime = minimalArrival;
    for(int j = 0; j < circuit.nPrimaryOutputs; j++)//time on primary inputs predefined
        circuit.placement[circuit.primaryOutputs[j]].arrivalTime = minimalArrival;
    
    for(Net* current = circuit.firstArrival; current; current = current->arrivalOrder)
    {
        if(reroute) AdaptiveRoute(circuit,*current);
        if(reanalize) CalculateDalays(circuit,*current);

        assert(HASREALSOURCE((*current)));

        double base = circuit.placement[current->arrPins[current->sourceIdx].cellIdx].arrivalTime;
        for(int h = 0; h < current->numOfPins; h++)
        {
            if(h == current->sourceIdx) continue;
            double time = base + current->arrPins[h].routeInfo->ArrivalTime;
            if(circuit.placement[current->arrPins[h].cellIdx].arrivalTime < time)
                circuit.placement[current->arrPins[h].cellIdx].arrivalTime = time;
        }
    }
}

void PropagateRequiredTime(Circuit& circuit, bool reanalize, bool reroute)
{
    const double maximumRequired = 0.0;

    for(int i = 0; i < circuit.nNodes; i++)
        circuit.placement[i].requiredTime = maximumRequired;
    for(int j = 0; j < circuit.nPrimaryInputs; j++)//time on primary outputs predefined
        circuit.placement[circuit.primaryInputs[j]].requiredTime = maximumRequired;
    
    for(Net* current = circuit.firstRequired; current; current = current->requiredOrder)
    {
        if(reroute) AdaptiveRoute(circuit,*current);
        if(reanalize) CalculateDalays(circuit,*current);

        assert(HASREALSOURCE((*current)));

        double minTime = maximumRequired;
        for(int h = 0; h < current->numOfPins; h++)
        {
            if(h == current->sourceIdx) continue;
            double req = circuit.placement[current->arrPins[h].cellIdx].requiredTime 
                - current->arrPins[h].routeInfo->ArrivalTime;
            if(req < minTime) minTime = req;
        }
        if(circuit.placement[current->arrPins[current->sourceIdx].cellIdx].requiredTime > minTime)
            circuit.placement[current->arrPins[current->sourceIdx].cellIdx].requiredTime = minTime;
    }
}

void PrintCircuitArrivals(Circuit& circuit)
{
    cout << "Arrival times on primary outputs:" << endl;
    for(int i = 0; i < circuit.nPrimaryOutputs; i++)
        cout << circuit.tableOfNames[circuit.primaryOutputs[i]].name << "\t"
        << circuit.placement[circuit.primaryOutputs[i]].arrivalTime << endl;
}

void PrintCircuitRequireds(Circuit& circuit)
{
    cout << "Required times on primary inputs:" << endl;
    for(int i = 0; i < circuit.nPrimaryInputs; i++)
        cout << circuit.tableOfNames[circuit.primaryInputs[i]].name << "\t"
        << circuit.placement[circuit.primaryInputs[i]].requiredTime << endl;
}

void PrintTimingReport(Circuit& circuit, char* filename)
{
    FILE* out = fopen(filename,"w");
    int shift_ = (int)(circuit.terminals - circuit.nodes);
    for(int i = 0; i < shift_ + circuit.nTerminals; i++)
    {
        if(i == circuit.nNodes) i = shift_;
        fprintf(out,"%s - %d\n",circuit.tableOfNames[i].name,i);
        fprintf(out,"\tarrival:  %8.3f\n",circuit.placement[i].arrivalTime);
        fprintf(out,"\trequired: %8.3f\n",circuit.placement[i].requiredTime);
    }
    fclose(out);
}

void PrintNets(Circuit& circuit, char* filename)
{
    FILE* out = fopen(filename,"w");
    for(int i = 0; i < circuit.nNets; i++)
    {
        fprintf(out,"%s - %d\n",circuit.nets[i].name,i);
        for(int j = 0; j < circuit.nets[i].numOfPins; j++)
        {
            fprintf(out,"\t%2s - %c - %d\n",circuit.nets[i].arrPins[j].type->Name.c_str(),circuit.nets[i].arrPins[j].chtype,circuit.nets[i].arrPins[j].cellIdx);
        }
    }
    fclose(out);
}

void PrintRequiredOrder(Circuit& circuit)
{
    Net* cur = circuit.firstRequired;
    while(cur)
    {
        cout << cur->name << " - " << (int)(cur - circuit.nets) << endl;
        cur = cur->requiredOrder;
    }
}

void PrintArrivalOrder(Circuit& circuit)
{
    Net* cur = circuit.firstArrival;
    while(cur)
    {
        cout << cur->name << " - " << (int)(cur - circuit.nets) << endl;
        cur = cur->arrivalOrder;
    }
}

void ComputeNetWeights(Circuit& circuit)
{
  if (!gOptions.isLEFDEFinput)
  {
    cout << "Skipping computing net-weights step\n";
    return;
  }
  MakeTimingLists(circuit);
  //cout << "initialize success" << endl;
  PropagateArrivalTime(circuit,true,true);
  //cout << "arrival times calculated" << endl;
  PrintCircuitArrivals(circuit);
  PropagateRequiredTime(circuit,false,false);
  //cout << "required times calculated" << endl;

  double* nodesSlack = new double[circuit.Shift_ + circuit.nTerminals];
  double sumNodesSlack; // ����� ������ ����� � ����� ����
  double maxNodeSlack = -1e10;
  int nodeIdx;

  for (int i = 0; i < circuit.Shift_ + circuit.nTerminals; ++i)
  {
    nodesSlack[i] = circuit.placement[i].arrivalTime - circuit.placement[i].requiredTime;
    if (maxNodeSlack < nodesSlack[i])
    {
      maxNodeSlack = nodesSlack[i];
    }
  }

  for (int i = 0; i < circuit.nNets; ++i)
  {
    circuit.netWeights[i] = 1.0;
    sumNodesSlack = 0.0;
    
    for (int j = 0; j < circuit.nets[i].numOfPins; ++j)
    {
      nodeIdx = circuit.nets[i].arrPins[j].cellIdx;
      //if (nodeIdx >= circuit.Shift_)
      sumNodesSlack += nodesSlack[nodeIdx];
    }

    circuit.netWeights[i] += sumNodesSlack / maxNodeSlack;
  }

  delete[] nodesSlack;
}