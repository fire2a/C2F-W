#ifndef SPOTTING
#define SPOTTING

#include "Cells.h"

// Include libraries
#include <cmath>
#include <iostream>
#include <math.h>
#include <stdio.h>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

using namespace std;

std::vector<int> Spotting(std::unordered_map<int, Cells>& Cells_Obj,
                          std::vector<std::vector<int>>& coordCells,
                          std::unordered_set<int>& AvailSet,
                          double WSD,
                          double WSC,
                          std::unordered_map<std::string, double> spottingParams,
                          bool verbose);

#endif
