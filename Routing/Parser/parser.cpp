/* 
 * parser.cpp
 * this is a part of itlDragon
 * Copyright (C) 2005, ITLab, Aleksey Bader, Zhivoderov
 * email: bader@newmail.ru
 * email: woody-quaker@yandex.ru
 * last modified 30 March 2006
 */

#include "parser.h"
#include <iostream>

#define fileNum 5   // number of files in aux file

str* table;
strExtend* newTable;
static int numOfNT;

/*
  Deletes spaces, tabulation, carriage return and line feed from the 
  beginning of a stream.
*/

int findName(str* table, char* name)
{
  int i;
  for(i = 0; i < numOfNT; ++i)
  {
    if (!strcmp(table[i].name,name))
      return i;
  }
  return -1;
}

int findNameBS(str* A, int Lb, int Ub, char* Key)
{  
  int M = 0;
  while (1)
  {
    M = (Lb + Ub)/2;
    if (strcmp(Key, A[M].name) < 0) 
      Ub = M - 1;
    else if (strcmp(Key, A[M].name) > 0) Lb = M + 1;
    else return M;
    if (Lb > Ub) return -1;
  }
}

int findNameBS(strExtend* A, int Lb, int Ub, char* Key)
{
  int M = 0;
  while (1)
  {
    M = (Lb + Ub)/2;
    if (strcmp(Key, A[M].name) < 0) 
      Ub = M - 1;
    else if (strcmp(Key, A[M].name) > 0) Lb = M + 1;
    else return M;
    if (Lb > Ub) return -1;
  }
}

void QSortStrExtend(strExtend* a, long N)
{
  long i = 0, j = N;
  strExtend temp, p;
  int rand0 = rand() % N;
  //GetIntegerRandomNumbers( &rand0, 1, 0, N);

  p.cellIdx = a[rand0].cellIdx;
  strcpy(p.name, a[rand0].name);

  do
  {
    while ( strcmp(a[i].name, p.name) < 0 ) i++;
    while ( strcmp(a[j].name, p.name) > 0 ) j--;

    if (i <= j)
    {
      strcpy(temp.name, a[i].name);
      strcpy(a[i].name, a[j].name);
      strcpy(a[j].name, temp.name);

      temp.cellIdx = a[i].cellIdx; 
      a[i].cellIdx = a[j].cellIdx; 
      a[j].cellIdx = temp.cellIdx;

      i++; j--;
    }
  } while ( i<=j );

  if ( j > 0 ) QSortStrExtend(a, j);
  if ( N > i ) QSortStrExtend(a+i, N-i);
}

void delFSpaces(std::ifstream& cin)
{
  char curr = cin.peek();
  while((curr == ' ') || (curr == '\t'))
  {
    cin.get();
    curr = cin.peek();
  }
  if (curr == '\n')
  {
    cin.get();
    delFSpaces(cin);
  }
  if (curr == '\r')
  {
    cin.get();
    delFSpaces(cin);
  }
}

/*
  Deletes comments from the beginning of a stream.
*/
void delComments(std::ifstream& cin)
{
  char curr = cin.peek();
  while(curr == '#')
  {
    while(curr != '\n') curr = cin.get();
    curr = cin.peek();
  }
}

/*
  Deletes comments from the beginning of a stream.
*/
void delCommentsAndSpaces(std::ifstream& cin)
{
  bool found = false;   // have found something distinct from the comment
  // or a space
  while (!found)
  {
    delFSpaces(cin);
    delComments(cin);
    if ((cin.peek() != '#') && (cin.peek() != ' ') && 
      (cin.peek() != '\n') && (cin.peek() != '\r'))
      found = true;
  }
}

