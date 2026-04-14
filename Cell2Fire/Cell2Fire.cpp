/* coding: utf-8
__version__ = "3.0"
__author__ = "Cristobal Pais"
__maintainer__ = "Jaime Carrasco, Cristobal Pais, David Woodruff, David Palacios"
*/
// Unified version by David Palacios
//  Include classes
#include "Cell2Fire.h"
#include "Cells.h"
#include "DataGenerator.h"
#include "FuelModelKitral.h"
#include "FuelModelUtils.h"
#include "ReadArgs.h"
#include "ReadCSV.h"
#include "WriteCSV.h"

// Include libraries
#include <algorithm>
#include <boost/random.hpp>
#include <chrono>
#include <cmath>
#include <iomanip>
#include <iostream>
#include <math.h>
#include <omp.h>
#include <random>
#include <sstream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

using namespace std;

// Global Variables — read-only after construction, shared across all threads
inputs* df_ptr;
inputs* df;
std::vector<std::vector<int>> coordCells;
std::vector<float> ignProb;
int currentSim = 0;
std::unordered_map<int, std::vector<int>> HarvestedCells;
std::vector<float> WeatherWeights;
std::vector<int> WeatherWeightIDs;
std::unordered_map<int, int> IgnitionHistory;
std::unordered_map<int, std::string> WeatherHistory;
// Per-simulation stats: [mean_surface_fl, mean_crown_fl, max_fl] — indexed by sim-1
// Written once per sim in Results() (each sim index is unique across threads → no lock needed)
std::vector<std::array<float, 3>> simStats;

/******************************************************************************
                                                                                                                                Utils
*******************************************************************************/

/**
 *
 *
 */
void
printSets(std::unordered_set<int> availCells,
          std::unordered_set<int> nonBurnableCells,
          std::unordered_set<int> burningCells,
          std::unordered_set<int> burntCells,
          std::unordered_set<int> harvestCells)
{
    std::cout << "\nSet information period" << std::endl;
    std::cout << "Available Cells:";
    for (auto& ac : availCells)
    {
        std::cout << " " << ac;
    }
    std::cout << std::endl;

    std::cout << "nonBurnable Cells:";
    for (auto& nbc : nonBurnableCells)
    {
        std::cout << " " << nbc;
    }
    std::cout << std::endl;

    std::cout << "Burning Cells:";
    for (auto& bc : burningCells)
    {
        std::cout << " " << bc;
    }
    std::cout << std::endl;

    std::cout << "Burnt Cells:";
    for (auto& bc : burntCells)
    {
        std::cout << " " << bc;
    }
    std::cout << std::endl;

    std::cout << "Firebreak Cells:";
    for (auto& hc : harvestCells)
    {
        std::cout << " " << hc;
    }
    std::cout << std::endl;
}

// Output grids
void
CSVGrid(int rows, int cols, int gridNumber, std::string gridFolder, std::vector<int>& statusCellsCSV, bool verbose)
{
    std::string gridName;
    if (gridNumber < 10)
    {
        gridName = gridFolder + "ForestGrid0" + std::to_string(gridNumber) + ".csv";
    }

    else
    {
        gridName = gridFolder + "ForestGrid" + std::to_string(gridNumber) + ".csv";
    }

    if (verbose)
    {
        std::cout << "We are plotting the current forest to a csv file " << gridName << std::endl;
    }
    CSVWriter CSVPloter(gridName, ",");
    CSVPloter.printCSV_V2(rows, cols, statusCellsCSV);
}

inline char
separator()
{
#if defined _WIN32 || defined __CYGWIN__
    return '\\';
#else
    return '/';
#endif
}

/******************************************************************************
                                                                                                                        Constructor
*******************************************************************************/

/**
 * @brief Constructs and initializes the Cell2Fire object to run a simulation.
 *
 * This constructor initializes the `Cell2Fire` object by reading configuration
 * and data files required for simulating fire spread in a forested
 * environment. It populates various member variables, including fuel
 * coefficients, forest structure, weather data, ignition points, and other
 * simulation-related parameters. It also performs preprocessing steps to
 * categorize cells into burnable and non-burnable types and to establish fire
 * ignition sets.
 *
 * @param _args List of command line arguments containing configuration and
 * file paths needed for the simulation.
 *
 * @return A fully initialized `Cell2Fire` object ready for simulation.
 *
 * ### Notes:
 * - Initializes forest data from the `fuels` data and `Data.csv` files.
 * - Initializes weather data from `Weather.csv` and optionally
 * `WeatherDistribution.csv`.
 * - Processes ignition points from `Ignitions.csv` (if provided) and computes
 * adjacent cells based on the ignition radius.
 * - Categorizes cells as burnable, non-burnable, or harvested based on input
 * data.
 * - If BBO tuning is enabled, initializes tuning factors using `BBOFuels.csv`.
 * - Validates and adjusts simulation parameters based on the weather file
 * consistency.
 */
