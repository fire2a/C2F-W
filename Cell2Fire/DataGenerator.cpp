// author = "Matias Vilches"
/**
 *@file
 */

#include "DataGenerator.h"

#include "tiffio.h"
#include <algorithm>
#include <array>
#include <cassert>
#include <cmath>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <limits>
#include <memory>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

inline char
separator()
{
#if defined _WIN32 || defined __CYGWIN__
    return '\\';
#else
    return '/';
#endif
}

/**
 * @brief Reads the model lookup table and creates dictionaries for the fuel types and cell's ColorsDict.
 *
 * The function will look for either `spain_lookup_table.csv`, `kitral_lookup_table.csv` or `fbp_lookup_table.csv`
 * in the input instance directory, depending on the chosen model.
 *
 * This file should have the following columns:
 * - grid_value: numeric id for fuel type within grid
 * - export_value: numeric id for fuel type
 * - descriptive_name: description of fuel type
 * - fuel_type: code for fuel type
 * - r: red
 * - g: green
 * - b: blue
 * - h: hue
 * - s: saturation
 * - l: lightness
 *
 * The function creates a fuel type map `<grid_value, fuel_type>` and a color map `<grid_value, r, g, b, 1.0>`
 *
 * @param filename Name of file containing the lookup table for the chosen simulation model
 * @return a tuple with a mapping of fuel code per fuel numeric id, and a mapping of color per fuel numeric id.
 */
std::tuple<std::unordered_map<std::string, std::string>,
           std::unordered_map<std::string, std::tuple<float, float, float, float>>>
Dictionary(const std::string& filename)
{

    std::unordered_map<std::string, std::string> FBPDict;
    std::unordered_map<std::string, std::tuple<float, float, float, float>> ColorsDict;

    int aux = 1;
    std::ifstream file(filename);
    std::string line;

    // Read file and save ColorsDict and ftypes dictionaries
    while (std::getline(file, line))
    {
        if (aux > 1)
        {
            aux += 1;

            // Replace hyphen
            size_t hyphenPos = line.find('-');
            while (hyphenPos != std::string::npos)
            {
                line.replace(hyphenPos, 1, "");
                hyphenPos = line.find('-');
            }

            // Replace newline
            size_t newlinePos = line.find('\n');
            while (newlinePos != std::string::npos)
            {
                line.replace(newlinePos, 1, "");
                newlinePos = line.find('\n');
            }

            // Replace "No" with "NF"
            size_t noPos = line.find("No");
            while (noPos != std::string::npos)
            {
                line.replace(noPos, 2, "NF");
                noPos = line.find("No");
            }

            // Split the line into tokens
            std::istringstream ss(line);
            std::vector<std::string> tokens;
            std::string token;
            while (std::getline(ss, token, ','))
            {
                tokens.push_back(token);
            }

            if (tokens[3].substr(0, 3) == "FM1")
            {
                FBPDict[tokens[0]] = tokens[3].substr(0, 4);
            }
            else if (tokens[3].substr(0, 3) == "Non" || tokens[3].substr(0, 3) == "NFn")
            {
                FBPDict[tokens[0]] = "NF";
            }
            else
            {
                FBPDict[tokens[0]] = tokens[3].substr(0, 3);
            }

            ColorsDict[tokens[0]] = std::make_tuple(
                std::stof(tokens[4]) / 255.0f, std::stof(tokens[5]) / 255.0f, std::stof(tokens[6]) / 255.0f, 1.0f);
        }

        if (aux == 1)
        {
            aux += 1;
        }
    }

    return std::make_tuple(FBPDict, ColorsDict);
}

// ForestGrid function
/**
 * @brief Reads fuels from ASCII raster file and creates a list of cells.
 *
 * @param filename Name of file containing raster of fuel type per cell
 * @param Dictionary Map of fuel numeric ID in raster grid to fuel model code.
 * @return A tuple with an array of the cells' fuel type numeric ID and an array of the cells' fuel type code,
 * the number of rows, the number of columns and the size of a cell.
 */