int ParseAux(const char* fileName, Circuit& circuit)
{
  if (fileName == 0)   // file not found
  {
    cout << "Error: null pointer on aux file\n";
    return 1;
  }
  // we put in a variable dir a way up to a file
  char dirName[256];
  const char *auxFileDirEnd=strrchr(fileName,'\\');
  if (auxFileDirEnd)
  {
    strncpy(dirName,fileName,auxFileDirEnd-fileName+1);
    dirName[auxFileDirEnd-fileName+1]=0;
  }
  else strcpy(dirName,"");
  std::ifstream auxFile(fileName);
  char * newNets   = 0;
  char * newNodes  = 0;
  char * newWts  = 0;
  char * newPl   = 0;
  char * newScl   = 0;
  char * newLEF   = 0;
  char * newDEF   = 0;
  char word1[100], word2;

  if (auxFile) // while all ok - the file exists
  {
    cout << "parsing aux file...\n";
    char curr = auxFile.peek();
    delCommentsAndSpaces(auxFile);
    auxFile >> word1;
    delCommentsAndSpaces(auxFile);
    auxFile >> word2;
    if (word2 != ':')
    {
      cout << "Bad format aux file!";
      return 1;
    }
    delCommentsAndSpaces(auxFile);
    int i;
    char fileNames[fileNum][100];
    char tempStr[100];
    for (i = 0; i < fileNum; ++i)
    {
      auxFile >> tempStr;
      sprintf(fileNames[i],dirName);
      strcat(fileNames[i],tempStr);
      delCommentsAndSpaces(auxFile);
    }
    for(int j=0; j < fileNum; ++j)
    {
      if (strstr(fileNames[j],".nets")) newNets =fileNames[j];
      else if (strstr(fileNames[j],".NETS")) newNets =fileNames[j];
      else if (strstr(fileNames[j],".nodes"))newNodes=fileNames[j];
      else if (strstr(fileNames[j],".NODES"))newNodes=fileNames[j];
      else if (strstr(fileNames[j],".wts"))  newWts  =fileNames[j];
      else if (strstr(fileNames[j],".WTS"))  newWts  =fileNames[j];
      else if (strstr(fileNames[j],".pl"))   newPl =fileNames[j];
      else if (strstr(fileNames[j],".PL"))   newPl =fileNames[j];
      else if (strstr(fileNames[j],".scl"))  newScl =fileNames[j];
      else if (strstr(fileNames[j],".SCL"))  newScl =fileNames[j];
      else if (strstr(fileNames[j],".lef"))   newLEF =fileNames[j];
      else if (strstr(fileNames[j],".LEF"))   newLEF =fileNames[j];
      else if (strstr(fileNames[j],".def"))  newDEF =fileNames[j];
      else if (strstr(fileNames[j],".DEF"))  newDEF =fileNames[j];
    }
    int res = 0;

    if(newLEF && newDEF)
    {
        gOptions.isLEFDEFinput = true;
        ParseLEFDEF(newLEF, newDEF, circuit);
    }
    else
    {
        if (newNodes)
        {
          res += ParseNodes(newNodes, circuit);
        }
        int numOfNaT = circuit.nNodes + circuit.nTerminals;
        if (newNets)
        {
          res += ParseNets(newNets, circuit);
        }
        if (newWts)
        {
          res += ParseWts(newWts, circuit);
        }
        if (newPl)
        {
          if (gOptions.plName[0] != '\0')
            newPl = gOptions.plName;
          res += ParsePl(newPl, circuit);
        }
        if (newScl)
        {
          res += ParseScl(newScl, circuit);
        }
        if (res)
        {
          cout << "The execution of programm was stopped.\n";
          return 1;
        }
    }
  }
  else
  {
    cout << "Aux file not found\n";
    return 1;
  }
  return 0;
}