Cell2Fire::Cell2Fire(arguments _args) : CSVForest(_args.InFolder + "fuels", " ")
{
    // Aux
    int i;
    this->CSVWeather = CSVReader();
    // Populate arguments from command line into the Cell2Fire object
    this->args = _args;
    this->args_ptr = &this->args;

    // this->wdf_ptr = new weatherDF;
    // this->wdf[100000];

    /********************************************************************
     *
     *															Initialization
     *steps
     *
     ********************************************************************/
    // DEBUGstd::cout << "------------------ Simulator C++ Beta version
    // ----------------------\n" << std::endl;
    /*
                  Command line arguments (all of them inside args structure)
    */
    // DEBUGstd::cout << "------ Command line values ------\n";
    // DEBUGprintArgs(this->args);

    /********************************************************************
                                                             Initialize fuel
    coefficients for Spread Model
    ********************************************************************/
    // fuel_coefs coefs[18];
    this->coef_ptr = &this->coefs[0];

    this->totalFirePeriods = this->args.MaxFirePeriods;

    /********************************************************************
                                    Global Values (Forest) and Instance (in
    memory for the moment)
    ********************************************************************/
    // DEBUGstd::cout << "\n------ Instance from file initialization ------\n";
    this->sim = 1;
    /********************************************************************
                                                                                    Read Instance from csv files...
    ********************************************************************/
    // Create forest structure
    forestDF frdf;
    // DEBUG
    std::cout << "\n------------------Forest Data ----------------------\n" << std::endl;
    std::vector<std::vector<std::string>> FDF = this->CSVForest.getData(_args.InFolder + "fuels");

    // DEBUGthis->CSVForest.printData(FDF);
    this->CSVForest.parseForestDF(&frdf, FDF);

    // DEBUGstd::cout << "------------------Detailed Data
    // ----------------------\n" << std::endl;
    this->rows = frdf.rows;
    this->cols = frdf.cols;
    this->widthSims = std::to_string(this->args.TotalSims).length();
    this->nCells = rows * cols;
    this->cellSide = frdf.cellside;
    this->areaCells = cellSide * cellSide;
    this->perimeterCells = 4 * cellSide;
    this->xllcorner = frdf.xllcorner;
    this->yllcorner = frdf.yllcorner;

    coordCells = frdf.coordCells;
    // this->adjCells = frdf.adjCells;

    /********************************************************************
                                                    Dataframes initialization:
    Forest and Weather
    ********************************************************************/
    // DEBUGstd::cout << "\n------ Read DataFrames: Forest and Weather ------\n";

    /* Forest DataFrame */
    std::string filename = this->args.InFolder + "Data.csv";
    std::string sep = ",";
    CSVReader CSVParser(filename, sep);

    std::cout << "Forest DataFrame from instance " << filename << std::endl;
    std::cout << "Number of cells: " << this->nCells << std::endl;
    df = new inputs[this->nCells];

    // Create empty df with size of NCells
    df_ptr = &df[0];  // access reference for the first element of df

    // Initialize ignProb before the streaming parse (parseDFDirect fills it from col 14)
    ignProb = std::vector<float>(this->nCells, 1);

    // Stream Data.csv directly into df without intermediate vector<vector<string>>
    CSVParser.parseDFDirect(df_ptr, filename, this->args_ptr, this->nCells, ignProb);

    // Initialize and populate relevant vectors
    fTypeCells = std::vector<int>(this->nCells, 1);
    this->statusCells = std::vector<int>(this->nCells, 0);
    this->cells_flat.assign(this->nCells, nullptr);
    // Outputs
    this->crownState = std::vector<int>(this->nCells, 0);
    this->crownFraction = std::vector<float>(this->nCells, 0);
    this->surfFraction = std::vector<float>(this->nCells, 0);
    this->surfaceIntensities = std::vector<float>(this->nCells, 0);
    this->RateOfSpreads = std::vector<float>(this->nCells, 0);
    this->surfaceFlameLengths = std::vector<float>(this->nCells, 0);
    this->crownIntensities = std::vector<float>(this->nCells, 0);
    this->crownFlameLengths = std::vector<float>(this->nCells, 0);
    this->maxFlameLengths = std::vector<float>(this->nCells, 0);

    // Non burnable types: populate relevant fields such as status and ftype
    std::string NoFuel = "NF ";
    std::string NoData = "ND ";
    std::string Empty = "";
    const char* NF = NoFuel.c_str();
    const char* ND = NoData.c_str();
    const char* EM = Empty.c_str();

    for (int l = 0; l < this->nCells; l++)
    {
        if (strcmp(df[l].fueltype, NF) == 0 || strcmp(df[l].fueltype, ND) == 0)
        {
            fTypeCells[l] = 0;

            this->statusCells[l] = 4;
        }
    }

    // Harvested cells
    if (strcmp(this->args.HarvestPlan.c_str(), EM) != 0)
    {
        std::string sep = ",";
        CSVReader CSVHPlan(this->args.HarvestPlan, sep);

        // Populate Ignitions vector
        std::vector<std::vector<std::string>> HarvestedDF = CSVHPlan.getData(this->args.HarvestPlan);
        // CSVHPlan.printData(HarvestedDF);

        // Cells
        int HCellsP = HarvestedDF.size() - 1;
        CSVHPlan.parseHarvestedDF(HarvestedCells, HarvestedDF, HCellsP);

        // Print-out
        std::cout << "Number of Firebreak Cells :" << HarvestedCells.size() << std::endl;
        for (auto it = HarvestedCells.begin(); it != HarvestedCells.end(); it++)
        {
            for (auto& it2 : it->second)
            {
                fTypeCells[it2 - 1] = 0;
    
                this->statusCells[it2 - 1] = 3;
            }
        }
    }

    /*
    if (this->args.UseWeatherWeights)
    {
        cout << "use weather weights" << endl;
        std::string sep = ",";
        CSVReader CSVWeatherWeights(this->args.WeatherWeightsFile, sep);

        std::vector<std::vector<std::string>> WeatherWeightsDF = CSVWeatherWeights.getData(this->args.WeatherWeightsFile);
        cout << "ww df "<< WeatherWeightsDF[0][0] << endl;

        CSVWeatherWeights.parseWeatherWeights(WeatherWeights, WeatherWeightIDs, WeatherWeightsDF, this->args.NWeatherFiles);

    }*/

    // Relevant sets: Initialization
    this->availCells.clear();
    nonBurnableCells.clear();
    this->burningCells.clear();
    this->burntCells.clear();
    this->harvestCells.clear();
    for (i = 0; i < this->statusCells.size(); i++)
    {
        if (this->statusCells[i] < 3)
            this->availCells.insert(i + 1);
        else if (this->statusCells[i] == 4)
            nonBurnableCells.insert(i + 1);
        else if (this->statusCells[i] == 3)
            this->harvestCells.insert(i + 1);
    }

    // POTENCIALMENTE AQUI ESTA MALO
    /* Weather DataFrame */
    // this->WeatherData = this->CSVWeather.getData();
    // std::cout << "\nWeather DataFrame from instance " << this->CSVWeather.fileName << std::endl;

    if (this->args.WeatherOpt.compare("distribution") == 0)
    {
        CSVReader CSVWeatherDistribution(_args.InFolder + "WeatherDistribution.csv", ",");
        this->WDist = CSVWeatherDistribution.getData(_args.InFolder + "WeatherDistribution.csv");
        CSVWeatherDistribution.printData(WDist);
    }

    // Populate WDF
    // int WPeriods = WeatherData.size() - 1;  // -1 due to header
    // wdf_ptr = &wdf[0];
    // std::cout << "Weather Periods: " << WPeriods << std::endl;

    // Populate the wdf objects
    // this->CSVWeather.parseWeatherDF(wdf_ptr, this->args_ptr, this->WeatherData, WPeriods);
    // std::cout << "\nPass Debug " << std::endl;
    // DEBUGthis->CSVWeather.printData(this->WeatherData);

    /*  Ignitions */
    int IgnitionYears;
    std::vector<int> IgnitionPoints;

    if (this->args.Ignitions)
    {
        // DEBUGstd::cout << "\nWe have specific ignition points:" << std::endl;

        /* Ignition points */
        std::string ignitionFile = args.InFolder + "Ignitions.csv";
        std::string sep = ",";
        CSVReader CSVIgnitions(ignitionFile, sep);

        // Populate Ignitions vector
        std::vector<std::vector<std::string>> IgnitionsDF = CSVIgnitions.getData(ignitionFile);
        // DEBUGstd::cout << "Ignition points from file " << ignitionFile <<
        // std::endl; DEBUGCSVIgnitions.printData(IgnitionsDF);

        // Total Years
        IgnitionYears = IgnitionsDF.size() - 1;
        // DEBUGstd::cout << "Ignition Years: " << IgnitionYears  << std::endl;
        // DEBUGstd::cout << "Total Years: " << this->args.TotalYears  <<
        // std::endl;
        args.TotalYears = std::min(args.TotalYears, IgnitionYears);
        // DEBUGstd::cout << "Setting TotalYears to " << args.TotalYears << " for
        // consistency with Ignitions file" << std::endl;

        // Ignition points
        this->IgnitionPoints = std::vector<int>(IgnitionYears, 0);
        CSVIgnitions.parseIgnitionDF(this->IgnitionPoints, IgnitionsDF, IgnitionYears);
        // this->IgnitionSets =
        // std::vector<unordered_set<int>>(this->IgnitionPoints.size());
        this->IgnitionSets = std::vector<std::vector<int>>(this->args.TotalYears);

        // Ignition radius
        if (this->args.IgnitionRadius > 0)
        {
            // Aux
            int i, a, igVal;
            // std::unordered_set<int> auxSet;
            std::vector<int> auxSet;
            int auxIg = 0;

            // Debug
            // for (i=0; i<this->args.TotalYears; i++)
            //	std::cout << this->IgnitionPoints[i] <<	std::endl;

            // Loop
            for (i = 0; i < this->args.TotalYears; i++)
            {
                auxSet.clear();
                igVal = this->IgnitionPoints[i];
                std::vector<int> adjacentCellsVector = adjacentCells(igVal, this->rows, this->cols);
                if (this->args.IgnitionRadius == 1)
                {
                    for (auto& nb : adjacentCellsVector)
                    {
                        if (nb != -1)
                        {
                            // this->IgnitionSets[auxIg].insert(nb.second);
                            this->IgnitionSets[auxIg].push_back(nb);
                        }
                    }
                    // this->IgnitionSets[auxIg].insert(igVal);
                    this->IgnitionSets[auxIg].push_back(igVal);
                }

                if (this->args.IgnitionRadius > 1)
                {
                    // Initial ignition set (first year for 1 degree)
                    for (auto& nb : adjacentCellsVector)
                    {
                        if (nb != -1)
                        {
                            // this->IgnitionSets[auxIg].insert(nb.second);
                            this->IgnitionSets[auxIg].push_back(nb);
                        }
                    }
                    int auxR = 1;
                    // auxSet.insert(igVal);
                    auxSet.push_back(igVal);

                    // loop over radius of adjacents
                    while (auxR < args.IgnitionRadius)
                    {
                        // unordered_set<int> IgnitionSetsAux =
                        // this->IgnitionSets[auxIg];
                        std::vector<int> IgnitionSetsAux = this->IgnitionSets[auxIg];

                        for (auto& c : IgnitionSetsAux)
                        {
                            // Populate Aux Set
                            std::vector<int> adjacentCellsVector = adjacentCells(c, this->rows, this->cols);
                            for (auto& na : adjacentCellsVector)
                            {
                                if (na != -1)
                                {
                                    // auxSet.insert(na.second);
                                    auxSet.push_back(na);
                                }
                            }
                        }
                        // Save aux set as Ignition Set
                        this->IgnitionSets[auxIg] = auxSet;
                        auxR += 1;
                    }
                }
                // Next iteration (year)
                auxIg++;
            }
        }

        // Clean vectors
        for (auto& v : this->IgnitionSets)
        {
            std::sort(v.begin(), v.end());
            v.erase(std::unique(v.begin(), v.end()), v.end());
        }

        // IgRadius Information
        if (this->args.verbose)
        {
            for (auto& nb : this->IgnitionSets[0])
            {
                std::cout << "Ignition sets[0] example:" << nb << std::endl;
            }
        }
    }

    /********************************************************************
     *
     *											Global
     *Parameters for loop
     *
     *********************************************************************/
    // Global parameters for the loop
    this->year = 1;
    this->weatherPeriod = 0;
    this->noIgnition = true;  // None = -1
    this->activeCrown = false;
    this->gridNumber = 0;
    this->fire_period = vector<int>(this->args.TotalYears, 0);
}

Cell2Fire::~Cell2Fire()
{
    for (auto& p : this->cells_flat) { delete p; p = nullptr; }
}

/******************************************************************************
                                                                                                                                Methods
*******************************************************************************/