std::tuple<std::vector<int>, std::vector<std::string>, int, int, float>
ForestGrid(const std::string& filename, const std::unordered_map<std::string, std::string>& Dictionary)
{
    std::ifstream file(filename);
    if (!file.is_open())
    {
        throw std::runtime_error("Error: Could not open file '" + filename + "'");
    }

    std::vector<std::string> filelines;

    // Read all lines from the file
    std::string line;
    while (std::getline(file, line))
    {
        filelines.push_back(line);
    }

    // Extract cellsize from line 5
    line = filelines[4];
    line.erase(std::remove(line.begin(), line.end(), '\n'), line.end());
    std::istringstream iss(line);
    std::string key, value;

    if (!(iss >> key >> value) || key != "cellsize")
    {
        std::cerr << "line=" << line << std::endl;
        throw std::runtime_error("Expected 'cellsize' on line 5 of " + filename);
    }

    float cellsize = std::stof(value);

    int cells = 0;
    int tFBPDicts = 0;
    int tcols = 0;
    std::vector<std::string> gridcell1;
    std::vector<std::string> gridcell2;
    std::vector<int> gridcell3;
    std::vector<std::string> gridcell4;
    std::vector<std::vector<std::string>> grid;
    std::vector<std::vector<std::string>> grid2;

    // Read the ASCII file with the grid structure
    for (size_t i = 6; i < filelines.size(); ++i)
    {
        line = filelines[i];

        // Remove newline characters
        line.erase(std::remove(line.begin(), line.end(), '\n'), line.end());

        // Remove leading and trailing whitespaces
        line.erase(line.begin(), std::find_if(line.begin(), line.end(), [](char c) { return !std::isspace(c); }));
        line.erase(std::find_if(line.rbegin(), line.rend(), [](char c) { return !std::isspace(c); }).base(),
                   line.end());

        std::istringstream iss(line);
        std::string token;
        while (iss >> token)
        {
            if (Dictionary.find(token) == Dictionary.end())
            {
                gridcell1.push_back("NF");
                gridcell2.push_back("NF");
                gridcell3.push_back(0);
                gridcell4.push_back("NF");
            }
            else
            {
                gridcell1.push_back(token);
                gridcell2.push_back(Dictionary.at(token));
                gridcell3.push_back(std::stoi(token));
                gridcell4.push_back(Dictionary.at(token));
            }
            tcols = std::max(tcols, static_cast<int>(gridcell1.size()));
        }

        grid.push_back(gridcell1);
        grid2.push_back(gridcell2);
        gridcell1.clear();
        gridcell2.clear();
    }
    // Adjacent list of dictionaries and Cells coordinates
    std::vector<std::array<int, 2>> CoordCells;
    CoordCells.reserve(grid.size() * tcols);
    int n = 1;
    tcols += 1;

    return std::make_tuple(gridcell3, gridcell4, grid.size(), tcols - 1, cellsize);
}

/**
 * @brief checks if a file exists.
 * @param filename Name of file
 * @return True if file can be opened, False if not.
 */
bool
fileExists(const std::string& filename)
{
    std::ifstream file(filename);
    return file.good();
}

/**
 * @brief Read grid data from ASCII file into a vector
 *
 * Transforms a raster layer into an array of information.
 *
 * @param filename name of file to read
 * @param data vector where information will be stored
 * @param nCells number of cells
 */
void
DataGrids(const std::string& filename, std::vector<float>& data, int nCells)
{
    std::ifstream file(filename);
    if (!file.is_open())
    {
        std::cerr << "Error: Could not open file '" << filename << "'" << std::endl;
        return;
    }

    std::vector<std::string> filelines;

    // Read all lines from the file
    std::string line;
    while (std::getline(file, line))
    {
        filelines.push_back(line);
    }

    // Extract cellsize from line 5
    line = filelines[4];
    line.erase(std::remove(line.begin(), line.end(), '\n'), line.end());
    std::istringstream iss(line);
    std::string key, value;

    if (!(iss >> key >> value) || key != "cellsize")
    {
        std::cerr << "Error: Expected 'cellsize' on line 5 of " << filename << std::endl;
        return;
    }

    float cellsize = std::stof(value);

    int aux = 0;

    // Read the ASCII file with the grid structure
    for (size_t i = 6; i < filelines.size(); ++i)
    {
        line = filelines[i];
        line.erase(std::remove(line.begin(), line.end(), '\n'), line.end());
        line = ' ' + line;
        std::istringstream iss(line);
        std::string token;

        while (iss >> token)
        {
            data[aux++] = std::stof(token);
            if (aux == nCells)
            {
                return;  // Stop reading if we've filled the data vector
            }
        }
    }
}