int ParseNodes(const char* fileName, Circuit& circuit)
{
  if (fileName == 0)
  {
    cout << "Parser error: null pointer on nodes file";
    return 1;
  }
  cout << "parsing nodes file...\n";
  std::ifstream nodesFile(fileName);
  delCommentsAndSpaces(nodesFile);
  char curr[100];
  char temp;
  nodesFile >> curr;
  if (!strcmp(curr,"UCLA"))
  {
    temp = nodesFile.get();
    while(temp != '\n')
    {
      temp = nodesFile.get();
    }
  }
  delCommentsAndSpaces(nodesFile);
  nodesFile >> curr;
  while (!(strcmp(curr,"NumNodes")) || !(strcmp(curr,"NumTerminals")))
  {
    delCommentsAndSpaces(nodesFile);
    nodesFile >> temp;
    if (temp != ':')
    {
      cout << "Bad nodes file.\n";
      return 1;
    }
    int num;
    nodesFile >> num;
    if (!strcmp(curr,"NumNodes"))
    {
      circuit.nNodes = num;
      circuit.tableOfNames = new str[circuit.nNodes];
    }
    if (!strcmp(curr,"NumTerminals"))
    {
      circuit.nTerminals = num;
      circuit.terminals = new Node[circuit.nTerminals];
    }
    delCommentsAndSpaces(nodesFile);
    nodesFile >> curr;
  }
  circuit.nNodes -= circuit.nTerminals;
  circuit.nodes = new Node[circuit.nNodes];
  int termCount = 0, nodesCount = 0;
  char tmp_name[20];
  float tmp_width, tmp_height;
  strcpy(tmp_name,curr);
  nodesFile >> tmp_width;
  delFSpaces(nodesFile);
  nodesFile >> tmp_height;
  delCommentsAndSpaces(nodesFile);
  nodesFile >> curr;
  if (!strcmp(curr,"terminal"))
  {
    circuit.terminals[termCount].height = static_cast<int>(tmp_height);
    circuit.terminals[termCount].width = static_cast<int>(tmp_width);
    strcpy(circuit.tableOfNames[circuit.nNodes+termCount].name, tmp_name);
    ++termCount;
    delCommentsAndSpaces(nodesFile);
    nodesFile >> curr;
  }
  else
  {
    circuit.nodes[nodesCount].height = static_cast<int>(tmp_height);
    circuit.nodes[nodesCount].width = static_cast<int>(tmp_width);
    strcpy(circuit.tableOfNames[nodesCount].name, tmp_name);
    ++nodesCount;
  }
  int i;
  numOfNT = circuit.nNodes+circuit.nTerminals;
  for(i = 1; i < numOfNT; ++i)
  {
    strcpy(tmp_name,curr);
    nodesFile >> tmp_width;
    delFSpaces(nodesFile);
    nodesFile >> tmp_height;
    delCommentsAndSpaces(nodesFile);
    nodesFile >> curr;
    if (!strcmp(curr,"terminal"))
    {
      circuit.terminals[termCount].height = static_cast<int>(tmp_height);
      circuit.terminals[termCount].width = static_cast<int>(tmp_width);
      strcpy(circuit.tableOfNames[circuit.nNodes+termCount].name, tmp_name);
      ++termCount;
      delCommentsAndSpaces(nodesFile);
      nodesFile >> curr;
    }
    else
    {
      circuit.nodes[nodesCount].height = static_cast<int>(tmp_height);
      circuit.nodes[nodesCount].width = static_cast<int>(tmp_width);
      strcpy(circuit.tableOfNames[nodesCount].name, tmp_name);
      ++nodesCount;
    }

  }
  //cout << "finish parsing nodes file\n";
  return 0;
}

int ParseScl(const char* fileName, Circuit& circuit)
{
  if (fileName == 0)
  {
    cout << "Parser error: null pointer on scl file";
    return 1;
  }
  cout << "parsing scl file...\n";
  FILE *sclFile;
  char currString[128];
  char tempString[128];
  char atempString[128];
  //char* pVal;
  int value;
  int value2;
  int paramCount = -1;
  float tempVal;
  sclFile = fopen(fileName, "r");
  if (sclFile)
  {
    fgets(currString, 128, sclFile);
    sscanf(currString, "%s %f", tempString, &tempVal);
    if (strcmp(tempString, "UCLA") == 0)
    {
      while (!feof(sclFile))
      {
        currString[0] = '\0';
        fgets(currString, 128, sclFile);
        if (currString[0] == '#') continue;
        paramCount = sscanf(currString, "%s : %d", tempString, &value);
        if (paramCount == -1) continue;
        if (_stricmp(tempString, "NumRows") == 0)
        {
          circuit.nRows = value;
          circuit.rows = new Row[circuit.nRows];
          continue;
        }
        for (int i = 0; i < circuit.nRows; ++i)
        {
          if (i != 0) fgets(currString, 128, sclFile);
          if (_stricmp(currString, "CoreRow Horizontal\n") == 0)
          {
            while (1)
            {
              fgets(currString, 128, sclFile);
              if (_stricmp(currString, "End\n") == 0) break;
              paramCount = sscanf(currString, "%s : %d", tempString, &value);
              if (paramCount == -1) continue;
              if (paramCount == 1) 
                sscanf(currString, "%s : %s", tempString, atempString);
              if (_stricmp(tempString, "SubrowOrigin") != 0)
              {
                if (_stricmp(tempString, "Coordinate") == 0)
                  circuit.rows[i].coordinate = value;
                if (_stricmp(tempString, "Height") == 0)
                  circuit.rows[i].height = value;
                if (_stricmp(tempString, "Sitewidth") == 0)
                  circuit.rows[i].siteWidth = value;
                if (_stricmp(tempString, "Sitespacing") == 0)
                  circuit.rows[i].siteSpacing = value;
                if (_stricmp(tempString, "Siteorient") == 0)
                  strcpy(circuit.rows[i].siteorient, atempString);
                if (_stricmp(tempString, "Sitesymmetry") == 0)
                  strcpy(circuit.rows[i].sitesymm, atempString); 
              }
              else
              {
                sscanf(currString, "%s : %d %s : %d", tempString, &value, atempString, &value2);
                circuit.rows[i].subrowOrigin = value;
                circuit.rows[i].numSites = value2;
              }
            }
          }
          else i--;
          paramCount = -1;
        }
      }
    }
    fclose(sclFile);
  }
  //cout << "finish parsing scl file\n";
  return 0;
}