/**
 * @brief Initializes a cell within the simulation and adds it to the cell
 * collection.
 *
 * This method creates a `Cells` object for the specified cell ID, populates it
 * with the necessary parameters, and stores it in the `Cells_Obj` unordered
 * map. It also initializes the fire-related fields for the cell and, if
 * verbose mode is enabled, prints debugging information about the cell.
 *
 * @param id The ID of the cell to initialize.
 *
 * ### Details:
 * - The `Cells` object is constructed using the cell's properties, including:
 *   - `areaCells`: The area of the cell.
 *   - `coordCells`: The coordinates of the cell.
 *   - `fTypeCells`: The fuel type of the cell.
 *   - `fTypeCells2`: A textual description of the fuel type.
 *   - `perimeterCells`: The perimeter of the cell.
 *   - `statusCells`: The status of the cell (e.g., burning, available).
 *   - `id`: The identifier for the cell.
 * - The newly created cell is added to the `Cells_Obj` unordered map.
 *
 */

void
Cell2Fire::InitCell(int id)
{
    this->cells_flat[id - 1] = new Cells(id - 1,
                                          this->areaCells,
                                          coordCells[id - 1],
                                          fTypeCells[id - 1],
                                          this->perimeterCells,
                                          this->statusCells[id - 1],
                                          id);
    this->cells_flat[id - 1]->initializeFireFields(coordCells, this->availCells, this->cols, this->rows);

    // Print info for debugging
    if (this->args.verbose)
        this->cells_flat[id - 1]->print_info();
}

/**
 * @brief Creates a directory for storing fire metric outputs within the output folder.
 *
 * This function generates a directory inside the specified output folder based on the given fire metric name.
 * If the metric corresponds to a type that produces multiple files (e.g., "Grids"), it creates a
 * nested directory for each simulation run.
 *
 * @param metric Name of the fire metric for which an output directory will be created.
 * @return The path to the created output folder for the given metric.
 *
 */
string
Cell2Fire::createOutputFolder(string metric)
{
    CSVWriter CSVFolder("", "");
    string tempName = this->args.OutFolder + metric + separator();
    CSVFolder.MakeDir(tempName);
    if (metric.compare("Grids") == 0)
    {
        tempName = this->args.OutFolder + metric + separator() + metric + std::to_string(this->sim) + separator();
        CSVFolder.MakeDir(tempName);
    }
    return tempName;
}

/**
 * @brief Resets the simulation environment for a new run.
 *
 * This method initializes and resets all relevant parameters, structures, and
 * files needed for the simulation. It handles:
 * - Global simulation parameters
 * - Output directories and files
 * - Weather configuration
 * - Randomized inputs for fire spread simulation
 * - Data structures related to cells and fire metrics
 *
 * @param rnumber Random seed or identifier for weather selection.
 * @param rnumber2 Random value for initializing ROS (Rate of Spread)
 * variations.
 * @param simExt Simulation extension identifier (default: 1).
 *
 */
void
Cell2Fire::reset(int rnumber, double rnumber2, int simExt = 1)
{
    // Reset info

    // Aux
    int i;

    // Reset global parameters for the simulation
    this->year = 1;
    this->weatherPeriod = 0;
    this->noIgnition = true;  // None = -1
    this->nIgnitions = 0;
    this->gridNumber = 0;
    this->activeCrown = false;
    this->done = false;
    this->fire_period = vector<int>(this->args.TotalYears, 0);
    this->sim = simExt;
    // Initial status grid folder
    if (this->args.OutputGrids || this->args.FinalGrid)
    {
        this->gridFolder = Cell2Fire::createOutputFolder("Grids");
    }

    // Messages Folder
    if (this->args.OutMessages)
    {
        this->messagesFolder = Cell2Fire::createOutputFolder("Messages");
    }
    // Cell Metrics CSV folder (replaces per-metric .asc folders)
    if (this->args.OutRos || this->args.OutFl || this->args.OutIntensity ||
        (this->args.OutCrown && this->args.AllowCROS) ||
        (this->args.OutCrownConsumption && this->args.AllowCROS))
    {
        this->cellStatsFolder = Cell2Fire::createOutputFolder("cellStats");
    }

    chooseWeather(this->args.WeatherOpt, rnumber, simExt);
    // Random ROS-CV
    this->ROSRV = std::abs(rnumber2);
    // std::cout << "ROSRV: " << this->ROSRV << std::endl;

    // Cells flat array: free any ignited cells from the previous simulation
    for (auto& p : this->cells_flat) { delete p; p = nullptr; }

    // Declare an iterator to unordered_map
    std::unordered_map<int, Cells>::iterator it;

    // Reset status
    fTypeCells.assign(this->nCells, 1);
    this->statusCells.assign(this->nCells, 0);
    this->crownState.assign(this->nCells, 0);
    this->crownFraction.assign(this->nCells, 0);
    this->surfFraction.assign(this->nCells, 0);
    this->surfaceIntensities.assign(this->nCells, 0);
    this->crownIntensities.assign(this->nCells, 0);
    this->RateOfSpreads.assign(this->nCells, 0);
    this->surfaceFlameLengths.assign(this->nCells, 0);
    this->crownFlameLengths.assign(this->nCells, 0);
    this->maxFlameLengths.assign(this->nCells, 0);

    this->FSCell.clear();
    int fsCols = (this->args.OutCrown && this->args.AllowCROS) ? 7 : 4;
    this->FSCell.reserve(this->nCells * fsCols * 4);  // 4x: each cell can send multiple messages across periods
    this->crownMetrics.clear();  // intensity and crown

    // Non burnable types: populate relevant fields such as status and ftype
    std::string NoFuel = "NF ";
    std::string NoData = "ND ";
    const char* NF = NoFuel.c_str();
    const char* ND = NoData.c_str();

    for (i = 0; i < this->nCells; i++)
    {
        if (strcmp(df[i].fueltype, NF) == 0 || strcmp(df[i].fueltype, ND) == 0)
        {
            fTypeCells[i] = 0;
            this->statusCells[i] = 4;
        }
    }

    // Relevant sets: Initialization
    this->availCells.clear();
    nonBurnableCells.clear();
    this->burningCells.clear();
    this->burntCells.clear();
    this->harvestCells.clear();

    // Harvest Cells
    for (auto it = HarvestedCells.begin(); it != HarvestedCells.end(); it++)
    {
        for (auto& it2 : it->second)
        {
            fTypeCells[it2 - 1] = 0;

            this->statusCells[it2 - 1] = 3;
        }
    }

    for (i = 0; i < this->statusCells.size(); i++)
    {
        if (this->statusCells[i] < 3)
            this->availCells.insert(i + 1);
        else if (this->statusCells[i] == 4)
            nonBurnableCells.insert(i + 1);
        else if (this->statusCells[i] == 3)
            this->harvestCells.insert(i + 1);
    }

    // Print-out sets information
    if (this->args.verbose)
    {
        printSets(this->availCells, nonBurnableCells, this->burningCells, this->burntCells, this->harvestCells);
    }
}

/**
 * @brief Simulates the ignition phase of the fire spread model for a single
 * simulation year.
 *
 * This method is responsible for selecting ignition points, either randomly or
 * from predefined data (e.g., CSV files), and attempting to ignite those
 * points based on the conditions of the cells (availability, burnability,
 * etc.). The simulation state is updated accordingly, including tracking
 * ignited, burnt, and available cells, as well as the fire and weather
 * periods.
 *
 * @param generator A random engine used to generate random values for ignition
 * point selection.
 * @param ep The current simulation epoch, used to seed the secondary random
 * generator for parallel simulations.
 * @return true If no ignition occurs during the current simulation year.
 * @return false If at least one ignition occurs during the current simulation
 * year.
 *
 * ### Detailed Behavior:
 * - If no ignition points are provided (args.Ignitions == 0), a random point
 * is selected repeatedly until a valid ignition is found or a limit is
 * reached.
 * - If ignition points are provided, the algorithm selects a point directly or
 * samples from a specified radius around the predefined points.
 * - The ignition process updates the state of the forest, including burning
 * and burnt cells, and prepares for the next period or year if no ignition
 * occurs.
 *
 * ### Key Updates:
 * - Ignition history (IgnitionHistory)
 * - Forest cell states (statusCells, burningCells, burntCells, availCells)
 * - Fire and weather periods
 *
 * ### Verbose Mode:
 * When `args.verbose` is true, detailed logs of the ignition process,
 * including selected points, random values, and status updates, are printed to
 * the console.
 */