/**
 * @brief Reads fuels raster file in tif format and creates a list of cells.
 *
 * @param filename Name of fuel data file.
 * @param Dictionary Mapping of fuel type numeric ID to fuel type code.
 * @return A tuple containing: vector of fuel type per cell as numeric ID, vector of fuel type per cell as model code,
 * number of rows in grid, number of columns in grid, cell size.
 */
std::tuple<std::vector<int>, std::vector<std::string>, int, int, float>
ForestGridTif(const std::string& filename, const std::unordered_map<std::string, std::string>& Dictionary)
{
    // Tries to open file
    std::cout << filename << '\n';
    TIFF* fuelsDataset = TIFFOpen(filename.c_str(), "r");
    if (!fuelsDataset)
    {
        throw std::runtime_error("Error: Could not open file '" + filename + "'");
    }

    int cells = 0;
    int tFBPDicts = 0;
    int tcols = 0;
    // Vectors with fuel information in different formats
    std::vector<std::string> gridcell1;
    std::vector<std::string> gridcell2;
    std::vector<int> gridcell3;
    std::vector<std::string> gridcell4;
    std::vector<std::vector<std::string>> grid;
    std::vector<std::vector<std::string>> grid2;
    // Get Raster dimentions
    uint32_t nXSize, nYSize;
    uint16_t data_type;
    uint16_t samplesPerPixel;
    TIFFGetField(fuelsDataset, TIFFTAG_IMAGEWIDTH, &nXSize);
    TIFFGetField(fuelsDataset, TIFFTAG_IMAGELENGTH, &nYSize);
    TIFFGetField(fuelsDataset, TIFFTAG_SAMPLEFORMAT, &data_type);
    TIFFGetField(fuelsDataset, TIFFTAG_SAMPLESPERPIXEL, &samplesPerPixel);
    tsize_t scan_size = TIFFRasterScanlineSize(fuelsDataset);
    std::cout << scan_size << '\n';
    std::cout << nXSize << '\n';
    std::cout << data_type << '\n';
    int n_bits = (scan_size / nXSize) * 8;
    std::cout << n_bits << '\n';
    double* modelPixelScale;
    uint32_t count;
    // TIFFGetField(tiff, 33424, &count, &data);
    TIFFGetField(fuelsDataset, 33550, &count, &modelPixelScale);
    // Gets cell size
    double cellSizeX{ modelPixelScale[0] };
    double cellSizeY{ modelPixelScale[1] };
    const double epsilon = std::numeric_limits<double>::epsilon();
    if (fabs(cellSizeX - cellSizeY) > epsilon)
    {
        throw std::runtime_error("Error: Cells are not square in: '" + filename + "'");
    }
    // Read raster data
    // Allocate memory for one row of pixel data
    void* buf;
    if (n_bits == 64)
    {
        buf = (double*)_TIFFmalloc(nXSize * sizeof(double));
    }
    else if (n_bits == 32)
    {
        buf = (int32_t*)_TIFFmalloc(nXSize * sizeof(int32_t));
    }
    else
    {
        throw std::runtime_error("Error: file type is not supported: '" + filename + "'");
    }

    if (!buf)
    {
        TIFFClose(fuelsDataset);
        throw std::runtime_error("Could not allocate memory");
    }
    // For each row
    for (int i = 0; i < nYSize; i++)
    {
        // Read pixel values for the current row
        if (TIFFReadScanline(fuelsDataset, buf, i) != 1)
        {
            _TIFFfree(buf);
            TIFFClose(fuelsDataset);
            throw std::runtime_error("Read error on row " + std::to_string(i));
        }
        // For each column
        float pixelValue;
        std::string token;
        for (int j = 0; j < nXSize; j++)
        {
            if (n_bits == 64)
            {
                double* values = (double*)buf;
                pixelValue = ((double*)buf)[j];
                token = std::to_string(static_cast<int>(((double*)buf)[j]));
            }
            else
            {
                float* values = (float*)buf;
                pixelValue = ((int32_t*)buf)[j];
                token = std::to_string(static_cast<int>(((int32_t*)buf)[j]));
            }
            // Access the pixel value at position (i, j)
            // std::cout << token << '\n';
            // std::cout << pixelValue << '\n';
            if (pixelValue != pixelValue || Dictionary.find(token) == Dictionary.end())
            {
                // If fuel not in Dictionary:
                gridcell1.push_back("NF");

                gridcell2.push_back("NF");

                gridcell3.push_back(0);
                gridcell4.push_back("NF");
            }
            else
            {

                gridcell1.push_back(token);
                gridcell2.push_back(Dictionary.at(token));
                gridcell3.push_back(std::stoi(token));
                gridcell4.push_back(Dictionary.at(token));
            }
            tcols = std::max(tcols, static_cast<int>(gridcell1.size()));
        }
        grid.push_back(gridcell1);
        grid2.push_back(gridcell2);
        gridcell1.clear();
        gridcell2.clear();
    }
    std::vector<std::array<int, 2>> CoordCells;
    CoordCells.reserve(grid.size() * tcols);
    int n = 1;
    tcols += 1;
    return std::make_tuple(gridcell3, gridcell4, grid.size(), tcols - 1, cellSizeX);
}

