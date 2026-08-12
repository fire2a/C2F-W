/* coding: utf-8
#include <fstream>
__version__ = "3.0"
__author__ = "Jaime Carrasco-Barra, Cristobal Pais"
__maintainer__ = "Jaime Carrasco-Barra, Matilde Rivas, David Palacios"
*/
// Unified version by David Palacios
//  Include classes
#include "Cell2Fire.h"
#include "Cells.h"
#include "DataGenerator.h"
#include "KitralKernel.h"
#include "ScottAndBurganKernel.h"
#include "FuelModelUtils.h"
#include "Lightning.h"
#include "ReadArgs.h"
#include "ReadCSV.h"
#include "ReadShp.h"
#include "Spotting.h"
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

// Global Variables (DFs with cells and weather info)
inputs* df_ptr;
// weatherDF* wdf_ptr;
// weatherDF wdf[100000];  // hard to dynamic allocate memory since it changes from
//  file to file, better to keep constant size;
inputs* df;
int currentSim = 0;
std::unordered_map<int, std::vector<float>> BBOFactors;
std::unordered_map<int, std::vector<int>> firebreakPlan;
std::vector<float> WeatherWeights;
std::vector<int> WeatherWeightIDs;
std::vector<int> NFTypesCells;
std::unordered_map<int, int> IgnitionHistory;
std::unordered_map<int, std::string> WeatherHistory;

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
          std::unordered_set<int> firebreakCells)
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
    for (auto& hc : firebreakCells)
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
 * - Categorizes cells as burnable, non-burnable, or firebreak based on input
 * data.
 * - If BBO tuning is enabled, initializes tuning factors using `BBOFuels.csv`.
 * - Validates and adjusts simulation parameters based on the weather file
 * consistency.
 */