bool
Cell2Fire::RunIgnition(boost::random::mt19937 generator, int ep)
{
    if (this->args.verbose)
    {
        printf("---------------------- Step 1: Ignition ----------------------\n");
    }
    /*******************************************************************
     *
     *												Step
     *1: Ignition
     *
     *******************************************************************/
    // Ignitions
    int aux = 0;
    int selected = 0;
    int loops = 0;
    int microloops = 0;
    this->noIgnition = false;
    currentSim = currentSim + 1;
    boost::random::mt19937 generator2 = boost::random::mt19937(args.seed * ep * this->nCells);
    boost::random::uniform_int_distribution<int> distribution(1, this->nCells);

    // std::default_random_engine generator2(args.seed * ep * this->nCells);  // * time(NULL)); //creates a different
    //  generator solving cases when parallel
    //  running creates simulations at same time
    // std::uniform_int_distribution<int> distribution(1, this->nCells);

    Cells* ignited_cell = nullptr;  // set when ignition succeeds

    // No Ignitions provided
    if (this->args.Ignitions == 0)
    {
        std::srand(args.seed);
        while (true)
        {
            microloops = 0;
            while (true)
            {
                aux = distribution(generator2);
                float rd_number = (float)rand() / ((float)(RAND_MAX / 0.999999999));
                if (ignProb[aux - 1] > rd_number)
                {
                    break;
                }
                microloops++;
                if (microloops > this->nCells * 100)
                {

                    this->noIgnition = true;
                    break;
                }
            }
            // Check information (Debugging)
            if (this->args.verbose)
            {
                std::cout << "aux: " << aux << std::endl;
            }

            // If cell is available and not initialized, initialize it
            if (this->statusCells[aux - 1] < 2)
            {
                if (!this->cells_flat[aux - 1])
                    InitCell(aux);

                Cells* cell_ptr = this->cells_flat[aux - 1];
                if (cell_ptr->getStatus() == "Available" && cell_ptr->fType != 0)
                {
                    IgnitionHistory[sim] = aux;
                    // std::cout << "Selected (Random) ignition point: " << aux << std::endl;
                    std::vector<int> ignPts = { aux };
                    if (cell_ptr->ignition(this->fire_period[year - 1],
                                           this->year,
                                           ignPts,
                                           &df[aux - 1],
                                           this->coef_ptr,
                                           this->args_ptr,
                                           &(this->wdf[this->weatherPeriod]),
                                           this->activeCrown,
                                           this->perimeterCells))
                    {
                        // Printing info about ignitions
                        if (this->args.verbose)
                        {
                            std::cout << "Cell " << cell_ptr->realId << " Ignites" << std::endl;
                            std::cout << "Cell " << cell_ptr->realId << " Status: " << cell_ptr->getStatus()
                                      << std::endl;
                        }

                        // Status
                        this->statusCells[cell_ptr->realId - 1] = 1;
                        ignited_cell = cell_ptr;

                        // Plotter placeholder
                        if (this->args.OutputGrids)
                        {
                            this->outputGrid();
                        }

                        break;
                    }
                }
            }

            loops++;
            if (loops > this->nCells * 100)
            {
                this->noIgnition = true;
                break;
            }
        }
    }

    // Ignitions with provided points from CSV
    else
    {
        int temp = IgnitionPoints[this->year - 1];

        // If ignition Radius != 0, sample from the Radius set
        if (this->args.IgnitionRadius > 0)
        {
            // Pick any at random and set temp with that cell
            boost::random::uniform_int_distribution<int> udistribution(0,
                                                                       this->IgnitionSets[this->year - 1].size() - 1);
            temp = this->IgnitionSets[this->year - 1][udistribution(generator)];
        }

        // std::cout << "Selected ignition point: " << temp << std::endl;
        //  this->
        IgnitionHistory[sim] = temp;

        // If cell is available
        if (this->statusCells[temp - 1] < 2)
        {
            if (!this->cells_flat[temp - 1])
                InitCell(temp);

            Cells* cell_ptr = this->cells_flat[temp - 1];

            // Not available or non burnable: no ignition
            if (cell_ptr->getStatus() != "Available" || cell_ptr->fType == 0)
            {
                this->noIgnition = true;
            }

            // Available and Burnable: ignition
            if (cell_ptr->getStatus() == "Available" && cell_ptr->fType != 0)
            {
                std::vector<int> ignPts = { temp };
                if (cell_ptr->ignition(this->fire_period[year - 1],
                                       this->year,
                                       ignPts,
                                       &df[temp - 1],
                                       this->coef_ptr,
                                       this->args_ptr,
                                       &(this->wdf[this->weatherPeriod]),
                                       this->activeCrown,
                                       this->perimeterCells))
                {
                    // Printing info about ignitions
                    if (this->args.verbose)
                    {
                        std::cout << "Cell " << cell_ptr->realId << " Ignites" << std::endl;
                        std::cout << "Cell " << cell_ptr->realId << " Status: " << cell_ptr->getStatus() << std::endl;
                    }

                    // Status
                    this->statusCells[cell_ptr->realId - 1] = 1;
                    ignited_cell = cell_ptr;
                }
            }
        }
        else
        {
            this->noIgnition = true;
            this->weatherPeriod = 0;
        }
    }

    // If ignition occurs, we update the forest status
    if (!this->noIgnition && ignited_cell)
    {
        int newId = ignited_cell->realId;
        if (this->args.verbose)
        std:
            cout << "New ID for burning cell: " << newId << std::endl;

        this->nIgnitions++;
        this->burningCells.insert(newId);
        this->burntCells.insert(newId);
        this->statusCells[newId - 1] = 2;
        this->availCells.erase(newId);

        // Print sets information
        if (this->args.verbose)
        {
            printSets(
                this->availCells, nonBurnableCells, this->burningCells, this->burntCells, this->harvestCells);
        }
    }

    // Plotter placeholder
    if (this->args.OutputGrids)
    {
        // DEBUGstd::cout << "Grid post ignition" << std::endl;
        this->outputGrid();
    }

    // Next period
    // this->fire_period[year - 1] += 1;   // DEBUGGING!!!
    if (this->args.verbose)
    {
        std::cout << "Fire period updated to " << this->fire_period[year - 1] << std::endl;
    }

    // Check weather period consistency
    updateWeather();

    // Print-out information regarding the weather and fire period
    if (this->args.verbose)
    {
        std::cout << "Fire Period Starts: " << this->fire_period[year - 1] << std::endl;
        std::cout << "\nCurrent weather conditions:" << std::endl;
        this->CSVWeather.printWeatherDF(this->wdf[this->weatherPeriod]);

        if (this->args.WeatherOpt.compare("constant") == 0)
            std::cout << "(NOTE: current weather is not used for ROS with "
                         "constant option)"
                      << std::endl;

        // End of the ignition step
        std::cout << "\nNext Fire Period: " << this->fire_period[year - 1] << std::endl;
    }

    // If no ignition occurs, go to next year (no multiple ignitions per year,
    // only one)
    if (this->noIgnition)
    {
        if (this->args.verbose)
        {
            std::cout << "No ignition" << std::endl;
            std::cout << "------------------------------------------------------"
                         "-------------------\n"
                      << std::endl;
        }

        // Next year
        this->year += 1;
    }

    // std::cout << endl << "el punto de ignicion es: " << aux << std::endl;

    return this->noIgnition;
}

/**
 * @brief  Message-sending phase of the fire spread model, where burning cells
 * propagate fire messages to neighboring cells.
 *
 * This method iterates through all burning cells, updates their fire progress,
 * and determines whether fire messages should be sent to neighboring cells. A
 * fire message is sent to a cell if fire reaches its center. Burnt-out cells
 * are also identified and removed from the burning set.
 *
 * @return A map of cell IDs to lists of neighboring cell IDs that received
 * fire messages. Each key is the ID of a burning cell, and the value is a list
 * of neighboring cell IDs affected by the fire.
 *
 * ### Detailed Behavior:
 * - Iterates over all currently burning cells to manage fire propagation using
 * either the `manageFire` or `manageFireBBO` methods, depending on tuning
 * options.
 * - Builds a list of neighboring cells (`sendMessageList`) that receive fire
 * messages for potential ignition.
 * - Tracks cells that burn out during this phase and removes them from the
 * active burning set.
 * - Handles special conditions where repeat fire propagation may occur.
 *
 * ### Key Updates:
 * - `burningCells`: Updated to exclude cells that have burnt out.
 * - `burnedOutList`: Contains cells that are no longer burning after this
 * phase.
 * - `messagesSent`: A flag indicating if any fire messages were sent during
 * this phase.
 *
 * ### Verbose Mode:
 * When `args.verbose` is enabled:
 * - Prints detailed logs of the fire progress and messages sent by each
 * burning cell.
 * - Outputs the current fire period, sets of cells (available, non-burnable,
 * burning, burnt, harvested), and fire message details.
 *
 * ### Warning:
 * - A warning is issued if the fire period approaches the maximum allowed
 * (`args.MaxFirePeriods`).
 */