/**
 * @brief This functionality is not currently available
 *
 * Read grid data from Tif file into a vector.
 * Transforms a raster layer into an array of information.
 *
 * @param filename name of file to read
 * @param data vector where information will be stored
 * @param nCells number of cells
 */
void
DataGridsTif(const std::string& filename, std::vector<float>& data, int nCells)
{
    /*
    Reads fuel data from a .tif
    Args:
    filename (std::string): Name of .tif file.
    Dictionary (std::unordered_map<std::string, std::string>&): Reference to
    fuels dictionary

    Returns:
        Fuel vectors, number of cells y cell size (tuple[std::vector<int>,
    std::vector<std::string>)
    */
    // Tries to open file
    std::cout << filename << '\n';
    TIFF* fuelsDataset = TIFFOpen(filename.c_str(), "r");
    if (!fuelsDataset)
    {
        throw std::runtime_error("Error: Could not open file '" + filename + "'");
    }

    std::vector<std::string> filelines;
    // Get cell side
    double* modelPixelScale;
    uint32_t count;
    // TIFFGetField(tiff, 33424, &count, &data);
    TIFFGetField(fuelsDataset, 33550, &count, &modelPixelScale);
    // Gets cell size
    double cellSizeX{ modelPixelScale[0] };
    double cellSizeY{ modelPixelScale[1] };
    const double epsilon = std::numeric_limits<double>::epsilon();
    if (fabs(cellSizeX - cellSizeY) > epsilon)
    {
        throw std::runtime_error("Error: Cells are not square in: '" + filename + "'");
    }
    double cellsize;
    cellsize = cellSizeX;
    // Get Raster dimentions
    uint32_t nXSize, nYSize;
    TIFFGetField(fuelsDataset, TIFFTAG_IMAGEWIDTH, &nXSize);
    TIFFGetField(fuelsDataset, TIFFTAG_IMAGELENGTH, &nYSize);
    tsize_t scan_size = TIFFRasterScanlineSize(fuelsDataset);
    int n_bits = (scan_size / nXSize) * 8;
    int aux = 0;
    // Read raster data
    // Allocate memory for one row of pixel data
    void* buf;
    if (n_bits == 64)
    {
        buf = (double*)_TIFFmalloc(nXSize * sizeof(double));
    }
    else if (n_bits == 32)
    {
        buf = (float*)_TIFFmalloc(nXSize * sizeof(float));
    }
    else
    {
        throw std::runtime_error("Error: file type is not supported: '" + filename + "'");
    }
    if (!buf)
    {
        TIFFClose(fuelsDataset);
        throw std::runtime_error("Could not allocate memory");
    }
    // For each row
    // For each column
    float pixelValue;
    for (int i = 0; i < nYSize; i++)
    {
        // Read pixel values for the current row
        if (TIFFReadScanline(fuelsDataset, buf, i) != 1)
        {
            _TIFFfree(buf);
            TIFFClose(fuelsDataset);
            throw std::runtime_error("Read error on row " + std::to_string(i));
        }
        // For each column
        for (int j = 0; j < nXSize; j++)
        {
            // Access the pixel value at position (i, j)
            if (n_bits == 64)
            {
                double* values = (double*)buf;
                pixelValue = ((double*)buf)[j];
            }
            else
            {
                float* values = (float*)buf;
                pixelValue = ((int32_t*)buf)[j];
            }
            if (pixelValue == pixelValue)
            {
                // std::cout << token << '\n';
                data[aux] = pixelValue;
            }
            else
            {
                data[aux] = pixelValue;
            }
            aux++;
            if (aux == nCells)
            {
                return;  // Stop reading if we've filled the data vector
            }
        }
    }
}