ITLDRAGON_ERROR CMDParse(int argc, char* argv[])
{
  if (argc == 1)
  {
    cout << "Error: there is no input file name\n";
    exit(0);
  }

  SetDefaultKeysValues();

  for (int i = 1; i < argc; ++i)
  {
    if (stricmp(argv[i], "-noLog") == 0)
    {
      gOptions.doLog = false;
      continue;
    }
    else if (stricmp(argv[i], "-f") == 0)
    {
      strcpy(gOptions.benchmarkName, argv[i+1]);
      ++i;
      continue;
    }
    else if (stricmp(argv[i], "-cfg") == 0)
    {
      strcpy(gOptions.configName, argv[i+1]);
      CfgParse(argv[i+1]);
      ++i;
      continue;
    }
    else if (stricmp( argv[i], "-noBS") == 0)
    {
      gOptions.doBinSwapping = false;
      continue;
    }
    else if (stricmp( argv[i], "-noDBS") == 0)
    {
      gOptions.doDirectedBinSwapping = false;
      continue;
    }
    else if (stricmp( argv[i], "-DBS") == 0)
    {
      gOptions.doRecursiveBisection = false;
      if (i < argc - 1 && argv[i+1][0] != '-')
      {
        strcpy(gOptions.plName, argv[i+1]);
        ++i;
      }
      continue;
    }
    else if (stricmp( argv[i], "-CA") == 0)
    {
      gOptions.doRecursiveBisection = false;
      gOptions.doDirectedBinSwapping = false;
      if (i < argc - 1 && argv[i+1][0] != '-')
      {
        strcpy(gOptions.plName, argv[i+1]);
        ++i;
      }
      continue;
    }
    else if (stricmp( argv[i], "-OR") == 0)
    {
      gOptions.doRecursiveBisection = false;
      gOptions.doDirectedBinSwapping = false;
      gOptions.doCellAnnealing = false;
      if (i < argc - 1 && argv[i+1][0] != '-')
      {
        strcpy(gOptions.plName, argv[i+1]);
        ++i;
      }
      continue;
    }
    else if (stricmp( argv[i], "-dp") == 0)
    {
      gOptions.doRecursiveBisection  = false;
      gOptions.doBinSwapping         = false;
      gOptions.doDirectedBinSwapping = false;
      gOptions.doCellAnnealing       = false;
      gOptions.doOverlapRemoving     = false;
      resultFileName = "itlDragon_DP results.txt";
      if (i < argc - 1 && argv[i+1][0] != '-')
      {
        strcpy(gOptions.plName, argv[i+1]);
        ++i;
      }
      continue;
    }
    else if (stricmp( argv[i], "-noDP") == 0)
    {
      gOptions.doDetailedPlacement = false;
      continue;
    }
    else if (stricmp( argv[i], "-check") == 0)
    {
      gOptions.doCheckLegality       = true;
      if (i < argc - 1 && argv[i+1][0] != '-')
      {
        strcpy(gOptions.plName, argv[i+1]);
        ++i;
      }
      continue;
    }
    else if (stricmp( argv[i], "-test") == 0)
    {
      gOptions.doTest   = true;
	    gOptions.doDumpGP = true;
      continue;
    }
    else if (stricmp( argv[i], "-dumpGP") == 0)
    {
      gOptions.doDumpGP = true;
      continue;
    }
    else if (stricmp( argv[i], "-onlyGP") == 0)
    {
      gOptions.doDumpGP = true;
      gOptions.onlyGP   = true;
      continue;
    }
  }
  return OK;
}