std::unordered_map<int, std::vector<int>>
Cell2Fire::SendMessages()
{
    // Clean list
    this->burnedOutList.clear();

    // Check ending
    if (fire_period[year - 1] == this->totalFirePeriods - 1 && this->args.verbose)
    {
        std::cout << "*** WARNING!!! About to hit MaxFirePeriods: " << this->totalFirePeriods << std::endl;
    }

    /// Send messages logic
    this->messagesSent = false;
    std::unordered_map<int, vector<int>> sendMessageList;

    // Repeat fire flag
    this->repeatFire = false;

    // Printing info before sending messages
    if (this->args.verbose)
    {
        std::cout << "\n---------------------- Step 2: Sending Messages from "
                     "Ignition ----------------------"
                  << std::endl;
        std::cout << "Current Fire Period:" << this->fire_period[this->year - 1] << std::endl;
        printSets(this->availCells, nonBurnableCells, this->burningCells, this->burntCells, this->harvestCells);
    }

    /*
                    Potential parallel zone: Send messages
                    Burning cells loop: sending messages (Embarrasingly
       parallel?: CP: should be) Each burning cell updates its fire progress and
       (if needed) populates their message
    */
    for (int cell : this->burningCells)
    {
        std::vector<int> aux_list;
        Cells* cell_ptr = this->cells_flat[cell - 1];

        // Cell's info
        if (this->args.verbose)
            cell_ptr->print_info();

        /*
                        Manage Fire method main step
        */
        if (cell_ptr->hasAvailableNeighbor())
        {
            // std::cout << "Entra a Manage Fire" << std::endl;
            aux_list = cell_ptr->manageFire(this->fire_period[this->year - 1],
                                            this->availCells,
                                            df,
                                            this->coef_ptr,
                                            coordCells,
                                            this->args_ptr,
                                            &this->wdf[this->weatherPeriod],
                                            &this->FSCell,
                                            &this->crownMetrics,
                                            this->activeCrown,
                                            this->ROSRV,
                                            this->perimeterCells,
                                            this->crownState,
                                            this->crownFraction,
                                            this->surfFraction,
                                            this->surfaceIntensities,
                                            this->RateOfSpreads,
                                            this->surfaceFlameLengths,
                                            this->crownFlameLengths,
                                            this->crownIntensities,
                                            this->maxFlameLengths);
            // std::cout << "Sale de Manage Fire" << std::endl;
        }

        else
        {
            if (this->args.verbose)
                std::cout << "\nCell " << cell
                          << " does not have any neighbor available for receiving "
                             "messages"
                          << std::endl;
        }

        // If message and not a true flag
        if (aux_list.size() > 0 && aux_list[0] != -100)
        {
            if (this->args.verbose)
                std::cout << "\nList is not empty" << std::endl;
            this->messagesSent = true;
            sendMessageList[cell_ptr->realId] = aux_list;
            if (this->args.verbose)
            {
                std::cout << "Message list content" << std::endl;
                for (auto& msg : sendMessageList[cell_ptr->realId])
                {
                    std::cout << "  Fire reaches the center of the cell " << msg
                              << "  Distance to cell (in meters) was 100.0"
                              << " " << std::endl;
                }
            }
        }

        // Repeat fire conditions if true flag
        if (aux_list.size() > 0 && aux_list[0] == -100)
        {
            this->repeatFire = true;
        }

        // Burnt out inactive burning cells
        if (aux_list.size() == 0)
        {
            // not parallel here
            this->burnedOutList.push_back(cell_ptr->realId);
            if (this->args.verbose)
            {
                std::cout << "\nMessage and Aux Lists are empty; adding to "
                             "BurnedOutList"
                          << std::endl;
            }
        }
    }
    /* End sending messages loop */

    // Check for burnt out updates via sets' difference
    for (auto& bc : this->burnedOutList)
    {
        auto lt = this->burningCells.find(bc);
        if (lt != this->burningCells.end())
        {
            this->burningCells.erase(bc);
        }
    }
    if (this->args.verbose)
        printSets(this->availCells, nonBurnableCells, this->burningCells, this->burntCells, this->harvestCells);

    return sendMessageList;
}

/**
 * @brief Processes incoming fire messages to determine the ignition and
 * progression of fire in cells.
 *
 * This method handles the reception and processing of messages sent by burning
 * cells. Based on the received messages, cells may ignite, continue burning,
 * or be marked as burnt out. The method also manages the transitions between
 * fire periods and updates relevant cell sets (e.g., available, burning,
 * burnt).
 *
 * @param sendMessageList A map where each key is the ID of a burning cell, and
 * the associated value is a list of neighboring cell IDs that received fire
 * messages from that cell.
 *
 * ### Detailed Behavior:
 *
 * - For each cell receiving messages:
 *   - Initializes the cell if it hasn't been processed before.
 *   - Checks ignition conditions and marks the cell as burnt if applicable.
 *   - Updates fire-related metadata, ensuring fire does not propagate back to
 * the sender cell.
 *
 * ### Key Updates:
 * - `burntCells`: Updated with cells that are fully burnt.
 * - `burningCells`: Updated with newly ignited cells.
 * - `availCells`: Reduced by removing newly burning cells.
 * - `fire_period`: Incremented at the end of each fire period.
 * - `year`: Incremented if no messages are sent and `repeatFire` is false.
 *
 * ### Verbose Mode:
 * When `args.verbose` is enabled:
 * - Logs detailed information about fire messages, ignition, and cell state
 * transitions.
 * - Prints the current state of cell sets (available, non-burnable, burning,
 * burnt, harvested).
 *
 * ### Weather Updates:
 * - Calls `updateWeather` to apply weather conditions for the next fire
 * period.
 * - Provides a note if the weather is constant, indicating its effect on the
 * fire model.
 */
void
Cell2Fire::GetMessages(const std::unordered_map<int, std::vector<int>>& sendMessageList)
{
    // Iterator
    // Information of the current step
    if (this->args.verbose)
    {
        std::cout << "\n---------------------- Step 3: Receiving and processing "
                     "messages from Ignition ----------------------"
                  << std::endl;
        std::cout << "Current Fire Period: " << this->fire_period[this->year - 1] << std::endl;
        printSets(this->availCells, nonBurnableCells, this->burningCells, this->burntCells, this->harvestCells);
    }

    // Conditions depending on number of messages and repeatFire flag
    // No messages but repetition
    if (this->repeatFire && !this->messagesSent)
    {
        if (this->args.verbose)
        {
            std::cout << "Fires are still alive, no message generated" << std::endl;
            std::cout << "Current fire period: " << this->fire_period[this->year - 1] << std::endl;
        }
        this->fire_period[this->year - 1] += 1;

        // Check if we need to update the weather
        this->updateWeather();
    }

    // Messages and repetition
    if (this->repeatFire && this->messagesSent)
    {
        if (this->args.verbose)
            std::cout << "Messages have been sent, next step. Current period: " << this->fire_period[this->year - 1]
                      << std::endl;
        this->repeatFire = false;
    }

    // No messages, no repeat (break and next year)
    if (!this->messagesSent && !this->repeatFire)
    {
        if (this->args.verbose)
        {
            std::cout << "\nNo messages during the fire period, end of year " << this->year << std::endl;
        }
        // Next year, reset weeks, weather period, and update burnt cells from
        // burning cells
        this->year += 1;
        this->weatherPeriod = 0;
        this->noMessages = true;
        // Update sets
        for (auto& bc : this->burningCells)
        {
            this->burntCells.insert(bc);
            this->statusCells[bc - 1] = 2;
        }
        this->burningCells.clear();
        if (this->args.verbose)
            printSets(
                this->availCells, nonBurnableCells, this->burningCells, this->burntCells, this->harvestCells);
    }

    // Mesages and no repeat
    if (this->messagesSent && !this->repeatFire)
    {

        // frequency array
        std::unordered_map<int, int> globalMessagesList;
        for (auto& sublist : sendMessageList)
        {
            for (int val : sublist.second)
            {
                if (globalMessagesList.find(val) == globalMessagesList.end())
                {
                    globalMessagesList[val] = 1;
                }
                else
                {
                    globalMessagesList[val] = globalMessagesList[val] + 1;
                }
            }
        }

        // Initialize cells if needed (getting messages)
        for (auto& _bc : globalMessagesList)
        {
            int bc = _bc.first;
            if (!this->cells_flat[bc - 1] && this->statusCells[bc - 1] < 2)
                InitCell(bc);
        }

        // Get burnt loop
        if (this->args.verbose)
        {
            for (auto& _bc : globalMessagesList)
            {
                printf("CELL %d inside global message list \n", _bc.first);
            }
        }

        std::unordered_set<int> burntList;
        bool checkBurnt;
        for (auto& _bc : globalMessagesList)
        {
            // printf("\n\nWE ARE DEBUGGING!!!! CELL TO BE ANALYZED GET BURNT IS
            // %d\n", _bc.first);
            int bc = _bc.first;
            if (this->statusCells[bc - 1] < 2)
            {
                if (!this->cells_flat[bc - 1])
                    InitCell(bc);

                Cells* bc_ptr = this->cells_flat[bc - 1];

                // Check if burnable, then check potential ignition
                if (bc_ptr->fType != 0)
                {
                    checkBurnt = bc_ptr->get_burned(this->fire_period[this->year - 1],
                                                    1,
                                                    this->year,
                                                    df,
                                                    this->coef_ptr,
                                                    this->args_ptr,
                                                    &this->wdf[this->weatherPeriod],
                                                    this->activeCrown,
                                                    this->perimeterCells);
                }
                else
                {
                    checkBurnt = false;
                }

                // Print-out regarding the burnt cell
                if (this->args.verbose)
                {
                    std::cout << "\nCell " << bc_ptr->realId << " got burnt (1 true, 0 false): " << checkBurnt
                              << std::endl;
                }

                // Update the burntlist
                if (checkBurnt)
                {
                    burntList.insert(bc_ptr->realId);

                    // Cleaning step: fire can't propagate back into the newly burnt cell
                    for (int i = 0; i < bc_ptr->nb_count; i++)
                    {
                        int origToNew = bc_ptr->nb_angles[i];
                        int newToOrig = (origToNew + 180) % 360;
                        int adjCellNum = bc_ptr->nb_ids[i];
                        Cells* adj_ptr = (adjCellNum > 0 && adjCellNum <= this->nCells)
                                             ? this->cells_flat[adjCellNum - 1]
                                             : nullptr;
                        if (adj_ptr)
                        {
                            int backSlot = adj_ptr->slotByAngle(newToOrig);
                            if (backSlot >= 0)
                                adj_ptr->nb_available[backSlot] = false;
                        }
                    }
                }
            }
        }

        // Update sets
        for (auto& bc : burntList)
        {
            this->burntCells.insert(bc);
            this->statusCells[bc - 1] = 2;
        }

        for (auto& bc : this->burnedOutList)
        {
            this->burntCells.insert(bc);
            this->statusCells[bc - 1] = 2;
        }

        for (auto& bc : burntList)
        {
            this->burningCells.insert(bc);
        }

        for (auto& bc : burningCells)
        {
            auto lt = this->availCells.find(bc);
            if (lt != this->availCells.end())
            {
                this->availCells.erase(bc);
            }
        }

        // Display info for debugging
        if (this->args.verbose)
        {
            printSets(
                this->availCells, nonBurnableCells, this->burningCells, this->burntCells, this->harvestCells);
        }

        /*
         *					Next Period
         */
        this->fire_period[this->year - 1] += 1;

        // If weather is not constant (random weather to be supported in next
        // commit...)
        this->updateWeather();

        // Message for constant weather
        if (this->args.WeatherOpt.compare("constant") == 0 && this->args.verbose)
        {
            std::cout << "(NOTE: current weather is not used for ROS with "
                         "constant option)"
                      << std::endl;
        }
    }
}