/**
 * @brief Create input data matrix
 *
 * @param GFuelType Array of fuel model codes
 * @param GFuelTypeN Array of fuel model ids
 * @param Elevation Array of terrain elevations
 * @param PS Array of slope
 * @param SAZ Array of slope azimuth
 * @param Curing Array of curing levels
 * @param CBD Array of canopy bulk densities
 * @param CBH Array of canopy base heights
 * @param CCF Array of canopy cover fractions
 * @param ProbMap Array of ignition probabilities
 * @param FMC Array of foliage moisture content
 * @param TreeHeight Array of tree heights
 * @param CombatPoints Array of points where firefighting is likely
 * @param InFolder Input data directory
 * @return An array of arrays representing a cell.
 */
std::vector<std::vector<std::unique_ptr<std::string>>>
GenerateDat(const std::vector<std::string>& GFuelType,
            const std::vector<int>& GFuelTypeN,
            const std::vector<float>& Elevation,
            const std::vector<float>& PS,
            const std::vector<float>& CBD,
            const std::vector<float>& CBH,
            const std::vector<float>& CCF,
            const std::vector<float>& ProbMap,
            const std::vector<float>& FMC,
            const std::vector<float>& TreeHeight,
            const std::vector<float>& CombatPoints,
            const std::string& InFolder)
{
    // DF columns
    std::vector<std::string> Columns
        = { "fueltype", "elev", "ps", "cbd", "cbh", "ccf", "ftypeN", "fmc", "probMap", "tree_height", "combat_point" };

    // Create a vector to store unique_ptr of ~BaseData
    std::vector<std::vector<std::unique_ptr<std::string>>> dataGrids;
    // Dataframe
    // std::vector<std::vector<boost::any>> DF(GFuelType.size(),
    // std::vector<boost::any>(Columns.size()));

    // Populate DF
    for (size_t i = 0; i < GFuelType.size(); ++i)
    {

        std::vector<std::unique_ptr<std::string>> rowData;
        // Fuel Type 0
        rowData.emplace_back(std::make_unique<std::string>(GFuelType[i]));

        // Elevation 1
        if (std::isnan(Elevation[i]))

        {
            rowData.emplace_back(std::make_unique<std::string>(""));
        }
        else
        {
            rowData.emplace_back(std::make_unique<std::string>(std::to_string(Elevation[i])));
        }
        // PS 2
        if (std::isnan(PS[i]))
        {
            rowData.emplace_back(std::make_unique<std::string>(""));
        }
        else
        {
            rowData.emplace_back(std::make_unique<std::string>(std::to_string(PS[i])));
        }
        // CBD 3
        if (std::isnan(CBD[i]))
        {
            rowData.emplace_back(std::make_unique<std::string>(""));
        }
        else
        {
            rowData.emplace_back(std::make_unique<std::string>(std::to_string(CBD[i])));
        }
        // CBH 4
        if (std::isnan(CBH[i]))
        {
            rowData.emplace_back(std::make_unique<std::string>(""));
        }
        else
        {
            rowData.emplace_back(std::make_unique<std::string>(std::to_string(CBH[i])));
        }
        // CCF 5
        if (std::isnan(CCF[i]))
        {
            rowData.emplace_back(std::make_unique<std::string>(""));
        }
        else
        {
            rowData.emplace_back(std::make_unique<std::string>(std::to_string(CCF[i])));
        }
        // Fuel Type N 6
        if (std::isnan(static_cast<double>(GFuelTypeN[i])))
        {
            rowData.emplace_back(std::make_unique<std::string>(""));
        }
        else
        {
            rowData.emplace_back(std::make_unique<std::string>(std::to_string(GFuelTypeN[i])));
        }

        // FMC 7
        if (std::isnan(FMC[i]))
        {
            rowData.emplace_back(std::make_unique<std::string>(""));
        }
        else
        {
            rowData.emplace_back(std::make_unique<std::string>(std::to_string(FMC[i])));
        }

        // ProbMap 8
        if (std::isnan(ProbMap[i]))
        {
            rowData.emplace_back(std::make_unique<std::string>(""));
        }
        else
        {
            rowData.emplace_back(std::make_unique<std::string>(std::to_string(ProbMap[i])));
        }

        // TreeHeight 9

        if (std::isnan(TreeHeight[i]))
        {
            rowData.emplace_back(std::make_unique<std::string>(""));
        }
        else
        {
            rowData.emplace_back(std::make_unique<std::string>(std::to_string(TreeHeight[i])));
        }
        // Combat 10
        if (std::isnan(CombatPoints[i]))
        {
            rowData.emplace_back(std::make_unique<std::string>(""));
        }
        else
        {
            rowData.emplace_back(std::make_unique<std::string>(std::to_string(CombatPoints[i])));
        }

        // Add the rowData to dataGrids
        dataGrids.push_back(std::move(rowData));

        rowData.clear();
    }

    return dataGrids;
}

