// Inclusions
#include "ReadArgs.h"
#include <iostream>
#include <fstream>
#include <vector>
#include <iterator>
#include <string>
#include <algorithm>
#include <dirent.h>
#include <getopt.h>
#include <cstdlib>

inline char separator()
{
#if defined _WIN32 || defined __CYGWIN__
	return '\\';
#else
	return '/';
#endif
}

void printHelp() {
    std::cout << "Usage: program [options]\n"
              << "Options:\n"
	      << "  -O, --Simulator <string>    Set simulator (S, C, K), default S\n"
              << "  -M, --InFolder <path>       Set input instance folder\n"
              << "  -N, --OutFolder <path>      Set output results folder\n"
	      << "  -L, --HarvestPlan <string>  Set Harvest plan\n"
	      << "  -P, --WeatherOpt <string>   Set Weather option\n"

	      << "  -a, --AllowCROS             Enable CROS\n"
	      << "  -b, --BBOTuning             Enable BBO tuning\n"
	      << "  -c, --FinalGrid             Output final scars grids\n"
	      << "  -d, --Ignitions             Enable reading from Ignitions.csv\n"
	      << "  -e, --IgnitionsLog          Output ignition history csv\n"
	      << "  -f, --OutCrown              Output crown fire\n"
	      << "  -g, --OutCrownConsumption   Output crown consumption\n"
	      << "  -i, --OutFl                 Output flame lenght\n"
	      << "  -j, --OutIntensity          Output fire line intensity (Byram)\n"
	      << "  -k, --OutMessages           Output propagation digraph messages\n"
	      << "  -l, --OutRos                Output Rate Of Spread\n"
	      << "  -m, --OutSurfConsumption    Output surface consumption\n"
	      << "  -n, --OutputGrids           Output scar grids\n"

	      << "  -p, --BFactor <float>       Set Backward ellipsoid spread factor\n"
	      << "  -q, --CBDFactor <float>     Set Crown bulk density factor (S&B)\n"
	      << "  -r, --CCFFactor <float>     Set Canopy cover fraction factor (S&B)\n"
	      << "  -s, --CROSThreshold <float> Set CROS threshold\n"
	      << "  -t, --EFactor <float>       Set Ellipsoid spread factor\n"
	      << "  -u, --FFactor <float>       Set Forward ellipsoid spread factor\n"
	      << "  -o, --FirePeriodLen <float> Set fire period length\n"
	      << "  -w, --HFIThreshold <float>  Set HFI threshold\n"
	      << "  -x, --HFactor <float>       Set Heat factor\n"
	      << "  -y, --ROS10Factor <float>   Set ROS10 factor (S&B)\n"
	      << "  -z, --ROSCV <float>         Set ROS coefficient of variation\n"
	      << "  -A, --ROSThreshold <float>  Set ROS threshold\n"

	      << "  -B, --FMC <int>             Set Fuel Moisture Content\n"
	      << "  -C, --IgnitionRadius <int>  Set Ignition radius\n"
	      << "  -D, --MaxFirePeriods <int>  Set Maximum fire periods\n"
	      << "  -E, --MinutesPerWP <int>    Set Minutes per weather period\n"
	      << "  -F, --NWeatherFiles <int>   Set Number of weather files\n"
	      << "  -G, --TotalSims <int>       Set Total simulations\n"
	      << "  -H, --TotalYears <int>      Set Total years\n"
	      << "  -I, --nthreads <int>        Set Number of threads\n"
	      << "  -J, --scenario <int>        Set Scenario\n"
	      << "  -K, --seed <int>            Set random number generator seed\n"

              << "  -v, --verbose               Enable verbose mode\n"
              << "  -h, --help                  Show this help message\n";
}