/**
 * @brief Generates and outputs simulation results, including fire behavior
 * metrics, final grid status, and logs.
 *
 * This method processes the results of the fire simulation, updating cell
 * statuses and generating various outputs such as grid data, fire spread
 * metrics, and intensity logs. It supports multiple file formats (e.g., CSV,
 * ASCII) and organizes output into designated folders for easy analysis and
 * debugging.
 *
 * ### Behavior:
 * - Updates the status of cells:
 *   - `burntCells` and `burningCells` are marked as fully burnt (`status =
 * 2`).
 * - Computes summary statistics:
 *   - Reports the number and percentage of available, burnt, non-burnable, and
 * firebreak cells.
 * - Writes outputs to files:
 *   - **Grid Status**: Outputs the final grid state to a CSV file if
 * `args.FinalGrid` is enabled.
 *   - **Network Messages**: Logs fire message data to a CSV file if
 * `args.OutMessages` is enabled.
 *   - **Fire Spread Metrics**:
 *     - Rate of Spread (ROS)
 *     - Byram Intensity
 *     - Flame Length
 *     - Crown Fraction Burn
 *     - Surface Fraction Burn
 *     - Crown Fire Behavior
 * - **Ignition Log**: Logs ignition history if `args.IgnitionsLog` is enabled.
 *
 * ### Outputs:
 * - The output files are organized into subfolders under the designated output
 * directory (`args.OutFolder`).
 * - Naming conventions include simulation IDs (`sim`) to ensure unique file
 * names.
 * - File types:
 *   - CSV for grids and messages.
 *   - ASCII for rate of spread, intensity, flame length, and other fire
 * behavior metrics.
 *
 *
 * ### Example Output:
 * ```
 * Simulation 3 Results:
 *         Cell Status            Count     Percent
 *         ----------------------------------------
 *         Available               2916      45.56%
 *         Burnt                   3456      54.00%
 *         Non-Burnable              28       0.44%
 *         Firebreak                  0       0.00%
 *         Total                   6400     100.00%
 * ```
 *
 * ### Notes:
 * - Ensure the output directory structure exists before running the simulation
 * to avoid file I/O errors.
 * - This method supports optional outputs, which can be toggled using
 * command-line arguments.
 */
void
Cell2Fire::Results()
{
    /*****************************************************************************
     *
     *												Steps
     *4: Results and outputs
     *
     ******************************************************************************/
    for (auto& br : this->burntCells)
    {
        if (this->cells_flat[br - 1])
            this->cells_flat[br - 1]->status = 2;
    }

    for (auto& br : this->burningCells)
    {
        if (this->cells_flat[br - 1])
            this->cells_flat[br - 1]->status = 2;
    }

    // Final results for comparison with Python
    // DEBUGstd::cout  << "\n ------------------------ Final results for
    // comparison with Python ------------------------";

    // Final report
    float ACells = this->availCells.size();
    float BCells = this->burntCells.size();
    float NBCells = nonBurnableCells.size();
    float HCells = this->harvestCells.size();

    std::cout << "\nSimulation " << this->sim << " Results:\n"
              << "\t" << std::left << std::setw(16) << "Cell Status" << std::right << std::setw(12) << "Count"
              << std::right << std::setw(12) << "Percent"
              << "\n";
    std::cout << "\t" << std::string(40, '-') << "\n";
    std::cout << "\t" << std::left << std::setw(16) << "Available" << std::right << std::setw(12)
              << static_cast<int>(ACells) << std::right << std::setw(11) << std::fixed << std::setprecision(2)
              << ACells / nCells * 100.0 << "%\n";
    std::cout << "\t" << std::left << std::setw(16) << "Burnt" << std::right << std::setw(12)
              << static_cast<int>(BCells) << std::right << std::setw(11) << std::fixed << std::setprecision(2)
              << BCells / nCells * 100.0 << "%\n";
    std::cout << "\t" << std::left << std::setw(16) << "Non-Burnable" << std::right << std::setw(12)
              << static_cast<int>(NBCells) << std::right << std::setw(11) << std::fixed << std::setprecision(2)
              << NBCells / nCells * 100.0 << "%\n";
    std::cout << "\t" << std::left << std::setw(16) << "Firebreak" << std::right << std::setw(12)
              << static_cast<int>(HCells) << std::right << std::setw(11) << std::fixed << std::setprecision(2)
              << HCells / nCells * 100.0 << "%\n";
    std::cout << "\t" << std::left << std::setw(16) << "Total" << std::right << std::setw(12)
              << static_cast<int>(nCells) << std::right << std::setw(11) << std::fixed << std::setprecision(2) << 100.0
              << "%\n";
    // Final Grid
    if (this->args.FinalGrid)
    {
        CSVWriter CSVFolder("", "");
        if (this->args.OutFolder.empty())
            this->gridFolder = this->args.InFolder + "simOuts" + separator() + "Grids" + separator() + "Grids"
                               + std::to_string(this->sim) + separator();

        // std::string gridName = this->gridFolder + "FinalStatus_" +
        // std::to_string(this->sim) + ".csv";
        outputGrid();
    }

    // Messages
    if (this->args.OutMessages)
    {
        std::string messagesName;
        std::ostringstream oss;
        oss.str("");
        oss << std::setfill('0') << std::setw(this->widthSims) << this->sim;
        messagesName = this->messagesFolder + "MessagesFile" + oss.str() + ".csv";
        if (this->args.verbose)
        {
            std::cout << "We are generating the network messages to a csv file " << messagesName << std::endl;
        }
        CSVWriter CSVPloter(messagesName, ",");
        int fsCols = (this->args.OutCrown && this->args.AllowCROS) ? 7 : 4;
        CSVPloter.printCSVDouble_V2(this->FSCell.size() / fsCols, fsCols, this->FSCell);
    }

    // Cell Metrics CSV — one row per burned cell, columns depend on active flags
    if (this->args.OutRos || this->args.OutFl || this->args.OutIntensity ||
        (this->args.OutCrown && this->args.AllowCROS) ||
        (this->args.OutCrownConsumption && this->args.AllowCROS))
    {
        std::ostringstream oss;
        oss << std::setfill('0') << std::setw(this->widthSims) << this->sim;
        std::string metricsName = this->cellStatsFolder + "cellStats" + oss.str() + ".csv";

        std::ofstream mfs(metricsName);
        // header
        mfs << "cell_id";
        if (this->args.OutRos)                                          mfs << ",ros";
        if (this->args.OutFl)                                           mfs << ",fl";
        if (this->args.OutIntensity)                                    mfs << ",intensity";
        if (this->args.OutCrownConsumption && this->args.AllowCROS)     mfs << ",cfb";
        if (this->args.OutCrown && this->args.AllowCROS)                mfs << ",crown";
        mfs << "\n";
        // rows — only burned cells (RateOfSpreads > 0 always for burned cells)
        for (int i = 0; i < this->nCells; i++)
        {
            if (this->RateOfSpreads[i] == 0) continue;
            mfs << (i + 1);
            if (this->args.OutRos)                                      mfs << "," << this->RateOfSpreads[i];
            if (this->args.OutFl)                                       mfs << "," << this->surfaceFlameLengths[i];
            if (this->args.OutIntensity)                                mfs << "," << this->surfaceIntensities[i];
            if (this->args.OutCrownConsumption && this->args.AllowCROS) mfs << "," << this->crownFraction[i];
            if (this->args.OutCrown && this->args.AllowCROS)            mfs << "," << this->crownState[i];
            mfs << "\n";
        }
        mfs.close();
    }

    // Ignition Logfile
    if (currentSim == args.TotalSims && this->args.IgnitionsLog)
    {

        if (this->args.verbose)
        {
            std::cout << "\nWriting (simulation, cell & weather ids): ";
            for (int i = 1; i < (int)IgnitionHistory.size() + 1; i++)
            {
                std::cout << "(" << i << "," << IgnitionHistory[i] << "," << WeatherHistory[i] << ")\t";
            }
        }

        CSVWriter ignitionsFile(this->args.OutFolder + sim_log_filename);
        ignitionsFile.printIgnitions(IgnitionHistory, WeatherHistory);
        IgnitionHistory.clear();
        WeatherHistory.clear();
    }

    // Per-simulation fire stats
    if (this->args.Stats)
    {
        float sum_sfl = 0.0f, sum_cfl = 0.0f, max_fl = 0.0f;
        int count = 0;
        for (int i = 0; i < (int)this->nCells; i++)
        {
            if (this->RateOfSpreads[i] == 0) continue;
            sum_sfl += this->surfaceFlameLengths[i];
            sum_cfl += this->crownFlameLengths[i];
            if (this->maxFlameLengths[i] > max_fl)
                max_fl = this->maxFlameLengths[i];
            count++;
        }
        float mean_sfl = count > 0 ? sum_sfl / count : 0.0f;
        float mean_cfl = count > 0 ? sum_cfl / count : 0.0f;
        simStats[this->sim - 1] = {mean_sfl, mean_cfl, max_fl};
    }
}