/**
 * @brief Save data matrix into a CSV file called `Data.csv`.
 * @param dataGrids Array of arrays storing each cell's input data
 * @param InFolder Directory where the CSV file will be created.
 */
void
writeDataToFile(const std::vector<std::vector<std::unique_ptr<std::string>>>& dataGrids, const std::string& InFolder)
{

    std::ofstream dataFile(InFolder + separator() + "Data.csv");
    std::vector<std::string> Columns = { "fueltype", "elev", "ps",          "cbd",         "cbh",         "ccf",
                                         "ftypeN",   "fmc",  "probability", "tree_height", "combat_point" };
    if (dataFile.is_open())
    {
        // Write header
        for (const auto& col : Columns)
        {
            dataFile << col << ",";
        }
        dataFile << "\n";

        // Write data
        for (const auto& rowData : dataGrids)
        {
            for (const auto& item : rowData)
            {
                dataFile << *item << ",";  // Dereference the unique_ptr before writing
            }
            dataFile << "\n";
        }
        dataFile.close();
    }
    else
    {
        std::cerr << "Error: Unable to open data file for writing" << std::endl;
    }
}

/**
 * @brief Reads all available input raster files and generates a new file called 'Data.csv' in which each row contains
 * the input data for a cell.
 *
 * If the 'fuels' raster file is in `tif` format, then it assumes all other rasters are also in that format.
 * Otherwise, it assumes `ascii` format.
 *
 * The function looks for the following files in `InFolder`:
 *  - elevation: terrain elevation [m]
 *  - saz: slope azimuth
 *  - slope: terrain slope
 *  - cur: curing level
 *  - cbd: canopy bulk density [km/m²]
 *  - cbh: canopy base height [m]
 *  - ccf: canopy cover fraction
 *  - probabilityMap: ignition probability map [%]
 *  - fmc: foliage moisture content
 *  - hm: tree height [m]
 *  - combatPoints
 *
 * The generated file contains the following columns: fuel type, latitude, longitude, elevation, wind speed (always
 * blank), wind direction (always blank), slope, slope azimuth, curing level, canopy bulk density, canopy base height,
 * canopy cover fraction, fuel type number ,foliage moisture content, ignition probability.
 * @param InFolder Input data directory
 * @param Simulator Simulation model code
 */
