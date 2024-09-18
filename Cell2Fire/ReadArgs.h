#ifndef READARGS
#define READARGS

#include <iostream>
#include <fstream>
#include <vector>
#include <iterator>
#include <unordered_map>
#include <unordered_set>
#include <string>
#include <algorithm>
#include <unordered_set>

typedef struct {
    bool AllowCROS = false;
    bool BBOTuning = false;
    bool FinalGrid = false;
    bool Ignitions = false;
    bool IgnitionsLog = false;
    bool OutCrown = false;
    bool OutCrownConsumption = false;
    bool OutFl = false;
    bool OutIntensity = false;
    bool OutMessages = false;
    bool OutRos = false;
    bool OutSurfConsumption = false;
    bool OutputGrids = false;
    bool verbose = false;

    float BFactor = 1.0f;
    float CBDFactor = 1.0f; //S&B 
    float CCFFactor = 0.0f; //S&B
    float CROSThreshold = 0.5f;
    float EFactor = 1.0f;
    float FFactor = 1.0f;
    float FirePeriodLen = 0.0f;
    float HFIThreshold = 0.1f;
    float HFactor = 1.0f;
    float ROS10Factor = 3.34f; //S&B
    float ROSCV = 0.0f;
    float ROSThreshold = 0.1f;

    int FMC = 100;
    int IgnitionRadius = 0;
    int MaxFirePeriods = 1000;
    int MinutesPerWP = 60;
    int NWeatherFiles = 0;
    int TotalSims = 3;
    int TotalYears = 1;
    int nthreads = 1;
    int scenario = 3;
    int seed = 123;

    std::string HarvestPlan = "";
    std::string InFolder = ".";
    std::string OutFolder = "results";
    std::string Simulator = "S";
    std::string WeatherOpt = "rows";

    std::unordered_set<int> BCells;
    std::unordered_set<int> HCells;
} arguments;

void printHelp();
void printArguments(const arguments& args);
void postProcessArguments(arguments& args);
bool parseArguments(int argc, char* argv[], arguments& args);

char* getCmdOption(char ** begin, char ** end, const std::string & option);

bool cmdOptionExists(char** begin, char** end, const std::string& option);

int countWeathers(std::string directory_path);


#endif // READARGS_H