/**
 * @brief Outputs the current state of the forest grid to a CSV file.
 *
 * This method generates a binary representation of the forest grid, with cell
 * statuses indicating their condition (burning, burnt, harvested, etc.) during
 * the simulation. The file is saved with a unique name in the designated
 * output folder.
 *
 * ### Behavior:
 * - **Cell Status Update**:
 *   - Updates a vector (`statusCells2`) to reflect the current statuses of
 * cells:
 *     - `1` for burning or burnt cells.
 *     - `-1` for harvested cells.
 *     - `0` for all other cells.
 * - **File Naming**:
 *   - Constructs a file name based on the simulation's `gridNumber` and saves
 * it in the `gridFolder` directory.
 *   - File names follow the pattern: `ForestGrid<gridNumber>.csv`.
 * - **Verbose Mode**:
 *   - Logs the file name and output process to the console if `args.verbose`
 * is enabled.
 * - **Output Format**:
 *   - The CSV file represents the forest grid in rows and columns, with each
 * cell's status stored in `statusCells2`.
 *
 * ### Outputs:
 * - A CSV file is saved in the `gridFolder` directory, with the grid's state
 * for the current simulation step.
 * - Example CSV content:
 *   ```
 *   0, 0, 1, 0
 *   0, -1, 0, 1
 *   1, 0, 0, 0
 *   ```
 *
 * ### Notes:
 * - Ensure the `gridFolder` directory exists before calling this method to
 * avoid file I/O errors.
 * - The method automatically increments `gridNumber` after each call to ensure
 * unique file names.
 */
void
Cell2Fire::outputGrid()
{
    // FileName
    std::string gridName;
    std::vector<int> statusCells2(this->nCells, 0);  //(long int, int);

    // Update status
    for (auto& bc : this->burningCells)
    {
        statusCells2[bc - 1] = 1;
    }
    for (auto& ac : this->burntCells)
    {
        statusCells2[ac - 1] = 1;
    }
    for (auto& hc : this->harvestCells)
    {
        statusCells2[hc - 1] = -1;
    }
    std::ostringstream oss;
    oss.str("");
    oss << std::setfill('0') << std::setw(this->widthSims) << this->gridNumber;
    gridName = this->gridFolder + "ForestGrid" + oss.str() + ".csv";
    if (this->args.verbose)
    {
        std::cout << "We are plotting the current forest to a csv file " << gridName << std::endl;
    }

    CSVWriter CSVPloter(gridName, ",");
    CSVPloter.printCSV(this->rows, this->cols, statusCells2);
    this->gridNumber++;
}

/**
 * @brief Updates the weather conditions during the simulation based on
 * specified criteria.
 *
 * This method dynamically updates the weather parameters used in the
 * simulation based on the simulation's fire period and weather options. It can
 * optionally output the current state of the forest grid and log weather
 * updates if verbose mode is enabled.
 *
 * ### Behavior:
 * - **Weather Update Check**:
 *   - Weather is updated if:
 *     - The `WeatherOpt` argument is not set to `"constant"`.
 *     - The fire period multiplied by the fire period length exceeds the
 * current `weatherPeriod`.
 *   - When the conditions are met, the `weatherPeriod` is incremented.
 * - **Grid Output**:
 *   - If the `OutputGrids` argument is enabled, the current forest grid is
 * output using the `outputGrid` method.
 * - **Verbose Logging**:
 *   - If `verbose` mode is enabled:
 *     - Logs the weather update to the console.
 *     - Prints detailed weather information for the current period using
 * `CSVWeather.printWeatherDF`.
 */
void
Cell2Fire::updateWeather()
{
    if (this->args.WeatherOpt != "constant"
        && this->fire_period[this->year - 1] * this->args.FirePeriodLen / this->args.MinutesPerWP
               > this->weatherPeriod + 1)
    {
        this->weatherPeriod++;
        if (this->args.OutputGrids)
        {
            this->outputGrid();
        }
        if (this->args.verbose)
        {
            std::cout << "\nWeather has been updated" << std::endl;
            this->CSVWeather.printWeatherDF(this->wdf[this->weatherPeriod]);
        }
    }
}

/**
 * @brief Executes a single simulation step in the forest fire model.
 *
 * This method simulates one fire period, handling all related processes such
 * as ignition, message exchange, weather updates, and transitioning between
 * simulation years or scenarios. It is the core operational logic for
 * advancing the simulation in discrete steps.
 *
 * ### Behavior:
 * - **Verbose Logging**:
 *   - If `verbose` is enabled, prints details about the current simulation
 * state, including the year, fire period, weather period, and grid status.
 *
 * - **Simulation Logic**:
 *   - **End of Simulation Checks**:
 *     - Ends the simulation if the current year exceeds the total years
 * (`TotalYears`).
 *     - If the available cells or burning cells are empty, the simulation is
 * marked as complete.
 *   - **Ignition**:
 *     - Handles ignition logic during the first fire period. If no ignition
 * occurs, checks for available burning cells.
 *   - **Fire Spread**:
 *     - Processes fire spread by sending and receiving messages between cells.
 *   - **Weather Updates**:
 *     - Resets the weather period at the start of each new year.
 *   - **Fire Period Limits**:
 *     - If the current fire period exceeds the maximum allowed fire periods
 * (`MaxFirePeriods`), advances to the next year.
 *   - **Results Handling**:
 *     - Outputs results and weather history if required.
 *
 * - **Transition to Next Simulation**:
 *   - Advances to the next simulation if the maximum year or conditions for
 * termination are met.
 *   - Handles additional simulation-specific logic based on weather options
 * and history requirements.
 *
 */