void
GenDataFile(const std::string& InFolder, const std::string& Simulator, std::string fuelsFile)
{
    std::cout << "\n------ Reading input data ------\n\n";
    std::unordered_map<std::string, std::string> FBPDict;
    std::unordered_map<std::string, std::tuple<float, float, float, float>> ColorsDict;

    // Determine the lookup table based on the Simulator
    std::string lookupTable;
    if (Simulator == "K")
    {
        lookupTable = InFolder + separator() + "kitral_lookup_table.csv";
    }
    else if (Simulator == "S")
    {
        lookupTable = InFolder + separator() + "spain_lookup_table.csv";
    }
    else if (Simulator == "C")
    {
        lookupTable = InFolder + separator() + "fbp_lookup_table.csv";
    }
    else if (Simulator == "P")
    {
        lookupTable = InFolder + separator() + "portugal_lookup_table.csv";
    }
    else
    {
        std::cerr << "Error: Simulator not recognized:" << Simulator << std::endl;
        exit(1);
    }

    // Check if the lookup table exists
    if (!fileExists(lookupTable))
    {
        std::cerr << "Error: Lookup table '" << lookupTable << "' not found" << std::endl;
        return;
    }

    // Call Dictionary function to read lookup table
    std::tie(FBPDict, ColorsDict) = Dictionary(lookupTable);

    // Call ForestGrid function
    // If fuels.tif exists, then .tif's are used, otherwise .asc
    std::string extension;
    std::string FGrid;
    if (!fuelsFile.empty())
    {
        std::string::size_type idx;
        idx = fuelsFile.rfind('.');

        if (idx != std::string::npos)
        {
            extension = fuelsFile.substr(idx);
            FGrid = InFolder + fuelsFile;
        }
    }
    else if (fileExists(InFolder + separator() + "fuels.tif"))
    {
        extension = ".tif";
        FGrid = InFolder + "fuels" + extension;
    }
    else
    {
        extension = ".asc";
        FGrid = InFolder + "fuels" + extension;
    }
    std::cout << "Using " << extension << '\n';
    // Call ForestGrid function
    std::vector<int> GFuelTypeN;
    std::vector<std::string> GFuelType;
    int FBPDicts, Cols;
    float CellSide;
    if (extension == ".tif")
    {
        std::tie(GFuelTypeN, GFuelType, FBPDicts, Cols, CellSide) = ForestGridTif(FGrid, FBPDict);
    }
    else
    {
        std::tie(GFuelTypeN, GFuelType, FBPDicts, Cols, CellSide) = ForestGrid(FGrid, FBPDict);
    }

    int NCells = GFuelType.size();

    // Call DataGrids function (formerly DataGrids)
    std::vector<float> Elevation(NCells, static_cast<float>(std::nanf("")));
    std::vector<float> PS(NCells, static_cast<float>(std::nanf("")));
    std::vector<float> CBD(NCells, static_cast<float>(std::nanf("")));
    std::vector<float> CBH(NCells, static_cast<float>(std::nanf("")));
    std::vector<float> CCF(NCells, static_cast<float>(std::nanf("")));
    std::vector<float> ProbMap(NCells, static_cast<float>(std::nanf("")));
    std::vector<float> FMC(NCells, static_cast<float>(std::nanf("")));
    std::vector<float> TreeHeight(NCells, static_cast<float>(std::nanf("")));
    std::vector<float> CombatPoints(NCells, static_cast<float>(std::nanf("")));

    std::vector<std::string> filenames = { "elevation" + extension, "slope" + extension, "cbd" + extension,
                                           "cbh" + extension,       "ccf" + extension,   "probabilityMap" + extension,
                                           "fmc" + extension,       "hm" + extension,    "combatPoints" + extension };

    for (const auto& name : filenames)
    {
        std::string filePath = InFolder + separator() + name;

        if (fileExists(filePath))
        {
            if (name == "elevation.asc")
            {
                DataGrids(filePath, Elevation, NCells);
            }
            else if (name == "slope.asc")
            {
                DataGrids(filePath, PS, NCells);
            }
            else if (name == "cbd.asc")
            {
                DataGrids(filePath, CBD, NCells);
            }
            else if (name == "cbh.asc")
            {
                DataGrids(filePath, CBH, NCells);
            }
            else if (name == "ccf.asc")
            {
                DataGrids(filePath, CCF, NCells);
            }
            else if (name == "probabilityMap.asc")
            {
                DataGrids(filePath, ProbMap, NCells);
            }
            else if (name == "fmc.asc")
            {
                DataGrids(filePath, FMC, NCells);
            }
            else if (name == "hm.asc")
            {
                DataGrids(filePath, TreeHeight, NCells);
            }
            else if (name == "combatPoints.asc")
            {
                DataGrids(filePath, CombatPoints, NCells);
            }
            else
            {
                // Handle the case where the file name doesn't match any
                // condition std::cout << "Unhandled file: " << name <<
                // std::endl;
            }
        }
        else
        {
            std::cout << "No " << name << " file, filling with NaN" << std::endl;
        }
    }

    // Call GenerateDat function
    std::vector<std::vector<std::unique_ptr<std::string>>> result = GenerateDat(
        GFuelType, GFuelTypeN, Elevation, PS, CBD, CBH, CCF, ProbMap, FMC, TreeHeight, CombatPoints, InFolder);

    writeDataToFile(result, InFolder);
    std::cout << "Generated data file" << std::endl;
}