void printArguments(const arguments& args) {

    // Booleans
    std::cout << "AllowCROS: " << args.AllowCROS << std::endl;
    std::cout << "BBOTuning: " << args.BBOTuning << std::endl;
    std::cout << "FinalGrid: " << args.FinalGrid << std::endl;
    std::cout << "Ignitions: " << args.Ignitions << std::endl;
    std::cout << "IgnitionsLog: " << args.IgnitionsLog << std::endl;
    std::cout << "OutCrown: " << args.OutCrown << std::endl;
    std::cout << "OutCrownConsumption: " << args.OutCrownConsumption << std::endl;
    std::cout << "OutFl: " << args.OutFl << std::endl;
    std::cout << "OutIntensity: " << args.OutIntensity << std::endl;
    std::cout << "OutMessages: " << args.OutMessages << std::endl;
    std::cout << "OutRos: " << args.OutRos << std::endl;
    std::cout << "OutSurfConsumption: " << args.OutSurfConsumption << std::endl;
    std::cout << "OutputGrids: " << args.OutputGrids << std::endl;
    std::cout << "verbose: " << args.verbose << std::endl;

    // Floats
    std::cout << "BFactor: " << args.BFactor << std::endl;
    std::cout << "CBDFactor: " << args.CBDFactor << std::endl;
    std::cout << "CCFFactor: " << args.CCFFactor << std::endl;
    std::cout << "CROSThreshold: " << args.CROSThreshold << std::endl;
    std::cout << "EFactor: " << args.EFactor << std::endl;
    std::cout << "FFactor: " << args.FFactor << std::endl;
    std::cout << "FirePeriodLen: " << args.FirePeriodLen << std::endl;
    std::cout << "HFIThreshold: " << args.HFIThreshold << std::endl;
    std::cout << "HFactor: " << args.HFactor << std::endl;
    std::cout << "ROS10Factor: " << args.ROS10Factor << std::endl;
    std::cout << "ROSCV: " << args.ROSCV << std::endl;
    std::cout << "ROSThreshold: " << args.ROSThreshold << std::endl;

    // Integers
    std::cout << "FMC: " << args.FMC << std::endl;
    std::cout << "IgnitionRadius: " << args.IgnitionRadius << std::endl;
    std::cout << "MaxFirePeriods: " << args.MaxFirePeriods << std::endl;
    std::cout << "MinutesPerWP: " << args.MinutesPerWP << std::endl;
    std::cout << "NWeatherFiles: " << args.NWeatherFiles << std::endl;
    std::cout << "TotalSims: " << args.TotalSims << std::endl;
    std::cout << "TotalYears: " << args.TotalYears << std::endl;
    std::cout << "nthreads: " << args.nthreads << std::endl;
    std::cout << "scenario: " << args.scenario << std::endl;
    std::cout << "seed: " << args.seed << std::endl;

    // Strings
    std::cout << "HarvestPlan: " << args.HarvestPlan << std::endl;
    std::cout << "InFolder: " << args.InFolder << std::endl;
    std::cout << "OutFolder: " << args.OutFolder << std::endl;
    std::cout << "Simulator: " << args.Simulator << std::endl;
    std::cout << "WeatherOpt: " << args.WeatherOpt << std::endl;

    // ??
    std::cout << "HCells: ";
    for (const auto& cell : args.HCells) {
        std::cout << cell << " ";
    }
    std::cout << std::endl;

    std::cout << "BCells: ";
    for (const auto& cell : args.BCells) {
        std::cout << cell << " ";
    }
    std::cout << std::endl;
}