void
Cell2Fire::Step(boost::random::mt19937 generator, int ep)
{
    // Iterator
    // Declare an iterator to unordered_map
    std::unordered_map<int, Cells>::iterator it;
    bool auxC = false;
    this->noMessages = false;
    // Conditions entering the step
    if (this->args.verbose)
    {
        std::cout << "********************************************" << std::endl;
        std::cout << "Year: " << this->year << std::endl;
        std::cout << "Fire Period: " << this->fire_period[this->year - 1] << std::endl;
        std::cout << "WeatherPeriod: " << this->weatherPeriod << std::endl;
        std::cout << "MaxFirePeriods: " << this->totalFirePeriods << std::endl;
        printSets(this->availCells, nonBurnableCells, this->burningCells, this->burntCells, this->harvestCells);
        std::cout << "********************************************" << std::endl;
    }
    // One step (one fire period, ignition - if needed -, sending messages and
    // receiving them - if needed) For completeness: just in case user runs it
    // longer than the horizon (should not happen)
    if (this->year > this->args.TotalYears)
    {
        if (this->args.verbose)
        {
            printf("\nYear is greater than the Horizon, no more steps");
        }
        this->done = true;
        // Print-out results to folder
        if (this->args.verbose)
            this->Results();

        // Next Sim
        this->sim += 1;
    }
    // Info
    if (this->args.verbose)
    {
        std::cout << "\nSimulating year" << this->year << "\nOut of totalYears:" << this->args.TotalYears;
    }
    // New operational step (ONE fire period)
    if (this->fire_period[this->year - 1] > 0 && !this->done)
    {
        // Fire Spread (one time step of RL - Operational)
        // Send messages after ignition (does not advance time!)
        std::unordered_map<int, std::vector<int>> SendMessageList = this->SendMessages();
        // Get Message
        this->GetMessages(SendMessageList);
    }
    // Operational dynamic
    // Ignition if we are in the first period (added workaround for no Messages)
    if (!this->done && !this->noMessages)
    {
        if (this->fire_period[this->year - 1] == 0)
        {
            // std::cout << "Entra a Ignition step 0" << std::endl;
            //  Continue only if ignition  - No ignition (True): done
            if (this->RunIgnition(generator, ep))
            {
                // Next year
                this->weatherPeriod = 0;
                // If more than planning horizon, next sim
                if (this->year > this->args.TotalYears)
                {
                    // Print-out results to folder
                    this->Results();
                    // Next Sim if max year
                    this->done = true;
                }
            }
            else
            {
                cout << "\nSimulation " << this->sim << " Start:"
                     << "\n\tweather file: " << WeatherHistory[this->sim]
                     << "\n\tignition cell: " << IgnitionHistory[this->sim]
                     << "\n\tmax time periods: " << this->totalFirePeriods << endl;

                // Start sending messages
                std::unordered_map<int, std::vector<int>> SendMessageList = this->SendMessages();
                // Get Message
                this->GetMessages(SendMessageList);
                // Check if no burning cells to stop
                if (this->burningCells.size() == 0)
                {
                    // Next Sim if max year
                    this->done = true;
                }
            }
        }
    }
    // Ending conditions
    if (this->fire_period[std::min(this->year, int(fire_period.size())) - 1] >= this->totalFirePeriods)
    {
        // Extra breaking condition: Max fire periods then go to next year
        if (this->args.verbose)
        {
            printf("\nNext year due to max periods...\n");
        }
        // Next Year/Season update
        this->weatherPeriod = 0;
        auxC = true;
        for (auto& bc : burningCells)
        {
            auto lt = this->availCells.find(bc);
            if (lt != this->availCells.end())
            {
                this->availCells.erase(bc);
            }
            this->burntCells.insert(bc);
            this->statusCells[bc - 1] = 2;
        }
        this->burningCells.clear();
    }
    // If more than planning horizon, next sim
    if (this->year > this->args.TotalYears)
    {
        // printf("\n\nEntra a year mayor al total...\n\n");
        //  Print-out results to folder
        this->Results();
        // Next Sim if max year
        this->sim += 1;
        this->done = true;
    }
    // Done flag (extra condition: no available cells or death of the team)
    if ((this->availCells.size() == 0) || (this->burningCells.size() == 0 && !this->noMessages) && !this->done && !auxC)
    {
        // Done
        this->done = true;
        // Print-out results to folder
        this->Results();
        // Next Sim if max year
        this->sim += 1;
    }

    // Print current status
    if (!this->done && this->args.verbose)
    {
        printf("\nFire Period: %d", this->fire_period[this->year - 1]);
        printf("\nYear: %d \n\n", this->year);
    }
}

void
Cell2Fire::chooseWeather(const string& weatherOpt, int rnumber, int simExt)
{
    string weatherFilename;
    int WPeriods;
    if (weatherOpt == "rows")
    {
        weatherFilename = this->args.InFolder + "Weather" + ".csv";
        try
        {
            this->WeatherData = this->CSVWeather.getData(weatherFilename);
        }
        catch (std::invalid_argument& e)
        {
            std::cerr << e.what() << std::endl;
            std::abort();
        }
        // std::cout << "\nWeather DataFrame from instance " << this->CSVWeather.fileName << std::endl;

        WPeriods = WeatherData.size() - 1;  // -1 due to header
        // this->wdf_ptr = &(this->wdf[0]);

        // Populate the wdf objects
        this->CSVWeather.parseWeatherDF(this->wdf, this->WeatherData, WPeriods);
    }
    else if (weatherOpt == "random")
    {
        // Random Weather
        weatherFilename = this->args.InFolder + "Weathers" + separator() + "Weather" + std::to_string(rnumber) + ".csv";
        
        // this->CSVWeather.setFilename(weatherFilename);

        /* Weather DataFrame */

        try
        {
            this->WeatherData = this->CSVWeather.getData(weatherFilename);
        }
        catch (std::invalid_argument& e)
        {
            std::cerr << e.what() << std::endl;
            std::abort();
        }
        // std::cout << "Weather file selected: " << this->CSVWeather.fileName << std::endl;

        // Populate WDF
        WPeriods = this->WeatherData.size() - 1;  // -1 due to header
        // this->wdf_ptr = &(this->wdf[0]);
        // std::cout << "Weather Periods: " << WPeriods << std::endl;

        // Populate the wdf object
        this->CSVWeather.parseWeatherDF(this->wdf, this->WeatherData, WPeriods);
    }
    int maxFP = this->args.MinutesPerWP / this->args.FirePeriodLen * WPeriods;
    if (this->args.MaxFirePeriods > maxFP || this->args.MaxFirePeriods < 0)
    {
        this->totalFirePeriods = maxFP;
    }
    else
    {
        this->totalFirePeriods = this->args.MaxFirePeriods;
    }
    WeatherHistory[simExt] = weatherFilename;
}

/******************************************************************************

                                                                                                                                Main Program

*******************************************************************************/
/**
 * @brief Entry point for the Cell2Fire simulation program.
 *
 * This function initializes the simulation environment, reads input arguments,
 * and executes the main simulation loop, either in serial or parallel mode.
 * The program simulates fire spread across a forest landscape using stochastic
 * methods.
 *
 * @return Exit status of the program (0 for success).
 *
 * The main tasks performed include:
 * - Parsing command-line arguments.
 * - Initializing the forest simulation object and random number generators.
 * - Executing simulation episodes in parallel, with each episode representing
 * a replication.
 * - Resetting the environment and running simulation steps until completion
 * conditions are met.
 * - Handling multi-threading for parallel simulations.
 *
 */
int
main(int argc, char* argv[])
{
    printf("version: %s\n", C2FW_VERSION.c_str());
    // Read Arguments
    std::cout << "\n------ Command line values ------\n";
    arguments args;
    arguments* args_ptr = &args;
    parseArgs(argc, argv, args_ptr);
    IgnitionHistory.reserve(args.TotalSims);
    WeatherHistory.reserve(args.TotalSims);
    if (args.Stats)
        simStats.resize(args.TotalSims, {0.0f, 0.0f, 0.0f});
    GenDataFile(args.InFolder);
    int ep = 0;
    // Episodes loop (episode = replication)
    // CP: Modified to account the case when no ignition occurs and no grids are
    // generated (currently we are not generating grid when no ignition happened,
    // TODO)
    int num_threads = args.nthreads;
    Cell2Fire Forest2(args);  // generate Forest object
    setup_const();
    if (args.UseWeatherWeights)
    {
        std::string sep = ",";
        CSVReader CSVWeatherWeights(args.WeatherWeightsFile, sep);

        std::vector<std::vector<std::string>> WeatherWeightsDF = CSVWeatherWeights.getData(args.WeatherWeightsFile);

        CSVWeatherWeights.parseWeatherWeights(WeatherWeights, WeatherWeightIDs, WeatherWeightsDF, args.NWeatherFiles);

    }
    cout << "\n-------Running simulations-------" << endl;
    // Parallel zone
#pragma omp parallel num_threads(num_threads)
    {
        // Number of threads
        int TID = omp_get_thread_num();
        if (TID == 0)
        {
            if (omp_get_num_threads() >= 2)
            {
                cout << "There are " << omp_get_num_threads() << " threads" << endl;
            }
            else
            {
                cout << "Serial version execution" << endl;
            }
        }

        Cell2Fire Forest = Forest2;  // each thread gets its own copy of the initial state
        // Random seed

        // Random numbers (weather file and ROS-CV)
        int rnumber;
        double rnumber2;
#pragma omp for
        for (int ep = 1; ep <= args.TotalSims; ep++)
        {

            boost::random::mt19937 generator = boost::random::mt19937(args.seed * ep);
            // Random generator and distributions
            boost::random::uniform_int_distribution<int> udistribution(1, args.NWeatherFiles);
            boost::random::normal_distribution<> ndistribution(0.0, 1.0);
            boost::random::uniform_int_distribution<int> udistributionIgnition(1, Forest.nCells);

            TID = omp_get_thread_num();
            if (args.UseWeatherWeights) {
                std::discrete_distribution<int> dist(std::begin(WeatherWeights), std::end(WeatherWeights));
                int weatherWeightIndex = dist(generator);
                rnumber = WeatherWeightIDs[weatherWeightIndex];
            } else {
                rnumber = udistribution(generator);}
            rnumber2 = ndistribution(generator);
            // Reset
            Forest.reset(rnumber, rnumber2, ep);
            // Time steps during horizon (or until we break it)
            for (int tstep = 0; tstep <= Forest.totalFirePeriods * Forest.args.TotalYears; tstep++)
            {
                Forest.Step(generator, ep);

                if (Forest.done)
                {
                    break;
                }
            }
        }
    }
    // Write per-simulation stats CSV
    if (args.Stats)
    {
        std::string statsPath = args.OutFolder + "stats.csv";
        std::ofstream sfs(statsPath);
        sfs << "simulation,mean_surface_fl,mean_crown_fl,max_fl\n";
        for (int s = 0; s < args.TotalSims; s++)
            sfs << (s + 1) << "," << simStats[s][0] << "," << simStats[s][1] << "," << simStats[s][2] << "\n";
        sfs.close();
        std::cout << "Stats written to: " << statsPath << "\n";
    }

    delete[] df;
    return 0;
}
