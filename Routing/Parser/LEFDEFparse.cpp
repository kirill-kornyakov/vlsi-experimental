#include "parser.h"

LEFParserData* ParseLEF(const char* filename);
int ParseDEF(const char* filename, Circuit& circuit);

void FindMainLayers(Circuit& circuit)
{
    list<RoutingLayerInfo*>::iterator rli = circuit.tech->Layers.begin();
    list<RoutingLayerInfo*>::iterator rle = circuit.tech->Layers.end();
    if(rli != rle)
    {
        ++rli;
        if(rli != rle)
        {
            if((*rli)->dir == RoutingLayerInfo::horizontal)
                circuit.hLayer = *rli;
            ++rli;
            if(rli != rle)
            {
                if((*rli)->dir == RoutingLayerInfo::vertical)
                    circuit.vLayer = *rli;
            }
        }
        if(circuit.vLayer == 0 || circuit.hLayer == 0)
            for(rli = circuit.tech->Layers.begin(); rli != rle; ++rli)
            {
                if((*rli)->dir == RoutingLayerInfo::horizontal && circuit.hLayer == 0)
                    circuit.hLayer = *rli;
                if((*rli)->dir == RoutingLayerInfo::vertical && circuit.vLayer == 0)
                    circuit.vLayer = *rli;
            }
    }
}

void ExtractRows(Circuit &circuit)
{
    double step = 16.0 * circuit.hLayer->Pitch;
    circuit.nRows = (int)(circuit.height / step);
    int numsites = (int)(circuit.width);
    double first = circuit.shiftX + (circuit.height - circuit.nRows * step) / 2.0;
    circuit.rows = new Row[circuit.nRows];
    for(int i = 0; i < circuit.nRows; i++, first+=step)
    {
        circuit.rows[i].coordinate = (int)first;
        circuit.rows[i].height = (int)step;
        circuit.rows[i].numSites = numsites;
        circuit.rows[i].siteorient[0] = 'N';
        circuit.rows[i].siteorient[1] = 0;
        circuit.rows[i].siteSpacing = 1;
        circuit.rows[i].sitesymm[0] = 'Y';
        circuit.rows[i].sitesymm[1] = 0;
        circuit.rows[i].siteWidth = 1;
        circuit.rows[i].subrowOrigin = 0;
    }
}

void StandardizeCells(Circuit& circuit)
{
    int h = circuit.rows[0].height;

    for(int i = 0; i < circuit.nNodes + circuit.nTerminals; i++)
    {
        if(circuit.nodes[i].width == 0.0) continue;
        circuit.nodes[i].width = (circuit.nodes[i].width * circuit.nodes[i].height) / h;
        circuit.nodes[i].height = h;
    }
}

void ShiftCoordinates(Circuit& circuit, bool tocenter)
{
    if(tocenter)
        for(int i =0; i < circuit.nNodes + circuit.nTerminals; i++)
        {
            circuit.placement[i].xCoord += circuit.nodes[i].width / 2.0;
            circuit.placement[i].yCoord += circuit.nodes[i].height / 2.0;
        }
    else
        for(int i =0; i < circuit.nNodes + circuit.nTerminals; i++)
        {
            circuit.placement[i].xCoord -= circuit.nodes[i].width / 2.0;
            circuit.placement[i].yCoord -= circuit.nodes[i].height / 2.0;
        }
}

int ParseLEFDEF(const char *lefname, const char* defname, Circuit& circuit)
{
    cout << "parsing " << lefname << endl;
    if((circuit.tech = ParseLEF(lefname)) == 0)
    {
        cout << "LEF parcer fails." << endl;
        exit(1);
    }

    FindMainLayers(circuit);

    cout << lefname << " parsed successfully" << endl;
    cout << "parsing " << defname << endl;
    int ret = ParseDEF(defname, circuit);
    if(ret != 0)
        cout << lefname << " parsed successfully" << endl;

    ExtractRows(circuit);

    StandardizeCells(circuit);

    ShiftCoordinates(circuit, true);

    return 0;
}