bool parseArguments(int argc, char* argv[], arguments& args) {
    static struct option long_options[] = {
        // Booleans
        {"AllowCROS", no_argument, 0, 'a'},
        {"BBOTuning", no_argument, 0, 'b'},
        {"FinalGrid", no_argument, 0, 'c'},
        {"Ignitions", no_argument, 0, 'd'},
        {"IgnitionsLog", no_argument, 0, 'e'},
        {"OutCrown", no_argument, 0, 'f'},
        {"OutCrownConsumption", no_argument, 0, 'g'},
        {"OutFl", no_argument, 0, 'i'},
        {"OutIntensity", no_argument, 0, 'j'},
        {"OutMessages", no_argument, 0, 'k'},
        {"OutRos", no_argument, 0, 'l'},
        {"OutSurfConsumption", no_argument, 0, 'm'},
        {"OutputGrids", no_argument, 0, 'n'},
        {"verbose", no_argument, 0, 'v'},

        // Floats
        {"BFactor", required_argument, 0, 'p'},
        {"CBDFactor", required_argument, 0, 'q'},
        {"CCFFactor", required_argument, 0, 'w'},
        {"CROSThreshold", required_argument, 0, 's'},
        {"EFactor", required_argument, 0, 't'},
        {"FFactor", required_argument, 0, 'u'},
        {"FirePeriodLen", required_argument, 0, 'o'},
        {"HFIThreshold", required_argument, 0, 'w'},
        {"HFactor", required_argument, 0, 'x'},
        {"ROS10Factor", required_argument, 0, 'y'},
        {"ROSCV", required_argument, 0, 'z'},
        {"ROSThreshold", required_argument, 0, 'A'},

        // Integers
	{"FMC", required_argument, 0, 'B'},
	{"IgnitionRadius", required_argument, 0, 'C'},
	{"MaxFirePeriods", required_argument, 0, 'D'},
	{"MinutesPerWP", required_argument, 0, 'E'},
	{"NWeatherFiles", required_argument, 0, 'F'},
	{"TotalSims", required_argument, 0, 'G'},
	{"TotalYears", required_argument, 0, 'H'},
	{"nthreads", required_argument, 0, 'I'},
	{"scenario", required_argument, 0, 'J'},
	{"seed", required_argument, 0, 'K'},

        // Strings
        {"HarvestPlan", required_argument, 0, 'L'},
        {"InFolder", required_argument, 0, 'M'},
        {"OutFolder", required_argument, 0, 'N'},
        {"Simulator", required_argument, 0, 'O'},
        {"WeatherOpt", required_argument, 0, 'P'},

        {"help", no_argument, 0, 'h'},
        {0, 0, 0, 0}
    };

    int opt;
    int option_index = 0;
    while ((opt = getopt_long(argc, argv, "a:b:c:d:e:f:g:h:i:j:k:l:m:n:o:p:q:r:s:t:u:v:w:x:y:z:A:B:C:D:E:F:G:H:I:J:K:L:M:N:O:P", long_options, &option_index)) != -1) {
        switch (opt) {
            case 'a': args.AllowCROS = true; break;
            case 'b': args.BBOTuning = true; break;
            case 'c': args.FinalGrid = true; break;
            case 'd': args.Ignitions = true; break;
            case 'e': args.IgnitionsLog = true; break;
	    case 'f': args.OutCrown = true; break;
            case 'g': args.OutCrownConsumption = true; break;
            case 'i': args.OutFl = true; break;
            case 'j': args.OutIntensity = true; break;
            case 'k': args.OutMessages = true; break;
            case 'l': args.OutRos = true; break;
            case 'm': args.OutSurfConsumption = true; break;
            case 'n': args.OutputGrids = true; break;
            case 'v': args.verbose = true; break;

            case 'p': args.BFactor = std::atof(optarg); break;
            case 'q': args.CBDFactor = std::atof(optarg); break;
            case 'r': args.CCFFactor = std::atof(optarg); break;
            case 's': args.CROSThreshold = std::atof(optarg); break;
            case 't': args.EFactor = std::atof(optarg); break;
            case 'u': args.FFactor = std::atof(optarg); break;
            case 'o': args.FirePeriodLen = std::atof(optarg); break;
            case 'w': args.HFIThreshold = std::atof(optarg); break;
            case 'x': args.HFactor = std::atof(optarg); break;
            case 'y': args.ROS10Factor = std::atof(optarg); break;
            case 'z': args.ROSCV = std::atof(optarg); break;
            case 'A': args.ROSThreshold = std::atof(optarg); break;
                  
            case 'B': args.FMC = std::atoi(optarg); break;
            case 'C': args.IgnitionRadius = std::atoi(optarg); break;
            case 'D': args.MaxFirePeriods = std::atoi(optarg); break;
            case 'E': args.MinutesPerWP = std::atoi(optarg); break;
            case 'F': args.NWeatherFiles = std::atoi(optarg); break;
	    case 'G': args.TotalSims = std::atoi(optarg); break;
	    case 'H': args.TotalYears = std::atoi(optarg); break;
	    case 'I': args.nthreads = std::atoi(optarg); break;
	    case 'J': args.scenario = std::atoi(optarg); break;
	    case 'K': args.seed = std::atoi(optarg); break;
                  
            case 'L': args.HarvestPlan = optarg; break;
            case 'M': args.InFolder = optarg; break;
            case 'N': args.OutFolder = optarg; break;
            case 'O': args.Simulator = optarg; break;
            case 'P': args.WeatherOpt = optarg; break;
            case 'h': printHelp(); return 0;
            default:
                std::cerr << "Unknown option: " << opt << std::endl;
		printHelp();
                return false;
        }
    }
    return true;
}

void postProcessArguments(arguments& args) {
    if (args.NWeatherFiles < 1) {
	if (args.WeatherOpt == "random") {
	    args.NWeatherFiles = countWeathers(args.InFolder + separator() + "Weathers");
	} else {
	    args.NWeatherFiles = 1;
	}
    }

    if (args.FirePeriodLen <= args.MinutesPerWP) {
	args.FirePeriodLen = args.MinutesPerWP;
    }

    if (args.Simulator != "S" && args.Simulator != "C" && args.Simulator != "K") {
	printf("Simulator not in [\"S\", \"C\", \"K\"]\n");
	exit(1);
    }

    if (args.InFolder.back() != separator()) {
	args.InFolder += separator();
    }

    if (args.OutFolder.empty()) {
	args.OutFolder = args.InFolder + "results";
    }

    if (args.OutFolder.back() != separator()) {
	args.OutFolder += separator();
    }
}

int countWeathers(const std::string directory_path) {
	DIR* dir;
	struct dirent* ent;
	int file_count = 0;

	if ((dir = opendir(directory_path.c_str())) != NULL) {
		while ((ent = readdir(dir)) != NULL) {
			if (ent->d_type == DT_REG) {
				std::string filename = ent->d_name;
				if (filename.substr(0, 7) == "Weather" && filename.substr(filename.size() - 4) == ".csv") {
					file_count++;
				}
			}
		}
		closedir(dir);
	}
	else {
		std::cout << "Could not open directory" << std::endl;
		return -1;
	}

	return file_count;
}