void CfgParse(char* fileName)
{
  FILE* configFile;
  if (fileName == 0)
  {
    cout << "Parser error: null pointer on config file";
    return;
  }
  char string[255], *pVar;
  int row_counter = -1, params_counter = 0;
  configFile = fopen(fileName, "r");
  if (configFile)
  {
    fgets(string, 255, configFile);
    if (string[0] == '[')
    {
      cout << "Parser error: no benchmark name in config file";
      return;
    }
    else
    {
      //sscanf(string, "%s/n", string);
      string[strlen(string) - 1] = '\0';
      strcpy(gOptions.benchmarkName, string);
    }   
    while (!feof(configFile))
    {
      strcpy(string, "");
      fgets(string, 255, configFile);
      pVar = NULL;
      pVar = strchr(string, ',');
      if (pVar) pVar[0] = '.';
      pVar = NULL;
      if (string[0] == '[')
      {
        row_counter++;
        params_counter = 0;
        pVar = strchr(string, ']');
        if (pVar)
        {
          //while ((pVar[0] == ' ') || (pVar[0] == '\t')) pVar++;
          if (pVar[1] == '-')
            switch (row_counter){
        case 0: gOptions.doRecursiveBisection = false; break;
        case 1: gOptions.doBinSwapping = false; break;
        case 2: gOptions.doDirectedBinSwapping= false; break;
        case 3: gOptions.doCellAnnealing = false; break;
        case 4: gOptions.doDetailedPlacement = false; break;
        default: break;
          }
          else if (pVar[1] == '+')
            switch (row_counter)
          {
            case 0: gOptions.doRecursiveBisection = true; break;
            case 1: gOptions.doBinSwapping = true; break;
            case 2: gOptions.doDirectedBinSwapping= true; break;
            case 3: gOptions.doCellAnnealing = true; break;
            case 4: gOptions.doDetailedPlacement = true; break;
            default: break;
          }
        }
        continue;
      }
      pVar = strchr(string, '=');
      if (pVar)
      {
        pVar++;
        while ((pVar[0] == ' ') || (pVar[0] == '\t')) pVar++;
        if (pVar[0] != '\n')
          gOptions.innerParameters[row_counter][params_counter] = atof(pVar);
      }
      params_counter++;
    }
    fclose(configFile);
  }
  else
  {
    cout << "Parser error opening config file\n";
  }
}

int ParsePl(const char* fileName, Circuit& circuit)
{
  cout << "parsing pl file...\n";
  FILE *plFile;
  char currString[128];
  char tempString[128];
  char orient[3];
  float x;
  float y;
  int counter = 0;
  int paramCount = -1;
  int idx;
  //char orient;
  float tempVal;
  plFile = fopen(fileName, "r");
  if (plFile)
  {
    fgets(currString, 128, plFile);
    sscanf(currString, "%s %f", tempString, &tempVal);
    if (strcmp(tempString, "UCLA") == 0)
    {
      circuit.placement = new Place[numOfNT];
      while (!feof(plFile) && (counter < numOfNT))
      {
        fgets(currString, 128, plFile);
        if (currString[0] == '#') continue;
        //if (strchr(currString, ':') == NULL) return 1;
        paramCount = sscanf(currString, "%s %f %f : %s", tempString, &x, &y, orient);
        if (paramCount == -1) continue;
        idx = findNameBS(newTable, 0, numOfNT, tempString);
        idx = newTable[idx].cellIdx;
        //idx = findName(table, tempString);
        circuit.placement[idx].xCoord = x;
        circuit.placement[idx].yCoord = y;
        /*circuit.placement[counter].xCoord = x;
        circuit.placement[counter].yCoord = y;*/
        strcpy(circuit.placement[counter].orient, orient);
        //strcpy(circuit.placement[counter].orient, "N");
        counter++;
      }
    }
    delete []newTable;
    fclose(plFile);
  }
  else
  {
    cout << "pl file not found\n";
    exit(0);
  }
  //cout << "finish parsing pl file\n";
  return 0;
}

