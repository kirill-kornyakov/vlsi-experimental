/*  
 * output.h
 * this is a part of itlAnalyticalPlacer source codes
 * Copyright (C) 2005-2007, UNN, ITLab, Zhivoderov, Kornyakov, Kurina
 * email: zhivoderov.a@gmail.com
 * email: itlab.sa@c​s.vmk.unn.ru
 */

#ifndef _OUTPUT_H_
#define _OUTPUT_H_

#include <fstream>
#include <iostream>
#include "..\include\data_structures.h"
#include "..\include\errors.h"
#include "..\include\global.h"
#include <stdio.h>
#include <time.h>
#include <io.h>

using namespace std;

void PrintCircuitInfo(Circuit& circuit);

void PrintResultString(int argc, char* argv[], Statistics& statistics, MULTIPLACER_ERROR errorCode);

void PrintToPL(const char* fileName, Circuit& circuit, Statistics& statistics,
               double shiftX = 0, double shiftY = 0, 
               int nBinRows = 1, int nBinCols = 1);

void PrintToPL(const char* fileName, Circuit& circuit, 
               double shiftX = 0, double shiftY = 0, 
               int nBinRows = 1, int nBinCols = 1);

void PrintToTmpPL(Circuit& circuit, Statistics& statistics, double shiftX = 0, double shiftY = 0);

void PrintToTmpPL(Circuit& circuit, double shiftX = 0, double shiftY = 0);

void PrintPLT(const char* fileName, Circuit& circuit);

MULTIPLACER_ERROR ShiftCoords(Circuit& circuit);

MULTIPLACER_ERROR ReshiftCoords(Circuit& circuit);

void CreateHTMLReport(double wireLength, double workTime, MULTIPLACER_ERROR errorCode);

void PrintNetsToRouterFormat(Circuit& circuit);

#endif