// Codigos de combustible de la tabla del kernel en uso: "GR3" -> 103, "PCH1" -> 1.
// Lo comparten --fuel-adjustment y --treat-fuel, de modo que en ambos se pueda escribir
// el codigo del modelo en vez del numero del raster.
static std::unordered_map<std::string, int>
_lookupCodes(const arguments& args)
{
    std::unordered_map<std::string, int> m;
    const std::string sep = "/";
    std::string tabla;
    if (args.Simulator == "K") tabla = args.InFolder + sep + "kitral_lookup_table.csv";
    else if (args.Simulator == "C") tabla = args.InFolder + sep + "fbp_lookup_table.csv";
    else if (args.Simulator == "P") tabla = args.InFolder + sep + "portugal_lookup_table.csv";
    else
    {
        tabla = args.InFolder + sep + "scott_and_burgan_lookup_table.csv";
        std::ifstream probe(tabla);
        if (!probe.good()) tabla = args.InFolder + sep + "spain_lookup_table.csv";
    }
    std::ifstream lt(tabla);
    std::string ln;
    while (std::getline(lt, ln))
    {
        std::vector<std::string> campos;
        std::stringstream ss(ln);
        std::string campo;
        while (std::getline(ss, campo, ',')) campos.push_back(campo);
        if (campos.size() < 4) continue;
        try
        {
            int num = std::stoi(campos[0]);
            std::string cod = campos[3];
            while (!cod.empty() && (cod.back() == '\r' || cod.back() == ' ')) cod.pop_back();
            if (!cod.empty()) m[cod] = num;
        }
        catch (const std::invalid_argument&) { continue; }
    }
    return m;
}

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

    // ---- Rio: leer shapefile (--river-shp) y rasterizar a celdas-rio ----
    this->coordCells = frdf.coordCells;
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

    // Populate DF
    std::vector<std::vector<std::string>> DF = CSVParser.getData(filename);
    std::cout << "Forest DataFrame from instance " << filename << std::endl;
    // DEBUGCSVParser.printData(DF);
    std::cout << "Number of cells: " << this->nCells << std::endl;
    df = new inputs[this->nCells];

    // Create empty df with size of NCells
    df_ptr = &df[0];  // access reference for the first element of df

    // Populate the df [nCells] objects
    CSVParser.parseDF(df_ptr,
                      DF,
                      this->args_ptr,
                      this->nCells);  // iterates from the first element of df, using DF,
                                      // args_ptr and the number of cells

    // Initialize and populate relevant vectors
    this->fTypeCells = std::vector<int>(this->nCells, 1);
    this->fTypeCells2 = std::vector<string>(this->nCells, "Burnable");
    this->statusCells = std::vector<int>(this->nCells, 0);
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

    this->ignProb = std::vector<float>(this->nCells, 1);
    CSVParser.parsePROB(this->ignProb, DF, this->nCells);

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
            this->fTypeCells[l] = 0;
            this->fTypeCells2[l] = "NonBurnable";
            this->statusCells[l] = 4;
        }
    }

    // Firebreak cells
    if (strcmp(this->args.FirebreakPlan.c_str(), EM) != 0)
    {
        std::string sep = ",";
        CSVReader CSVFirebreakPlan(this->args.FirebreakPlan, sep);

        // Populate Ignitions vector
        std::vector<std::vector<std::string>> firebreakDF = CSVFirebreakPlan.getData(this->args.FirebreakPlan);
        // CSVFirebreakPlan.printData(firebreakDF);

        // Cells
        int HCellsP = firebreakDF.size() - 1;
        CSVFirebreakPlan.parseFirebreakDF(firebreakPlan, firebreakDF, HCellsP);

        // Print-out
        std::cout << "Number of Firebreak Cells :" << firebreakPlan.size() << std::endl;
        for (auto it = firebreakPlan.begin(); it != firebreakPlan.end(); it++)
        {
            for (auto& it2 : it->second)
            {
                this->fTypeCells[it2 - 1] = 0;
                this->fTypeCells2[it2 - 1] = "NonBurnable";
                this->statusCells[it2 - 1] = 3;
            }
        }
    }

    // ---- Tratamientos silviculturales sobre una zona ------------------------
    // Modifican la estructura del rodal sin eliminar la celda, que es lo que distingue
    // un raleo o una poda de un cortafuegos. Subir la altura de base de copa dificulta
    // que el fuego de superficie inicie copas; bajar la densidad aparente dificulta que
    // el fuego de copas se propague. La carga superficial no se toca por separado: en
    // Scott & Burgan la carga DEFINE el modelo, asi que reducirla es reasignar el
    // combustible (--treat-fuel), no bajar un numero.
    if (!this->args.TreatmentShp.empty())
    {
        std::vector<ShpPart> parts;
        if (!readShapefileParts(this->args.TreatmentShp, parts))
            throw std::runtime_error("No se pudo leer " + this->args.TreatmentShp
                                     + " (debe ser PolyLine o Polygon en la CRS de la instancia)");
        const int shpType = readShapefileType(this->args.TreatmentShp);
        const bool isPolygon = (shpType == 5 || shpType == 15 || shpType == 25);

        // celdas de la zona: las que el poligono cubre por sobre el umbral de cobertura
        std::set<int> zona;
        std::set<int> tocadas;
        {
            const double paso = this->cellSide * 0.25;
            for (const auto& part : parts)
                for (size_t k = 0; k + 1 < part.size(); ++k)
                {
                    double x0 = part[k].x, y0 = part[k].y, x1 = part[k + 1].x, y1 = part[k + 1].y;
                    double seg = std::sqrt((x1 - x0) * (x1 - x0) + (y1 - y0) * (y1 - y0));
                    int ns = std::max(1, (int)std::ceil(seg / paso));
                    for (int t = 0; t <= ns; ++t)
                    {
                        double x = x0 + (x1 - x0) * t / ns, y = y0 + (y1 - y0) * t / ns;
                        int c = (int)((x - this->xllcorner) / this->cellSide);
                        int r = this->rows - 1 - (int)((y - this->yllcorner) / this->cellSide);
                        if (r >= 0 && r < this->rows && c >= 0 && c < this->cols)
                            tocadas.insert(r * this->cols + c + 1);
                    }
                }
        }
        if (isPolygon)
        {
            // el interior completo, no solo el borde: la zona tratada es un area
            double bx0, by0, bx1, by1;
            readShapefileBBox(this->args.TreatmentShp, bx0, by0, bx1, by1);
            int c0 = std::max(0, (int)((bx0 - this->xllcorner) / this->cellSide));
            int c1 = std::min(this->cols - 1, (int)((bx1 - this->xllcorner) / this->cellSide));
            int r0 = std::max(0, this->rows - 1 - (int)((by1 - this->yllcorner) / this->cellSide));
            int r1 = std::min(this->rows - 1, this->rows - 1 - (int)((by0 - this->yllcorner) / this->cellSide));
            for (int r = r0; r <= r1; ++r)
                for (int c = c0; c <= c1; ++c)
                {
                    double x = this->xllcorner + c * this->cellSide;
                    double y = this->yllcorner + (this->rows - 1 - r) * this->cellSide;
                    if (cellAreaFraction(parts, x, y, this->cellSide) >= this->args.BarrierCover)
                        zona.insert(r * this->cols + c + 1);
                }
        }
        else
        {
            zona = tocadas;
        }

        int nuevoFuel = -1;
        if (!this->args.TreatFuel.empty())
        {
            auto codigos = _lookupCodes(this->args);
            auto it = codigos.find(this->args.TreatFuel);
            if (it != codigos.end()) nuevoFuel = it->second;
            else
            {
                try { nuevoFuel = std::stoi(this->args.TreatFuel); }
                catch (const std::exception&)
                {
                    throw std::runtime_error("--treat-fuel: '" + this->args.TreatFuel
                                             + "' no existe en la tabla de " + this->args.Simulator);
                }
            }
        }

        int nCbh = 0, nCbd = 0, nCcf = 0, nFuel = 0;
        for (int cid : zona)
        {
            inputs& d = df[cid - 1];
            if (this->args.TreatCBH >= 0.0f) { d.cbh = this->args.TreatCBH; ++nCbh; }
            if (this->args.TreatCBD >= 0.0f) { d.cbd = this->args.TreatCBD; ++nCbd; }
            if (this->args.TreatCCF >= 0.0f) { d.ccf = this->args.TreatCCF; ++nCcf; }
            if (nuevoFuel > 0)
            {
                d.nftype = nuevoFuel;
                this->fTypeCells[cid - 1] = 1;          // sigue siendo quemable
                ++nFuel;
            }
        }
        std::cout << "Treatment: " << zona.size() << " celda(s) en "
                  << this->args.TreatmentShp;
        if (nCbh)  std::cout << " | cbh=" << this->args.TreatCBH;
        if (nCbd)  std::cout << " | cbd=" << this->args.TreatCBD;
        if (nCcf)  std::cout << " | ccf=" << this->args.TreatCCF;
        if (nFuel) std::cout << " | fuel=" << this->args.TreatFuel << "(" << nuevoFuel << ")";
        std::cout << std::endl;
    }

    // ---- Factores de ajuste del ROS por tipo de combustible -----------------
    // Analogo al fuel adjustment factor de FARSITE: multiplica el ROS de las celdas
    // de ese combustible, igual en todas las direcciones. CSV de dos columnas
    // (codigo,factor) con encabezado opcional. Los combustibles no listados quedan
    // en 1.0. La numeracion es la del kernel en uso, por eso el archivo va por
    // instancia y no hay una tabla comun a los tres.
    if (!this->args.FuelAdjustmentFile.empty())
    {
        std::unordered_map<std::string,int> codigoAFuel = _lookupCodes(this->args);

        std::ifstream fa(this->args.FuelAdjustmentFile);
        if (!fa.good())
            throw std::runtime_error("No se pudo abrir " + this->args.FuelAdjustmentFile);
        std::string linea;
        int n = 0;
        while (std::getline(fa, linea))
        {
            if (linea.empty()) continue;
            size_t coma = linea.find(',');
            if (coma == std::string::npos) continue;
            std::string clave = linea.substr(0, coma);
            while (!clave.empty() && (clave.back() == '\r' || clave.back() == ' ')) clave.pop_back();
            try
            {
                int code;
                auto ic = codigoAFuel.find(clave);
                if (ic != codigoAFuel.end())
                {
                    code = ic->second;   // codigo string, p.ej. GR3 o PCH1
                }
                else
                {
                    // No es un codigo de la tabla: se acepta como numerico. Si tampoco
                    // lo es, stoi lanza invalid_argument y se trata mas abajo: un
                    // codigo mal escrito no puede pasar en silencio, porque dejaria la
                    // corrida sin calibrar sin que nadie se entere.
                    size_t consumidos = 0;
                    code = std::stoi(clave, &consumidos);
                    if (consumidos != clave.size()) throw std::invalid_argument(clave);
                }
                double f = std::stod(linea.substr(coma + 1));
                if (f <= 0.0)
                    throw std::runtime_error("Factor no positivo para el combustible "
                                             + std::to_string(code));
                fuelAdjustment[code] = f;
                ++n;
            }
            catch (const std::invalid_argument&)
            {
                // El encabezado es la unica linea no numerica que se tolera.
                if (clave == "fuel" || clave == "Fuel" || clave == "codigo" || clave == "code")
                    continue;
                std::string validos;
                int k2 = 0;
                for (auto& e : codigoAFuel)
                {
                    if (k2++ >= 12) { validos += " ..."; break; }
                    validos += " " + e.first;
                }
                throw std::runtime_error("Combustible '" + clave + "' no existe en la tabla de "
                                         + this->args.Simulator + ". Codigos validos:" + validos);
            }
        }
        std::cout << "Fuel adjustment: " << n << " combustible(s) con factor";
        int k = 0;
        for (auto& e : fuelAdjustment)
        {
            if (k++ >= 8) { std::cout << " ..."; break; }
            std::cout << "  " << e.first << "=" << e.second;
        }
        std::cout << std::endl;
    }

    // ---- Barreras vectoriales: rios, caminos y cortafuegos ------------------
    // Semantica UNIFICADA para las tres: la celda deja de ser combustible
    // (statusCells=3, sale de availCells) y ademas queda marcada como barrera, de
    // modo que el breaching pueda saltarla. Sin breaching, detienen el fuego.
    // El tipo solo distingue la atribucion en RiverCrossings*.csv.
    //
    // Regla de rasterizacion segun geometria:
    //   Polygon  -> la celda cae si su CENTRO esta dentro del poligono. Asi una
    //               franja mas angosta que la celda no se convierte en un muro del
    //               ancho de la celda: solo captura las celdas que realmente cubre.
    //   PolyLine -> una linea no encierra nada, asi que se toman las celdas que
    //               atraviesa (muestreo denso). Es una sobreestimacion inevitable:
    //               para anchos sub-celda conviene entregar poligonos.
    {
        std::vector<std::pair<std::string, std::string>> vecSrc;  // {ruta, tipo}
        if (!this->args.RiverShp.empty()) vecSrc.push_back({ this->args.RiverShp, "river" });
        if (!this->args.RoadShp.empty()) vecSrc.push_back({ this->args.RoadShp, "road" });
        if (!this->args.FirebreakShp.empty()) vecSrc.push_back({ this->args.FirebreakShp, "firebreak" });
        if (this->args.UseRivers)
            for (const auto& p : listShapefiles(this->args.InFolder + "Rivers"))
                vecSrc.push_back({ p.first, "river" });
        if (this->args.UseRoads)
            for (const auto& p : listShapefiles(this->args.InFolder + "Roads"))
                vecSrc.push_back({ p.first, "road" });
        if (this->args.UseFirebreaks)
            for (const auto& p : listShapefiles(this->args.InFolder + "Firebreaks"))
                vecSrc.push_back({ p.first, "firebreak" });

        int vecTotal = 0;
        for (const auto& src : vecSrc)
        {
            // Aviso temprano si no solapa el raster: casi siempre es CRS equivocada.
            double bx0, by0, bx1, by1;
            if (readShapefileBBox(src.first, bx0, by0, bx1, by1))
            {
                double rx1 = this->xllcorner + this->cols * this->cellSide;
                double ry1 = this->yllcorner + this->rows * this->cellSide;
                if (bx1 < this->xllcorner || bx0 > rx1 || by1 < this->yllcorner || by0 > ry1)
                    std::cout << "Barrier[" << src.second << "]: AVISO, " << src.first
                              << " no solapa el raster. Revisa la CRS." << std::endl;
            }

            std::vector<ShpPart> parts;
            if (!readShapefileParts(src.first, parts))
            {
                std::cout << "Barrier[" << src.second << "]: no se pudo leer " << src.first
                          << " (debe ser PolyLine o Polygon en la CRS de la instancia)" << std::endl;
                continue;
            }
            const int shpType = readShapefileType(src.first);
            const bool isPolygon = (shpType == 5 || shpType == 15 || shpType == 25);

            // Celdas que la geometria atraviesa: base para todo lo que sigue.
            // Solo estas pueden tener cobertura significativa, asi que restringir el
            // test de area a este conjunto evita recorrer el bbox completo (para el
            // Rio Claro son 54 km de trazado y el bbox abarca media instancia).
            std::set<int> tocadas;
            {
                const double paso = this->cellSide * 0.25;
                for (const auto& part : parts)
                    for (size_t k2 = 0; k2 + 1 < part.size(); ++k2)
                    {
                        double x0 = part[k2].x, y0 = part[k2].y;
                        double x1 = part[k2 + 1].x, y1 = part[k2 + 1].y;
                        double seg = std::sqrt((x1 - x0) * (x1 - x0) + (y1 - y0) * (y1 - y0));
                        int ns = std::max(1, (int)std::ceil(seg / paso));
                        for (int t = 0; t <= ns; ++t)
                        {
                            double x = x0 + (x1 - x0) * t / ns;
                            double y = y0 + (y1 - y0) * t / ns;
                            int cc = (int)((x - this->xllcorner) / this->cellSide);
                            int rr = this->rows - 1 - (int)((y - this->yllcorner) / this->cellSide);
                            if (rr >= 0 && rr < this->rows && cc >= 0 && cc < this->cols)
                                tocadas.insert(rr * this->cols + cc + 1);
                        }
                    }
            }

            std::set<int> cells;
            if (isPolygon)
            {
                // Cobertura de area sobre las celdas atravesadas.
                for (int cid : tocadas)
                {
                    int r = (cid - 1) / this->cols, c = (cid - 1) % this->cols;
                    double x0 = this->xllcorner + c * this->cellSide;
                    double y0 = this->yllcorner + (this->rows - 1 - r) * this->cellSide;
                    if (cellAreaFraction(parts, x0, y0, this->cellSide) >= this->args.BarrierCover)
                        cells.insert(cid);
                }
            }
            else
            {
                double step = this->cellSide * 0.5;
                for (auto& part : parts)
                    for (size_t k2 = 0; k2 + 1 < part.size(); ++k2)
                    {
                        double x0 = part[k2].x, y0 = part[k2].y;
                        double x1 = part[k2 + 1].x, y1 = part[k2 + 1].y;
                        double seg = std::sqrt((x1 - x0) * (x1 - x0) + (y1 - y0) * (y1 - y0));
                        int nstep = std::max(1, (int)std::ceil(seg / step));
                        for (int t = 0; t <= nstep; ++t)
                        {
                            double x = x0 + (x1 - x0) * t / nstep;
                            double y = y0 + (y1 - y0) * t / nstep;
                            int col = (int)((x - this->xllcorner) / this->cellSide);
                            int row = this->rows - 1 - (int)((y - this->yllcorner) / this->cellSide);
                            if (row >= 0 && row < this->rows && col >= 0 && col < this->cols)
                                cells.insert(row * this->cols + col + 1);
                        }
                    }
            }

            // ---- Aristas bloqueadas -------------------------------------------
            // El bloqueo real va en la TRANSICION: para cada celda vecina de la
            // barrera se prueban sus 8 salidas, y si el segmento centro-a-centro
            // cruza la geometria, ese paso queda bloqueado con su ancho cruzado.
            // Esto hace que un rio de 10 m detenga el fuego igual que uno de 100 y
            // elimina las fugas diagonales de una barrera de una celda de ancho.
            {
                // Candidatas para aristas: las atravesadas mas su vecindad.
                std::set<int> cand;
                for (int cid : tocadas)
                {
                    int r = (cid - 1) / this->cols, c = (cid - 1) % this->cols;
                    for (int dr = -2; dr <= 2; ++dr)
                        for (int dc = -2; dc <= 2; ++dc)
                        {
                            int rr = r + dr, cc = c + dc;
                            if (rr >= 0 && rr < this->rows && cc >= 0 && cc < this->cols)
                                cand.insert(rr * this->cols + cc + 1);
                        }
                }
                int nArcs = 0;
                for (int from : cand)
                {
                    int r = (from - 1) / this->cols, c = (from - 1) % this->cols;
                    double ax = this->xllcorner + (c + 0.5) * this->cellSide;
                    double ay = this->yllcorner + (this->rows - 1 - r + 0.5) * this->cellSide;
                    for (int dr = -1; dr <= 1; ++dr)
                        for (int dc = -1; dc <= 1; ++dc)
                        {
                            if (dr == 0 && dc == 0) continue;
                            int rr = r + dr, cc = c + dc;
                            if (rr < 0 || rr >= this->rows || cc < 0 || cc >= this->cols) continue;
                            int to = rr * this->cols + cc + 1;
                            double bx = this->xllcorner + (cc + 0.5) * this->cellSide;
                            double by = this->yllcorner + (this->rows - 1 - rr + 0.5) * this->cellSide;
                            double w = crossedWidth(ax, ay, bx, by, parts, this->args.BarrierWidth);
                            if (w > 0.0)
                            {
                                long long k = this->arcKey(from, to);
                                auto prev = this->blockedArcs.find(k);
                                if (prev == this->blockedArcs.end() || prev->second < w)
                                {
                                    this->blockedArcs[k] = w;
                                    this->arcBarrierType[k] = src.second;
                                }
                                ++nArcs;
                            }
                        }
                }
                std::cout << "Barrier[" << src.second << "]: " << nArcs
                          << " transicion(es) bloqueada(s)" << std::endl;
            }

            for (int cid : cells)
            {
                // no combustible: via firebreakPlan, que es lo que re-aplica el reset
                firebreakPlan[1].push_back(cid);
                this->fTypeCells[cid - 1] = 0;
                this->fTypeCells2[cid - 1] = "NonBurnable";
                this->statusCells[cid - 1] = 3;
                // y barrera, para que el breaching pueda saltarla (con su tipo)
                this->barrierCells[cid] = src.second;
                if (src.second == "river") this->riverCells.insert(cid);
            }
            vecTotal += (int)cells.size();
            std::cout << "Barrier[" << src.second << "]: " << cells.size() << " celda(s) desde "
                      << src.first << " (" << (isPolygon ? "poligono, cobertura >= umbral" : "linea, celdas atravesadas")
                      << ")" << std::endl;
        }
        if (vecTotal > 0)
            std::cout << "Barriers: " << vecTotal << " celda(s) vectoriales en total" << std::endl;
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
    this->nonBurnableCells.clear();
    this->burningCells.clear();
    this->burntCells.clear();
    this->firebreakCells.clear();
    for (i = 0; i < this->statusCells.size(); i++)
    {
        if (this->statusCells[i] < 3)
            this->availCells.insert(i + 1);
        else if (this->statusCells[i] == 4)
            this->nonBurnableCells.insert(i + 1);
        else if (this->statusCells[i] == 3)
            this->firebreakCells.insert(i + 1);
    }

    // Fuga diagonal: dos celdas de barrera que solo se tocan en un vertice no cortan la
    // propagacion 8-conectada, porque el fuego pasa por la diagonal que las cruza. Una
    // linea de cortafuegos trazada en diagonal no detiene nada: en un caso de prueba de
    // 11x11 con la antidiagonal completa marcada, el fuego alcanzaba igual las 25 celdas
    // del otro lado. Las barreras dadas como geometria ya lo evitan porque bloquean
    // arcos; las dadas por celda (--FirebreakCells, firebreakPlan) necesitan esto.
    if (!this->firebreakCells.empty())
    {
        int nDiag = 0;
        for (int cid : this->firebreakCells)
        {
            const int r = (cid - 1) / this->cols, c = (cid - 1) % this->cols;
            // solo las dos diagonales "hacia adelante": la simetrica la aporta la otra celda
            const int dr[2] = { 1, 1 }, dc[2] = { 1, -1 };
            for (int k = 0; k < 2; ++k)
            {
                const int r2 = r + dr[k], c2 = c + dc[k];
                if (r2 < 0 || r2 >= this->rows || c2 < 0 || c2 >= this->cols) continue;
                const int diagId = r2 * this->cols + c2 + 1;
                if (!this->firebreakCells.count(diagId)) continue;
                // las dos celdas que comparten ese vertice y no son barrera
                const int a = r * this->cols + c2 + 1;
                const int b = r2 * this->cols + c + 1;
                if (this->firebreakCells.count(a) && this->firebreakCells.count(b)) continue;
                // Ancho "infinito": no es un hueco que el fuego pueda saltar segun su
                // longitud de llama, sino un paso que geometricamente no existe. Ningun
                // --breach-factor ni --spot-factor debe habilitarlo.
                const double INFRANQUEABLE = 1e9;
                this->blockedArcs[arcKey(a, b)] = INFRANQUEABLE;
                this->blockedArcs[arcKey(b, a)] = INFRANQUEABLE;
                this->arcBarrierType[arcKey(a, b)] = "firebreak";
                this->arcBarrierType[arcKey(b, a)] = "firebreak";
                ++nDiag;
            }
        }
        if (nDiag > 0)
            std::cout << "Firebreak: " << nDiag << " cruce(s) diagonal(es) bloqueado(s)" << std::endl;
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

        // --- Modalidad B: puntos desde shapefile (--ignition-shp) -------------
        // Un punto = la ignicion de un anio, en el orden del archivo, igual que
        // las filas de Ignitions.csv. Requiere que el .shp este en la CRS de la
        // instancia (misma conversion que --river-shp).
        bool ignitionsFromShp = false;
        if (!this->args.IgnitionShp.empty())
        {
            std::vector<ShpPoint> pts;
            if (readShapefilePoints(this->args.IgnitionShp, pts))
            {
                std::vector<int> shpCells;
                int outside = 0;
                for (auto& q : pts)
                {
                    int col = (int)((q.x - this->xllcorner) / this->cellSide);
                    int row = this->rows - 1 - (int)((q.y - this->yllcorner) / this->cellSide);
                    if (row >= 0 && row < this->rows && col >= 0 && col < this->cols)
                        shpCells.push_back(row * this->cols + col + 1);
                    else
                        ++outside;
                }
                if (outside > 0)
                    std::cout << "Ignition: " << outside << " punto(s) fuera del raster, descartados"
                              << std::endl;
                if (shpCells.empty())
                {
                    throw std::runtime_error("Ignition: ningun punto de " + this->args.IgnitionShp
                                             + " cae dentro del raster. Revisa que el .shp este en la CRS"
                                               " de la instancia.");
                }
                IgnitionYears = (int)shpCells.size();
                args.TotalYears = std::min(args.TotalYears, IgnitionYears);
                this->IgnitionPoints = shpCells;
                ignitionsFromShp = true;
                std::cout << "Ignition: " << shpCells.size() << " punto(s) desde "
                          << this->args.IgnitionShp << " (celda(s):";
                for (size_t k = 0; k < shpCells.size() && k < 10; ++k) std::cout << " " << shpCells[k];
                if (shpCells.size() > 10) std::cout << " ...";
                std::cout << ")" << std::endl;
            }
            else
            {
                throw std::runtime_error("Ignition: no se pudo leer " + this->args.IgnitionShp
                                         + " (debe ser Point o MultiPoint en la CRS de la instancia)");
            }
        }

        // --- Modalidad A: Ignitions.csv (por defecto) -------------------------
        if (!ignitionsFromShp)
        {
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
        }
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

    /* Active front: read a set of cells to ignite simultaneously (ActiveCells.csv) */
    if (this->args.ActiveFront)
    {
        this->ActiveFrontCells.clear();

        // --- Modalidad B: frente desde shapefile (--active-front-shp) ---------
        // Acepta PolyLine/Polygon (se rasteriza el trazado con muestreo denso, igual
        // que --river-shp) o Point/MultiPoint (cada punto es una celda semilla).
        // Un frente es naturalmente una linea; los puntos existen por comodidad.
        // OJO con Polygon: se rasteriza el ANILLO, no el interior relleno.
        bool frontFromShp = false;
        if (!this->args.ActiveFrontShp.empty())
        {
            std::set<int> cells;  // set: el muestreo denso repite celdas
            std::vector<ShpPart> parts;
            std::vector<ShpPoint> pts;
            const char* kind = nullptr;

            if (readShapefileParts(this->args.ActiveFrontShp, parts))
            {
                kind = "linea(s)";
                double step = this->cellSide * 0.5;
                for (auto& part : parts)
                {
                    for (size_t s = 0; s + 1 < part.size(); ++s)
                    {
                        double x0 = part[s].x, y0 = part[s].y, x1 = part[s + 1].x, y1 = part[s + 1].y;
                        double seg = std::sqrt((x1 - x0) * (x1 - x0) + (y1 - y0) * (y1 - y0));
                        int nstep = std::max(1, (int)std::ceil(seg / step));
                        for (int t = 0; t <= nstep; ++t)
                        {
                            double x = x0 + (x1 - x0) * t / nstep;
                            double y = y0 + (y1 - y0) * t / nstep;
                            int col = (int)((x - this->xllcorner) / this->cellSide);
                            int row = this->rows - 1 - (int)((y - this->yllcorner) / this->cellSide);
                            if (row >= 0 && row < this->rows && col >= 0 && col < this->cols)
                                cells.insert(row * this->cols + col + 1);
                        }
                    }
                }
            }
            else if (readShapefilePoints(this->args.ActiveFrontShp, pts))
            {
                kind = "punto(s)";
                for (auto& q : pts)
                {
                    int col = (int)((q.x - this->xllcorner) / this->cellSide);
                    int row = this->rows - 1 - (int)((q.y - this->yllcorner) / this->cellSide);
                    if (row >= 0 && row < this->rows && col >= 0 && col < this->cols)
                        cells.insert(row * this->cols + col + 1);
                }
            }
            else
            {
                throw std::runtime_error("Active front: no se pudo leer " + this->args.ActiveFrontShp
                                         + " (debe ser PolyLine/Polygon o Point/MultiPoint)");
            }

            if (cells.empty())
            {
                throw std::runtime_error("Active front: ninguna celda de " + this->args.ActiveFrontShp
                                         + " cae dentro del raster. Revisa que el .shp este en la CRS"
                                           " de la instancia.");
            }
            this->ActiveFrontCells.assign(cells.begin(), cells.end());
            frontFromShp = true;
            std::cout << "Active front: " << this->ActiveFrontCells.size() << " celda(s) desde "
                      << this->args.ActiveFrontShp << " (" << kind << ")" << std::endl;
        }

        // --- Modalidad A: ActiveCells.csv (por defecto) -----------------------
        if (!frontFromShp)
        {
            std::string sepAF = ",";
            std::string activeFile = args.InFolder + "ActiveCells.csv";
            CSVReader CSVActive(activeFile, sepAF);
            std::vector<std::vector<std::string>> ActiveDF = CSVActive.getData(activeFile);
            for (size_t rr = 1; rr < ActiveDF.size(); ++rr)  // skip header row
            {
                if (ActiveDF[rr].size() >= 2 && !ActiveDF[rr][1].empty())
                    this->ActiveFrontCells.push_back(std::stoi(ActiveDF[rr][1]));
            }
        }
        if (!this->ActiveFrontCells.empty())
        {
            this->args.TotalYears = 1;  // the front is a single scenario
            this->IgnitionPoints = std::vector<int>(1, this->ActiveFrontCells[0]);
            this->IgnitionSets = std::vector<std::vector<int>>(1);
            std::cout << "Active front: " << this->ActiveFrontCells.size()
                      << " seed cells will ignite simultaneously" << std::endl;
        }
    }

    /* BBO Tuning factors (only the ones present in the instances*/
    if (this->args.BBOTuning)
    {
        // Get fuel types numbers
        CSVParser.parseNDF(NFTypesCells, DF, this->nCells);

        // BBO File
        std::string BBOFile = args.InFolder + "BBOFuels.csv";
        std::string sep = ",";
        CSVReader CSVBBO(BBOFile, sep);

        // Populate BBO vector
        std::vector<std::vector<std::string>> BBODF = CSVBBO.getData(BBOFile);
        int BBONTypes = BBODF.size() - 1;
        CSVBBO.parseBBODF(BBOFactors, BBODF, BBONTypes);

        // Print-out
        std::cout << "BBO Factors:" << std::endl;
        for (auto it = BBOFactors.begin(); it != BBOFactors.end(); it++)
        {
            std::cout << " " << it->first << ": ";
            for (auto& it2 : it->second)
            {
                std::cout << it2 << " ";
            }
        }
        std::cout << std::endl;
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
    // Declare an iterator to unordered_map
    std::unordered_map<int, Cells>::iterator it2;

    // Initialize cell, insert it inside the unordered map
    Cells Cell(id - 1,
               this->areaCells,
               this->coordCells[id - 1],
               this->fTypeCells[id - 1],
               this->fTypeCells2[id - 1],
               this->perimeterCells,
               this->statusCells[id - 1],
               id);
    this->Cells_Obj.insert(std::make_pair(id, Cell));

    // Get object from unordered map
    it2 = this->Cells_Obj.find(id);

    // Initialize the fire fields for the selected cel
    it2->second.initializeFireFields(this->coordCells, this->availCells, this->cols, this->rows);

    // Print info for debugging
    if (this->args.verbose)
        it2->second.print_info();
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
    // ROS Folder
    if (this->args.OutRos)
    {
        this->rosFolder = Cell2Fire::createOutputFolder("RateOfSpread");
    }
    // Surface Byram Intensity Folder
    if (this->args.OutIntensity)
    {
        this->surfaceIntensityFolder = Cell2Fire::createOutputFolder("SurfaceIntensity");
    }
    // Crown Byram Intensity Folder
    if ((this->args.OutIntensity) && (this->args.AllowCROS)
        && ((this->args.Simulator == "S") || this->args.Simulator == "P" || this->args.Simulator == "K"))
    {
        this->crownIntensityFolder = Cell2Fire::createOutputFolder("CrownIntensity");
    }
    // Surface Flame Length Folder
    if (this->args.OutFl)
    {
        this->surfaceFlameLengthFolder = Cell2Fire::createOutputFolder("SurfaceFlameLength");
    }
    // Crown Flame Length Folder
    if ((this->args.OutFl) && (this->args.AllowCROS) && ((this->args.Simulator == "S") || this->args.Simulator == "P" || this->args.Simulator == "K"))
    {
        this->crownFlameLengthFolder = Cell2Fire::createOutputFolder("CrownFlameLength");
    }
    // max Flame Length Folder
    if ((this->args.OutFl) && (this->args.AllowCROS) && ((this->args.Simulator == "S") || this->args.Simulator == "P" || this->args.Simulator == "K"))
    {
        this->maxFlameLengthFolder = Cell2Fire::createOutputFolder("MaxFlameLength");
    }
    // Crown Folder
    if (this->args.OutCrown && this->args.AllowCROS)
    {
        this->crownFolder = Cell2Fire::createOutputFolder("CrownFire");
    }
    // Crown Fraction Burn Folder
    if (this->args.OutCrownConsumption && this->args.AllowCROS)
    {
        this->cfbFolder = Cell2Fire::createOutputFolder("CrownFractionBurn");
    }
    // Surf Fraction Burn Folder
    if (this->args.OutSurfConsumption && this->args.Simulator == "C")
    {
        this->sfbFolder = Cell2Fire::createOutputFolder("SurfFractionBurn");
    }

    chooseWeather(this->args.WeatherOpt, rnumber, simExt);
    // Random ROS-CV
    this->ROSRV = std::abs(rnumber2);
    // std::cout << "ROSRV: " << this->ROSRV << std::endl;

    // Cells dictionary
    this->Cells_Obj.clear();

    // Declare an iterator to unordered_map
    std::unordered_map<int, Cells>::iterator it;

    // Reset status
    this->fTypeCells = std::vector<int>(this->nCells, 1);
    this->fTypeCells2 = std::vector<string>(this->nCells, "Burnable");
    this->statusCells = std::vector<int>(this->nCells, 0);
    this->crownState = std::vector<int>(this->nCells, 0);
    this->crownFraction = std::vector<float>(this->nCells, 0);
    this->surfFraction = std::vector<float>(this->nCells, 0);
    this->surfaceIntensities = std::vector<float>(this->nCells, 0);
    this->crownIntensities = std::vector<float>(this->nCells, 0);
    this->RateOfSpreads = std::vector<float>(this->nCells, 0);
    this->surfaceFlameLengths = std::vector<float>(this->nCells, 0);
    this->crownFlameLengths = std::vector<float>(this->nCells, 0);
    this->maxFlameLengths = std::vector<float>(this->nCells, 0);

    this->FSCell.clear();
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
            this->fTypeCells[i] = 0;
            this->fTypeCells2[i] = "NonBurnable";
            this->statusCells[i] = 4;
        }
    }

    // Relevant sets: Initialization
    this->availCells.clear();
    this->nonBurnableCells.clear();
    this->burningCells.clear();
    this->burntCells.clear();
    this->firebreakCells.clear();

    // Firebreak Cells
    for (auto it = firebreakPlan.begin(); it != firebreakPlan.end(); it++)
    {
        for (auto& it2 : it->second)
        {
            this->fTypeCells[it2 - 1] = 0;
            this->fTypeCells2[it2 - 1] = "NonBurnable";
            this->statusCells[it2 - 1] = 3;
        }
    }

    for (i = 0; i < this->statusCells.size(); i++)
    {
        if (this->statusCells[i] < 3)
            this->availCells.insert(i + 1);
        else if (this->statusCells[i] == 4)
            this->nonBurnableCells.insert(i + 1);
        else if (this->statusCells[i] == 3)
            this->firebreakCells.insert(i + 1);
    }

    // Print-out sets information
    if (this->args.verbose)
    {
        printSets(this->availCells, this->nonBurnableCells, this->burningCells, this->burntCells, this->firebreakCells);
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
    std::unordered_map<int, Cells>::iterator it;
    // std::uniform_int_distribution<int> distribution(1, this->nCells);

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
                if (this->ignProb[aux - 1] > rd_number)
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
            if (this->statusCells[aux - 1] < 3 && this->burntCells.find(aux) == this->burntCells.end())
            {
                if (this->Cells_Obj.find(aux) == this->Cells_Obj.end())
                {
                    InitCell(aux);
                    it = this->Cells_Obj.find(aux);
                }

                if (it->second.getStatus() == "Available" && it->second.fType != 0)
                {
                    IgnitionHistory[sim] = aux;
                    // std::cout << "Selected (Random) ignition point: " << aux << std::endl;
                    std::vector<int> ignPts = { aux };
                    if (it->second.ignition(this->fire_period[year - 1],
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
                            std::cout << "Cell " << it->second.realId << " Ignites" << std::endl;
                            std::cout << "Cell " << it->second.realId << " Status: " << it->second.getStatus()
                                      << std::endl;
                        }

                        // Status
                        this->statusCells[it->second.realId - 1] = 1;

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
        if (this->burntCells.find(temp) == this->burntCells.end() && this->statusCells[temp - 1] < 3)
        {
            if (this->Cells_Obj.find(temp) == this->Cells_Obj.end())
            {
                // Initialize cell, insert it inside the unordered map
                InitCell(temp);
            }

            // Iterator
            it = this->Cells_Obj.find(temp);

            // Not available or non burnable: no ignition
            if (it->second.getStatus() != "Available" || it->second.fType == 0)
            {
                this->noIgnition = true;
            }

            // Available and Burnable: ignition
            if (it->second.getStatus() == "Available" && it->second.fType != 0)
            {
                std::vector<int> ignPts = { temp };
                if (it->second.ignition(this->fire_period[year - 1],
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
                        std::cout << "Cell " << it->second.realId << " Ignites" << std::endl;
                        std::cout << "Cell " << it->second.realId << " Status: " << it->second.getStatus() << std::endl;
                    }

                    // Status
                    this->statusCells[it->second.realId - 1] = 1;
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
    if (!this->noIgnition)
    {
        int newId = it->second.realId;
        if (this->args.verbose)
        std:
            cout << "New ID for burning cell: " << newId << std::endl;

        this->nIgnitions++;
        this->burningCells.insert(newId);
        this->burntCells.insert(newId);
        this->availCells.erase(newId);

        // Print sets information
        if (this->args.verbose)
        {
            printSets(
                this->availCells, this->nonBurnableCells, this->burningCells, this->burntCells, this->firebreakCells);
        }
    }

    // --- Active front: ignite all remaining seed cells simultaneously ---
    if (this->args.ActiveFront)
    {
        bool anyAF = false;
        for (size_t k = 0; k < this->ActiveFrontCells.size(); ++k)
        {
            int fc = this->ActiveFrontCells[k];
            if (fc < 1 || fc > this->nCells)
                continue;
            if (this->burntCells.find(fc) != this->burntCells.end())  // already lit (e.g. first cell)
            {
                anyAF = true;
                continue;
            }
            if (this->statusCells[fc - 1] >= 3)  // non-burnable / firebreak / firebreak
                continue;
            if (this->Cells_Obj.find(fc) == this->Cells_Obj.end())
                InitCell(fc);
            std::unordered_map<int, Cells>::iterator itf = this->Cells_Obj.find(fc);
            if (itf->second.getStatus() != "Available" || itf->second.fType == 0)
                continue;
            std::vector<int> ipf = { fc };
            if (itf->second.ignition(this->fire_period[this->year - 1],
                                     this->year, ipf, &df[fc - 1],
                                     this->coef_ptr, this->args_ptr,
                                     &(this->wdf[this->weatherPeriod]),
                                     this->activeCrown, this->perimeterCells))
            {
                this->statusCells[fc - 1] = 1;
                this->nIgnitions++;
                this->burningCells.insert(fc);
                this->burntCells.insert(fc);
                this->availCells.erase(fc);
                anyAF = true;
            }
        }
        if (anyAF)
            this->noIgnition = false;
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
 * burning, burnt, firebreak), and fire message details.
 *
 * ### Warning:
 * - A warning is issued if the fire period approaches the maximum allowed
 * (`args.MaxFirePeriods`).
 */
std::unordered_map<int, std::vector<int>>
Cell2Fire::SendMessages()
{
    // Iterator
    std::unordered_map<int, Cells>::iterator it;

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
        printSets(this->availCells, this->nonBurnableCells, this->burningCells, this->burntCells, this->firebreakCells);
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
        // Get object from unordered map
        it = this->Cells_Obj.find(cell);

        // Cell's info
        if (this->args.verbose)
        {
            it->second.print_info();
        }

        /*
                        Manage Fire method main step
        */
        if (it->second.ROSAngleDir.size() > 0)
        {
            // std::cout << "Entra a Manage Fire" << std::endl;
            if (!this->args.BBOTuning)
            {
                //&df[cell-1] replaced by full df for getting the slopes
                aux_list = it->second.manageFire(this->fire_period[this->year - 1],
                                                 this->availCells,
                                                 df,
                                                 this->coef_ptr,
                                                 this->coordCells,
                                                 this->Cells_Obj,
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
            }

            else
            {
                // TODO  MAY 11: Pass the slope factor or the slopes of the
                // adjacent cells
                auto factors = BBOFactors.find(NFTypesCells[cell - 1]);
                aux_list = it->second.manageFireBBO(this->fire_period[this->year - 1],
                                                    this->availCells,
                                                    &df[cell - 1],
                                                    this->coef_ptr,
                                                    this->coordCells,
                                                    this->Cells_Obj,
                                                    this->args_ptr,
                                                    &this->wdf[this->weatherPeriod],
                                                    &this->FSCell,
                                                    &this->crownMetrics,
                                                    this->activeCrown,
                                                    this->ROSRV,
                                                    this->perimeterCells,
                                                    factors->second,
                                                    this->crownState,
                                                    this->crownFraction,
                                                    this->surfFraction,
                                                    this->surfaceIntensities,
                                                    this->RateOfSpreads,
                                                    this->surfaceFlameLengths);
            }
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
            // ---- Filtro de aristas bloqueadas por barreras vectoriales ----------
            // Se descarta el paso a la vecina cuando el segmento centro-a-centro
            // cruza la geometria de una barrera. Sin modelo de cruce esto detiene el
            // fuego siempre, sea el rasgo de 5 m o de 100, y sin fugas diagonales.
            // Con --breach-factor/--spot-factor se permite si el alcance supera el
            // ancho cruzado, que sale de la geometria y no de contar celdas.
            if (!this->blockedArcs.empty())
            {
                const int fromId = it->second.realId;
                double L = this->maxFlameLengths[fromId - 1];
                if (L <= 0.0) L = this->surfaceFlameLengths[fromId - 1];
                const double U_ms = this->wdf[this->weatherPeriod].ws / 3.6;
                const double reach = std::max(this->args.BreachFactor * L,
                                              this->args.SpotFactor * L * U_ms);
                std::vector<int> keep;
                keep.reserve(aux_list.size());
                for (int to : aux_list)
                {
                    auto ba = this->blockedArcs.find(this->arcKey(fromId, to));
                    if (ba == this->blockedArcs.end() || reach >= ba->second)
                        keep.push_back(to);
                    else if (this->args.verbose)
                        std::cout << "  barrera bloquea " << fromId << " -> " << to
                                  << " (ancho " << ba->second << " m, alcance " << reach
                                  << " m)" << std::endl;
                }
                aux_list.swap(keep);
                if (aux_list.empty())
                {
                    this->burnedOutList.push_back(fromId);
                    continue;
                }
            }
            if (this->args.verbose)
                std::cout << "\nList is not empty" << std::endl;
            this->messagesSent = true;
            sendMessageList[it->second.realId] = aux_list;
            if (this->args.verbose)
            {
                std::cout << "Message list content" << std::endl;
                for (auto& msg : sendMessageList[it->second.realId])
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
            this->burnedOutList.push_back(it->second.realId);
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
    // ============ CRUCE DE BARRERA: BREACHING (Prometheus) + SPOTTING (Albini) ============
    // Una celda ardiendo puede cruzar una barrera no-quemable a favor del viento por:
    //   - Breaching (contacto de llama):  R_breach = BreachFactor * FL          (barreras angostas)
    //   - Spotting  (lofting de pavesas): d_spot   = SpotFactor  * FL * U_ms    (barreras anchas)
    // Cruza si  max(R_breach, d_spot) >= W (ancho de la barrera).  Determinista.
    // Si la barrera cruzada incluye celdas-rio (--river-shp), se registra el evento.
    if (this->args.BreachFactor > 0.0 || this->args.SpotFactor > 0.0)
    {
        double waz = this->wdf[this->weatherPeriod].waz;          // rumbo a favor del viento (compass)
        double U_ms = this->wdf[this->weatherPeriod].ws / 3.6;    // viento en m/s (ws en km/h)
        double dr = -std::cos(waz * M_PI / 180.0);
        double dc = std::sin(waz * M_PI / 180.0);
        std::vector<std::pair<int, int>> crossings;              // (origen, objetivo)
        std::unordered_set<int> src = this->burningCells;
        for (auto& b : this->burnedOutList) src.insert(b);
        for (auto& bc : src)
        {
            int id = bc;
            double L = this->maxFlameLengths[id - 1];                 // llama efectiva = max(copa, superficie) - compartido por todos los kernels
            if (L <= 0.0) L = this->surfaceFlameLengths[id - 1];
            if (L <= 0.0 && this->args.Simulator == "K")              // fallback KITRAL (usa arrays de KITRAL): solo --sim K
                L = flame_length_from_ros(&df[id - 1], this->RateOfSpreads[id - 1]);
            if (L <= 0.0) continue;
            double R_breach = this->args.BreachFactor * L;
            double d_spot = this->args.SpotFactor * L * U_ms;
            double reach = std::max(R_breach, d_spot);            // alcance total (m)
            if (reach <= 0.0) continue;
            int r0 = (id - 1) / this->cols;
            int c0 = (id - 1) % this->cols;
            // paso 1: barrera inmediata a favor del viento (no-quemable o rio)
            int r1 = (int)std::round(r0 + dr);
            int c1 = (int)std::round(c0 + dc);
            if (r1 < 0 || r1 >= this->rows || c1 < 0 || c1 >= this->cols) continue;
            int nid = r1 * this->cols + c1 + 1;
            // Barrera = no quemable, cortafuegos, o cualquier capa vectorial cargada.
            // OJO: firebreakCells (cortafuegos) NO estaba incluido antes, asi que el
            // breaching nunca cruzaba un cortafuegos -- justo el caso de uso central
            // (dimensionar su ancho). Incluirlo cambia resultados cuando se combinan
            // cortafuegos con --breach-factor/--spot-factor.
            bool barrier = (this->nonBurnableCells.find(nid) != this->nonBurnableCells.end())
                        || (this->firebreakCells.find(nid) != this->firebreakCells.end())
                        || (this->barrierCells.find(nid) != this->barrierCells.end());
            if (!barrier) continue;
            // caminar la barrera hasta la 1a celda quemable, midiendo W y el tipo cruzado
            int maxSteps = (int)std::ceil(reach / this->cellSide) + 2;
            int target = -1; double W = 0.0; bool crossedRiver = false;
            std::string crossedType = "barrier";
            // El vecino inmediato (s=1) se evalua aqui, fuera del bucle de abajo. Sin
            // esto, una barrera de UNA celda nunca fijaba el tipo y el cruce quedaba
            // atribuido como "barrier" generico (o no se registraba).
            {
                auto b1 = this->barrierCells.find(nid);
                if (b1 != this->barrierCells.end()) { crossedRiver = true; crossedType = b1->second; }
            }
            for (int s = 2; s <= maxSteps; ++s)
            {
                int rr = (int)std::round(r0 + dr * s);
                int cc = (int)std::round(c0 + dc * s);
                if (rr < 0 || rr >= this->rows || cc < 0 || cc >= this->cols) break;
                int cid = rr * this->cols + cc + 1;
                auto bIt = this->barrierCells.find(cid);
                if (bIt != this->barrierCells.end()) { crossedRiver = true; crossedType = bIt->second; }
                if (this->nonBurnableCells.find(cid) != this->nonBurnableCells.end()
                    || this->firebreakCells.find(cid) != this->firebreakCells.end()
                    || bIt != this->barrierCells.end())
                    continue;                                    // sigue en la barrera
                W = (s - 1) * this->cellSide;                    // ancho cruzado (m)
                if (this->availCells.find(cid) != this->availCells.end()) target = cid;
                break;
            }
            if (target > 0 && reach >= W)
            {
                crossings.push_back(std::make_pair(id, target));
                const char* mech = (d_spot >= W) ? "spotting" : "breaching";
                if (crossedRiver || this->args.verbose)
                    this->riverCrossingLog.push_back(
                        std::to_string(this->weatherPeriod) + "," + std::to_string(id) + ","
                        + std::to_string(target) + "," + mech + "," + std::to_string(W) + ","
                        + std::to_string(L) + "," + std::to_string(reach) + ","
                        + crossedType);
            }
        }
        for (auto& cr : crossings)
        {
            sendMessageList[cr.first].push_back(cr.second);
            this->messagesSent = true;
        }
        if (this->args.verbose && !crossings.empty())
            std::cout << "Cruce de barrera: " << crossings.size() << " evento(s)" << std::endl;
    }

    if (this->args.verbose)
        printSets(this->availCells, this->nonBurnableCells, this->burningCells, this->burntCells, this->firebreakCells);

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
 * burnt, firebreak).
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
    std::unordered_map<int, Cells>::iterator it;

    // Information of the current step
    if (this->args.verbose)
    {
        std::cout << "\n---------------------- Step 3: Receiving and processing "
                     "messages from Ignition ----------------------"
                  << std::endl;
        std::cout << "Current Fire Period: " << this->fire_period[this->year - 1] << std::endl;
        printSets(this->availCells, this->nonBurnableCells, this->burningCells, this->burntCells, this->firebreakCells);
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
        }
        this->burningCells.clear();
        if (this->args.verbose)
            printSets(
                this->availCells, this->nonBurnableCells, this->burningCells, this->burntCells, this->firebreakCells);
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
            if (this->Cells_Obj.find(bc) == this->Cells_Obj.end()
                && this->burntCells.find(bc) == this->burntCells.end())
            {
                // Initialize cell, insert it inside the unordered map
                InitCell(bc);
                it = this->Cells_Obj.find(bc);
            }
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
            if (this->burntCells.find(bc) == this->burntCells.end())
            {
                if (this->Cells_Obj.find(bc) == this->Cells_Obj.end())
                {

                    // Initialize cell, insert it inside the unordered map
                    InitCell(bc);
                    it = this->Cells_Obj.find(bc);
                }
                else
                    it = this->Cells_Obj.find(bc);

                // Check if burnable, then check potential ignition
                if (it->second.fType != 0)
                {
                    checkBurnt = it->second.get_burned(this->fire_period[this->year - 1],
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
                    std::cout << "\nCell " << it->second.realId << " got burnt (1 true, 0 false): " << checkBurnt
                              << std::endl;
                }

                // Update the burntlist
                if (checkBurnt)
                {
                    burntList.insert(it->second.realId);

                    // Cleaning step
                    // Fire can't be propagated back
                    int cellNum = it->second.realId - 1;
                    for (auto& angle : it->second.angleToNb)
                    {
                        int origToNew = angle.first;
                        // Which neighbor am I to the burnt cell
                        int newToOrig = (origToNew + 180) % 360;
                        int adjCellNum = angle.second;  // Check
                        auto adjIt = Cells_Obj.find(adjCellNum);
                        if (adjIt != Cells_Obj.end())
                        {
                            adjIt->second.ROSAngleDir.erase(newToOrig);
                        }
                    }
                }
            }
        }

        // Update sets
        for (auto& bc : burntList)
        {
            this->burntCells.insert(bc);
        }

        for (auto& bc : this->burnedOutList)
        {
            this->burntCells.insert(bc);
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
                this->availCells, this->nonBurnableCells, this->burningCells, this->burntCells, this->firebreakCells);
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
    // Iterator
    // Declare an iterator to unordered_map
    std::unordered_map<int, Cells>::iterator it;
    int i;

    for (auto& br : this->burntCells)
    {
        if (this->Cells_Obj.find(br) != this->Cells_Obj.end())
        {
            // Get object from unordered map
            it = this->Cells_Obj.find(br);

            // Initialize the fire fields for the selected cel
            it->second.status = 2;
        }
    }

    for (auto& br : this->burningCells)
    {
        if (this->Cells_Obj.find(br) != this->Cells_Obj.end())
        {
            // Get object from unordered map
            it = this->Cells_Obj.find(br);

            // Initialize the fire fields for the selected cel
            it->second.status = 2;
        }
    }

    // Final results for comparison with Python
    // DEBUGstd::cout  << "\n ------------------------ Final results for
    // comparison with Python ------------------------";

    // Final report
    float ACells = this->availCells.size();
    float BCells = this->burntCells.size();
    float NBCells = this->nonBurnableCells.size();
    float HCells = this->firebreakCells.size();

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

    // Log de cruces de barrera/rio (breaching + spotting)
    if (!this->riverCrossingLog.empty() && !this->args.OutFolder.empty())
    {
        std::string rcName = this->args.OutFolder + "RiverCrossings" + std::to_string(this->sim) + ".csv";
        std::ofstream rc(rcName);
        rc << "weatherPeriod,sourceCell,targetCell,mechanism,widthM,flameLenM,reachM,type\n";
        for (auto& line : this->riverCrossingLog) rc << line << "\n";
        rc.close();
        std::cout << "RiverCrossings: " << this->riverCrossingLog.size()
                  << " evento(s) -> " << rcName << std::endl;
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
        // CSVPloter.printCSVDouble_V2(this->FSCell.size() - this->nIgnitions, 4,
        // this->FSCell);
        CSVPloter.printCSVDouble_V2(this->FSCell.size() / 4, 4, this->FSCell);
    }

    // RateOfSpread
    if (this->args.OutRos)
    {
        std::string rosName;
        std::ostringstream oss;
        oss.str("");
        oss << std::setfill('0') << std::setw(this->widthSims) << this->sim;
        rosName = this->rosFolder + "ROSFile" + oss.str() + ".asc";
        if (this->args.verbose)
        {
            std::cout << "We are generating the Rate of Spread to a asc file " << rosName << std::endl;
        }
        CSVWriter CSVPloter(rosName, " ");
        CSVPloter.printASCII(
            this->rows, this->cols, this->xllcorner, this->yllcorner, this->cellSide, this->RateOfSpreads);
    }

    // Intensity
    if (this->args.OutIntensity)
    {
        this->surfaceIntensityFolder = this->args.OutFolder + "SurfaceIntensity" + separator();
        std::string intensityName;
        std::ostringstream oss;
        oss.str("");
        oss << std::setfill('0') << std::setw(this->widthSims) << this->sim;
        intensityName = this->surfaceIntensityFolder + "SurfaceIntensity" + oss.str() + ".asc";

        if (this->args.verbose)
        {
            std::cout << "We are generating the Byram Intensity to a asc file " << intensityName << std::endl;
        }
        CSVWriter CSVPloter(intensityName, " ");
        CSVPloter.printASCII(
            this->rows, this->cols, this->xllcorner, this->yllcorner, this->cellSide, this->surfaceIntensities);
    }

    // Crown Intensity
    if ((this->args.OutIntensity) && (this->args.AllowCROS)
        && ((this->args.Simulator == "S") || this->args.Simulator == "P" || this->args.Simulator == "K"))
    {
        this->crownIntensityFolder = this->args.OutFolder + "CrownIntensity" + separator();
        std::string intensityName;
        std::ostringstream oss;
        oss.str("");
        oss << std::setfill('0') << std::setw(this->widthSims) << this->sim;
        intensityName = this->crownIntensityFolder + "CrownIntensity" + oss.str() + ".asc";
        if (this->args.verbose)
        {
            std::cout << "We are generating the Crown Intensity to a asc file " << intensityName << std::endl;
        }
        CSVWriter CSVPloter(intensityName, " ");
        CSVPloter.printASCII(
            this->rows, this->cols, this->xllcorner, this->yllcorner, this->cellSide, this->crownIntensities);
    }

    // Surface Flame Length
    if (this->args.OutFl)
    {
        this->surfaceFlameLengthFolder = this->args.OutFolder + "SurfaceFlameLength" + separator();
        std::string flName;
        std::ostringstream oss;
        oss.str("");
        oss << std::setfill('0') << std::setw(this->widthSims) << this->sim;
        flName = this->surfaceFlameLengthFolder + "SurfaceFlameLength" + oss.str() + ".asc";
        if (this->args.verbose)
        {
            std::cout << "We are generating the Flame Length to a asc file " << flName << std::endl;
        }
        CSVWriter CSVPloter(flName, " ");
        CSVPloter.printASCII(
            this->rows, this->cols, this->xllcorner, this->yllcorner, this->cellSide, this->surfaceFlameLengths);
    }

    // Crown Flame length
    if ((this->args.OutFl) && (this->args.AllowCROS) && ((this->args.Simulator == "S") || this->args.Simulator == "P" || this->args.Simulator == "K"))
    {
        this->crownFlameLengthFolder = this->args.OutFolder + "CrownFlameLength" + separator();
        std::string fileName;
        std::ostringstream oss;
        oss.str("");
        oss << std::setfill('0') << std::setw(this->widthSims) << this->sim;
        fileName = this->crownFlameLengthFolder + "CrownFlameLength" + oss.str() + ".asc";
        if (this->args.verbose)
        {
            std::cout << "We are generating the Crown Flame Length to a asc file " << fileName << std::endl;
        }
        CSVWriter CSVPloter(fileName, " ");
        CSVPloter.printASCII(
            this->rows, this->cols, this->xllcorner, this->yllcorner, this->cellSide, this->crownFlameLengths);
    }

    // Max Flame length
    if ((this->args.OutFl) && (this->args.AllowCROS) && ((this->args.Simulator == "S") || this->args.Simulator == "P" || this->args.Simulator == "K"))
    {
        this->maxFlameLengthFolder = this->args.OutFolder + "MaxFlameLength" + separator();
        std::string fileName;
        std::ostringstream oss;
        oss.str("");
        oss << std::setfill('0') << std::setw(this->widthSims) << this->sim;
        fileName = this->maxFlameLengthFolder + "MaxFlameLength" + oss.str() + ".asc";
        if (this->args.verbose)
        {
            std::cout << "We are generating the Max Flame Length to a asc file " << fileName << std::endl;
        }
        CSVWriter CSVPloter(fileName, " ");
        CSVPloter.printASCII(
            this->rows, this->cols, this->xllcorner, this->yllcorner, this->cellSide, this->maxFlameLengths);
    }

    // Intensity
    if ((this->args.OutCrownConsumption) && (this->args.AllowCROS))
    {
        this->cfbFolder = this->args.OutFolder + "CrownFractionBurn" + separator();
        std::string cfbName;
        std::ostringstream oss;
        oss.str("");
        oss << std::setfill('0') << std::setw(this->widthSims) << this->sim;
        cfbName = this->cfbFolder + "Cfb" + oss.str() + ".asc";
        if (this->args.verbose)
        {
            std::cout << "We are generating the Crown Fraction Burn to a asc file " << cfbName << std::endl;
        }
        CSVWriter CSVPloter(cfbName, " ");
        CSVPloter.printASCII(
            this->rows, this->cols, this->xllcorner, this->yllcorner, this->cellSide, this->crownFraction);
    }

    // Intensity
    if ((this->args.OutSurfConsumption) && (this->args.Simulator == "C"))
    {
        this->sfbFolder = this->args.OutFolder + "SurfFractionBurn" + separator();
        std::string sfbName;
        std::ostringstream oss;
        oss.str("");
        oss << std::setfill('0') << std::setw(this->widthSims) << this->sim;
        sfbName = this->sfbFolder + "Sfb" + oss.str() + ".asc";
        if (this->args.verbose)
        {
            std::cout << "We are generating the Surface Fraction Burn to a asc file " << sfbName << std::endl;
        }
        CSVWriter CSVPloter(sfbName, " ");
        CSVPloter.printASCII(
            this->rows, this->cols, this->xllcorner, this->yllcorner, this->cellSide, this->surfFraction);
    }

    // Crown
    if ((this->args.OutCrown) && (this->args.AllowCROS))
    {
        this->crownFolder = this->args.OutFolder + "CrownFire" + separator();
        std::string crownName;
        std::ostringstream oss;
        oss.str("");
        oss << std::setfill('0') << std::setw(this->widthSims) << this->sim;
        crownName = this->crownFolder + "Crown" + oss.str() + ".asc";
        if (this->args.verbose)
        {
            std::cout << "We are generating the Crown behavior to a asc file " << crownName << std::endl;
        }
        CSVWriter CSVPloter(crownName, " ");
        // CSVPloter.printCrownAscii(this->rows, this->cols, this->xllcorner,
        // this->yllcorner, this->cellSide, this->crownMetrics, statusCells2);
        // /OLD VERSION
        CSVPloter.printASCIIInt(
            this->rows, this->cols, this->xllcorner, this->yllcorner, this->cellSide, this->crownState);
    }

    // Ignition Logfile
    if (currentSim == args.TotalSims && this->args.IgnitionsLog)
    {

        if (this->args.verbose)
        {
            std::cout << "\nWriting (simulation, cell & weather ids): ";
            for (i = 1; i < IgnitionHistory.size() + 1; i++)
            {
                std::cout << "(" << i << "," << IgnitionHistory[i] << "," << WeatherHistory[i] << ")\t";
            }
        }

        CSVWriter ignitionsFile(this->args.OutFolder + sim_log_filename);
        ignitionsFile.printIgnitions(IgnitionHistory, WeatherHistory);
    }
}

/**
 * @brief Outputs the current state of the forest grid to a CSV file.
 *
 * This method generates a binary representation of the forest grid, with cell
 * statuses indicating their condition (burning, burnt, firebreak, etc.) during
 * the simulation. The file is saved with a unique name in the designated
 * output folder.
 *
 * ### Behavior:
 * - **Cell Status Update**:
 *   - Updates a vector (`statusCells2`) to reflect the current statuses of
 * cells:
 *     - `1` for burning or burnt cells.
 *     - `-1` for firebreak cells.
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
    for (auto& hc : this->firebreakCells)
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
        printSets(this->availCells, this->nonBurnableCells, this->burningCells, this->burntCells, this->firebreakCells);
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
        this->CSVWeather.parseWeatherDF(this->wdf, this->args_ptr, this->WeatherData, WPeriods);
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
        this->CSVWeather.parseWeatherDF(this->wdf, this->args_ptr, this->WeatherData, WPeriods);
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
    GenDataFile(args.InFolder, args.Simulator);
    int ep = 0;
    // Episodes loop (episode = replication)
    // CP: Modified to account the case when no ignition occurs and no grids are
    // generated (currently we are not generating grid when no ignition happened,
    // TODO)
    int num_threads = args.nthreads;
    Cell2Fire Forest2(args);  // generate Forest object
    std::vector<Cell2Fire> Forests(num_threads, Forest2);
    if (args.Simulator == "K")
    {
        setup_const();
    }
    else if (args.Simulator == "S")
    {
    }
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

        Cell2Fire Forest = Forests[TID];
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
    delete[] df;
    return 0;
}