int ParseWts(const char* fileName, Circuit& circuit)
{
  cout << "parsing wts file...\n";
  FILE *wtsFile;
  char currString[128];
  char tempString[128];
  float weight;
  int counter = 0;
  int paramCount = -1;
  float tempVal;
  wtsFile = fopen(fileName, "r");
  if (wtsFile)
  {
    fgets(currString, 128, wtsFile);
    sscanf(currString, "%s %f", tempString, &tempVal);
    if (strcmp(tempString, "UCLA") == 0)
    {
      circuit.weights = new Weight[numOfNT];
      while (!feof(wtsFile) && (counter < numOfNT))
      {
        fgets(currString, 128, wtsFile);
        if (currString[0] == '#') continue;
        //if (strchr(currString, ':') == NULL) return 1;
        paramCount = sscanf(currString, "%s %f", tempString, &weight);
        if (paramCount == -1) continue;
        circuit.weights[counter].wts = weight;
        counter++;
        paramCount = -1;
      }
    }
    fclose(wtsFile);
  }

  return 0;
}

int ParseNets(const char* fileName, Circuit& circuit)
{
  cout << "parsing nets file...\n";
  FILE *netFile;
  char currString[128];
  char tempString[128];
  char atempString[128];
  char* pVal;
  int value;
  int numNets = 0;
  int numPins = 0;
  int netsCounter = 0;
  int firstParams = 0;
  int paramCount = -1;
  float xOffset = 0;
  float yOffset = 0;
  int cellIdx = -1;
  float tempVal;
  netFile = fopen(fileName, "r");
  if (netFile)
  {
    fgets(currString, 128, netFile);
    sscanf(currString, "%s %f", tempString, &tempVal);
    if (strcmp(tempString, "UCLA") == 0)
    {
      newTable = new strExtend[numOfNT];
      for (int i = 0; i < numOfNT; ++i)
      {
        strcpy( newTable[i].name, circuit.tableOfNames[i].name );
        newTable[i].cellIdx = i; 
      }
      QSortStrExtend(newTable, numOfNT - 1);
      /*for (int i = 0; i < numOfNT; ++i)
      {
      cout << newTable[i].name << "\t" << newTable[i].cellIdx << "\n";
      }*/
      while (!feof(netFile))
      {
        strcpy(currString, "#");
        fgets(currString, 128, netFile);
        if (currString[0] == '#') continue;
        //if (strchr(currString, ':') == NULL) return 1;
        paramCount = sscanf(currString, "%s : %d", tempString, &value);
        if (paramCount == -1) continue;
        if (_stricmp(tempString, "NumNets") == 0)
        {
          firstParams++;
          circuit.nNets = value;
          circuit.nets = new Net[circuit.nNets];
        }
        if (_stricmp(tempString, "NumPins") == 0)
        {
          firstParams++;
          circuit.nPins = value;
        }
        if (firstParams == 2)
        {
          if (_stricmp(tempString, "NetDegree") == 0)
          {
            pVal = strchr(currString, ':');
            sscanf(pVal, ": %d", &value);
            circuit.nets[netsCounter].numOfPins = value;
            circuit.nets[netsCounter].arrPins   = new Pin[value];
            int prevIdx = 0;
            for (int j = 0; j < circuit.nets[netsCounter].numOfPins; ++j)
            {
              xOffset = 0;
              yOffset = 0;
              fgets(currString, 128, netFile);
              paramCount = sscanf(currString, "%s %s : %f %f", tempString, atempString, &xOffset, &yOffset);
              cellIdx = findNameBS(newTable, 0, numOfNT, tempString);
              circuit.nets[netsCounter].arrPins[j].cellIdx = newTable[cellIdx].cellIdx;
              circuit.nets[netsCounter].arrPins[j].chtype    = atempString[0];
              /*cellIdx = findNameBS(table, 0, numOfNT, tempString);
              (*nets)[netsCounter].arrPins[j].cellIdx = cellIdx;*/
              //(*nets)[netsCounter].arrPins[j].cellIdx = findName(table, tempString);
              //prevIdx = (*nets)[netsCounter].arrPins[j].cellIdx;
              circuit.nets[netsCounter].arrPins[j].xOffset = xOffset;
              circuit.nets[netsCounter].arrPins[j].yOffset = yOffset;
            }
            netsCounter++;
            if (netsCounter == circuit.nNets) break;
          }
        }
        paramCount = -1;
      }
      //delete []newTable;
    }
    fclose(netFile);
  }
  //cout << "finish parsing net file\n";
  return 0;
}

ITLDRAGON_ERROR ValidateKeys()
{
  
  return OK;
}