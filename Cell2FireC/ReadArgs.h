#ifndef READARGS2
#define READARGS2

#include <iostream>
#include <getopt.h>
#include <fstream>
#include <vector>
#include <iterator>
#include <unordered_map>
#include <unordered_set>
#include <string>
#include <algorithm>
#include <unistd.h>

typedef struct{ 
	std::string InFolder, OutFolder, WeatherOpt, HarvestPlan, Simulator;
	bool OutMessages, OutFl,OutIntensity,OutRos,OutCrown,OutCrownConsumption,OutSurfConsumption, Trajectories, NoOutput, verbose, Ignitions, OutputGrids, FinalGrid, PromTuned, Stats, BBOTuning, AllowCROS;
	float ROSCV, ROSThreshold, CROSThreshold, HFIThreshold, HFactor, FFactor, BFactor, EFactor, FirePeriodLen;
	float	CBDFactor, CCFFactor, ROS10Factor, CROSActThreshold;
	int MinutesPerWP, MaxFirePeriods, TotalYears, TotalSims, NWeatherFiles, IgnitionRadius, seed, nthreads,FMC,scenario;
	std::unordered_set<int>  HCells, BCells;
} arguments;

void display_help();

void parseArgs(int argc, char* argv[], arguments* args_ptr);

int countWeathers(std::string directory_path);

#endif