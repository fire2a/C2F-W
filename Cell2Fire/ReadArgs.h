#ifndef READARGS
#define READARGS

#include <algorithm>
#include <fstream>
#include <iostream>
#include <iterator>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>
/*
 *   Args structure
 */
// Se declara como 'struct arguments' y no como 'typedef struct {...} arguments;':
// con inicializadores por defecto en los miembros, la forma anonima con typedef
// dispara C5208 en MSVC, que WindowsTests.vcxproj trata como error.
// Factores de ajuste del ROS por codigo de combustible (fuel adjustment factors,
// FARSITE). Global porque Cells::manageFire los necesita y solo recibe 'arguments*';
// se llena una vez al cargar la instancia y no cambia durante la simulacion.
extern std::unordered_map<int, double> fuelAdjustment;
inline double
fuelAdjOf(int fuelCode)
{
    auto it = fuelAdjustment.find(fuelCode);
    return (it == fuelAdjustment.end()) ? 1.0 : it->second;
}

struct arguments
{
    std::string InFolder, OutFolder, WeatherOpt, FirebreakPlan, Simulator, WeatherWeightsFile, MoistureMode;
    std::string FchMode = "kitral";  // --fch-mode: kitral|emc|ffmc|isi
    std::string LbMode = "kitral";   // --lb-mode: kitral|sb
    bool FmcShading = false;         // --fmc-shading: acond. espacial humedad (sunny/shade)
    double BreachFactor = 0.0;       // --breach-factor k: breaching Prometheus (0=off). Cruza barrera si k*FL >= ancho
    double SpotFactor = 0.0;         // --spot-factor k: spotting Albini (0=off). Cruza si k*FL*viento(m/s) >= ancho
    std::string RiverShp = "";       // --river-shp: shapefile del rio (polilinea/poligono, CRS de la instancia)
    std::string IgnitionShp = "";    // --ignition-shp: shapefile de puntos de ignicion (1 punto = 1 anio)
    std::string ActiveFrontShp = ""; // --active-front-shp: frente activo como linea o puntos
    std::string RoadShp = "";        // --road-shp: caminos como barrera (mismo rol que el rio)
    bool UseRivers = false;          // --rivers: carga instancia/Rivers/*.shp
    bool UseRoads = false;           // --roads: carga instancia/Roads/*.shp
    bool UseFirebreaks = false;      // --firebreaks: carga instancia/Firebreaks/*.shp
    float BarrierWidth = 10.0;       // --barrier-width: ancho declarado (m) de las barreras dadas como polilinea
    float BarrierCover = 0.8;        // --barrier-cover: fraccion de celda cubierta para declararla no combustible
    std::string FuelAdjustmentFile = "";  // --fuel-adjustment: CSV fuel,factor por tipo de combustible
    std::string FirebreakShp = "";   // --firebreak-shp: cortafuegos desde un .shp puntual
    float Latitude = 0.0f;       // latitud (grados, +N) para humedad espacial Modo 2 (--latitude)
    bool HasLatitude = false;    // true si --latitude fue provisto (si no, usa data->lat por celda)
    std::string Scenario = "D2L2";  // escenario S&B DkLm (--scenario; reemplaza la columna del Weather)
    bool PortugalPreset = false;    // --sim P: preset Portugal (Fernandes) sobre motor S&B/rothermel_s
    bool OutMessages, OutFl, OutIntensity, OutRos, OutCrown, OutCrownConsumption, OutSurfConsumption, Trajectories,
        NoOutput, verbose, IgnitionsLog, Ignitions, OutputGrids, FinalGrid, PromTuned, Stats, BBOTuning, AllowCROS, UseWeatherWeights, ActiveFront;
    float ROSCV, ROSThreshold, CROSThreshold, HFIThreshold, HFactor, FFactor, BFactor, EFactor, FirePeriodLen;
    float CBDFactor, CCFFactor, ROS10Factor, CROSActThreshold;
    int MinutesPerWP, MaxFirePeriods, TotalYears, TotalSims, NWeatherFiles, IgnitionRadius, seed, nthreads, FMC,
        scenario;
    std::unordered_set<int> HCells, BCells;
};

char* getCmdOption(char** begin, char** end, const std::string& option);

bool cmdOptionExists(char** begin, char** end, const std::string& option);

void parseArgs(int argc, char* argv[], arguments* args_ptr);

void printArgs(arguments args);

int countWeathers(std::string directory_path);

#endif
