#ifndef READCSV
#define READCSV

#include "Cells.h"
#include "ReadArgs.h"

#include <gdal/gdal_priv.h>
#include <iostream>
#include <fstream>
#include <vector>
#include <iterator>
#include <string>
#include <algorithm>
#include <boost/algorithm/string.hpp>
#include <boost/variant.hpp>
#include <unordered_map>
#include <unordered_set>
#include <errno.h>

void getTifData(std::string, inputs *df_ptr, const char *pszFilename, int number_of_arguments, int raster_band);

/*
 *   Forest structure
 */
typedef struct
{
	std::vector<std::vector<int>> coordCells;
} forestDF;

/*
 * A class to read data from a csv file.
 */
class CSVReader
{
public:
	// inmutable
	std::string fileName;
	std::string delimeter;
	const char *filenamechar;

	// Constructor
	CSVReader(std::string filename, std::string delm = ",");

	// Function to fetch data from a CSV File
	std::vector<std::vector<std::string>> getData();
	void readLookUpTable(inputs *df_ptr, int nCells);

	std::tuple<const char *, const char *, int, int, float, float, float> readTifMetadata(arguments *args_ptr);

	// Print data to console (Debug)
	void printData(std::vector<std::vector<std::string>> &DF);

	void readTifArgs(inputs *df_ptr, arguments *args_ptr, int nargs, int raster_band);
	void readTifFile(inputs *df_ptr, const char *file, int raster_band);

	// Populate DF (Spanish version)
	void parseDF(inputs *df_ptr, std::vector<std::vector<std::string>> &DF, arguments *args_ptr, int NCells);

	// Populate NFtypes (Spanish version)
	void parseNDF(std::vector<int> &NFTypes, std::vector<std::vector<std::string>> &DF, int NCells);

	// Populate Probabilities
	void parsePROB(std::vector<float> &probabilities, std::vector<std::vector<std::string>> &DF, int NCells);
	void parsePROB2(std::vector<float> &probabilities, std::string ignition_prob);

	// Populate Weather DF (Spanish version)
	void parseWeatherDF(weatherDF *wt_ptr, arguments *args_ptr, std::vector<std::vector<std::string>> &DF, int WPeriods);

	// Populate Ignition Points
	void parseIgnitionDF(std::vector<int> &ig, std::vector<std::vector<std::string>> &DF, int IgPeriods);

	// Populates ForestDF
	void parseForestDF(forestDF *frt_ptr, inputs *df_ptr, int rows, int cols, int cellside, double xllcorner, double yllcorner);

	// Populate Harvested Cells
	void parseHarvestedDF(std::unordered_map<int, std::vector<int>> &hc, std::vector<std::vector<std::string>> &DF, int HPeriods);

	// Populate BBO Factors
	void parseBBODF(std::unordered_map<int, std::vector<float>> &bbo, std::vector<std::vector<std::string>> &DF, int NFTypes);

	// Prints individual cell info
	void printDF(inputs df);

	void printDataTif(inputs *df, int Ncells);

	// Prints individual weather row info
	void printWeatherDF(weatherDF wdf);
};

#endif
