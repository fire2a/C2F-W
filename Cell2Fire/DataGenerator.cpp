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
#include <cstring>
#include <fstream>
#include <iostream>
#include <limits>
#include <memory>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

// Suppress libtiff warnings about GeoTIFF/GDAL metadata tags it doesn't know
// about (33550, 33922, 34736, 34737, 42113).  These are registered by
// libgeotiff/GDAL and are harmless; libtiff on Linux prints them as "Unknown
// field" warnings which confuse users.
static void suppressTiffWarnings(const char* /*module*/, const char* fmt, va_list /*ap*/)
{
    if (std::strstr(fmt, "Unknown field with tag") != nullptr) return;
    // Let other warnings reach stderr so real problems are visible
    // (we intentionally don't forward them here to avoid va_list re-use issues)
}
static const TIFFErrorHandler s_tiffWarningHandlerInstalled =
    (TIFFSetWarningHandler(suppressTiffWarnings), nullptr);

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
    int n_bits = (scan_size / nXSize) * 8;
    double* modelPixelScale;
    uint32_t count;
    TIFFGetField(fuelsDataset, 33550, &count, &modelPixelScale);
    double cellSizeX{ modelPixelScale[0] };
    double cellSizeY{ modelPixelScale[1] };
    const double epsilon = std::numeric_limits<double>::epsilon();
    if (fabs(cellSizeX - cellSizeY) > epsilon)
    {
        TIFFClose(fuelsDataset);
        throw std::runtime_error("Error: Cells are not square in: '" + filename + "'");
    }

    // Use SAMPLEFORMAT to distinguish float32 from int32 in the 32-bit case.
    bool is_float32 = (n_bits == 32 && data_type == SAMPLEFORMAT_IEEEFP);

    void* buf;
    if (n_bits == 64)
        buf = _TIFFmalloc(nXSize * sizeof(double));
    else if (n_bits == 32)
        buf = _TIFFmalloc(nXSize * sizeof(float));
    else
    {
        TIFFClose(fuelsDataset);
        throw std::runtime_error("Error: file type is not supported: '" + filename + "'");
    }
    if (!buf)
    {
        TIFFClose(fuelsDataset);
        throw std::runtime_error("Could not allocate memory");
    }

    for (int i = 0; i < nYSize; i++)
    {
        if (TIFFReadScanline(fuelsDataset, buf, i) != 1)
        {
            _TIFFfree(buf);
            TIFFClose(fuelsDataset);
            throw std::runtime_error("Read error on row " + std::to_string(i));
        }
        float pixelValue;
        std::string token;
        for (int j = 0; j < nXSize; j++)
        {
            if (n_bits == 64)
            {
                pixelValue = static_cast<float>(((double*)buf)[j]);
                token = std::to_string(static_cast<int>(((double*)buf)[j]));
            }
            else if (is_float32)
            {
                pixelValue = ((float*)buf)[j];
                token = std::to_string(static_cast<int>(pixelValue));
            }
            else
            {
                pixelValue = static_cast<float>(((int32_t*)buf)[j]);
                token = std::to_string(((int32_t*)buf)[j]);
            }
            if (pixelValue != pixelValue || Dictionary.find(token) == Dictionary.end())
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
    _TIFFfree(buf);
    TIFFClose(fuelsDataset);

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
    std::cout << filename << '\n';
    TIFF* fuelsDataset = TIFFOpen(filename.c_str(), "r");
    if (!fuelsDataset)
        throw std::runtime_error("Error: Could not open file '" + filename + "'");

    uint32_t nXSize, nYSize;
    TIFFGetField(fuelsDataset, TIFFTAG_IMAGEWIDTH, &nXSize);
    TIFFGetField(fuelsDataset, TIFFTAG_IMAGELENGTH, &nYSize);

    double* modelPixelScale;
    uint32_t count;
    TIFFGetField(fuelsDataset, 33550, &count, &modelPixelScale);
    const double epsilon = std::numeric_limits<double>::epsilon();
    if (fabs(modelPixelScale[0] - modelPixelScale[1]) > epsilon)
    {
        TIFFClose(fuelsDataset);
        throw std::runtime_error("Error: Cells are not square in: '" + filename + "'");
    }

    tsize_t scan_size = TIFFRasterScanlineSize(fuelsDataset);
    int n_bits = (scan_size / nXSize) * 8;

    // Use SAMPLEFORMAT to distinguish float32 from int32 in the 32-bit case.
    uint16_t sample_format = SAMPLEFORMAT_UINT;
    TIFFGetField(fuelsDataset, TIFFTAG_SAMPLEFORMAT, &sample_format);
    bool is_float32 = (n_bits == 32 && sample_format == SAMPLEFORMAT_IEEEFP);

    void* buf;
    if (n_bits == 64)
        buf = _TIFFmalloc(nXSize * sizeof(double));
    else if (n_bits == 32)
        buf = _TIFFmalloc(nXSize * sizeof(float));
    else
    {
        TIFFClose(fuelsDataset);
        throw std::runtime_error("Error: file type is not supported: '" + filename + "'");
    }
    if (!buf)
    {
        TIFFClose(fuelsDataset);
        throw std::runtime_error("Could not allocate memory");
    }

    int aux = 0;
    for (int i = 0; i < nYSize; i++)
    {
        if (TIFFReadScanline(fuelsDataset, buf, i) != 1)
        {
            _TIFFfree(buf);
            TIFFClose(fuelsDataset);
            throw std::runtime_error("Read error on row " + std::to_string(i));
        }
        for (int j = 0; j < nXSize; j++)
        {
            float pixelValue;
            if (n_bits == 64)
                pixelValue = static_cast<float>(((double*)buf)[j]);
            else if (is_float32)
                pixelValue = ((float*)buf)[j];
            else
                pixelValue = static_cast<float>(((int32_t*)buf)[j]);

            data[aux] = pixelValue;
            if (++aux == nCells)
            {
                _TIFFfree(buf);
                TIFFClose(fuelsDataset);
                return;
            }
        }
    }
    _TIFFfree(buf);
    TIFFClose(fuelsDataset);
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
 * @param InFolder Input data directory
 * @return An array of arrays representing a cell.
 */

// ---- File-scope lookup tables (shared by writeDataFileDirect & populateInputsDirect) ----

static const std::unordered_map<std::string, float> s_GFLD = {
    { "C1", 0.75f }, { "C2", 0.8f },  { "C3", 1.15f }, { "C4", 1.2f },  { "C5", 1.2f },
    { "C6", 1.2f },  { "C7", 1.2f },
    { "D1", std::numeric_limits<float>::quiet_NaN() },
    { "D2", std::numeric_limits<float>::quiet_NaN() },
    { "S1", std::numeric_limits<float>::quiet_NaN() },
    { "S2", std::numeric_limits<float>::quiet_NaN() },
    { "S3", std::numeric_limits<float>::quiet_NaN() },
    { "O1a", 0.35f }, { "O1b", 0.35f },
    { "M1", std::numeric_limits<float>::quiet_NaN() },
    { "M2", std::numeric_limits<float>::quiet_NaN() },
    { "M3", std::numeric_limits<float>::quiet_NaN() },
    { "M4", std::numeric_limits<float>::quiet_NaN() },
    { "NF", std::numeric_limits<float>::quiet_NaN() },
    { "M1_5", 0.1f },  { "M1_10", 0.2f }, { "M1_15", 0.3f }, { "M1_20", 0.4f },
    { "M1_25", 0.5f }, { "M1_30", 0.6f }, { "M1_35", 0.7f }, { "M1_40", 0.8f },
    { "M1_45", 0.8f }, { "M1_50", 0.8f }, { "M1_55", 0.8f }, { "M1_60", 0.8f },
    { "M1_65", 1.0f }, { "M1_70", 1.0f }, { "M1_75", 1.0f }, { "M1_80", 1.0f },
    { "M1_85", 1.0f }, { "M1_90", 1.0f }, { "M1_95", 1.0f }
};

static const std::unordered_map<std::string, int> s_PDFD = {
    { "M3_5", 5 }, { "M3_10", 10 }, { "M3_15", 15 }, { "M3_20", 20 }, { "M3_25", 25 },
    { "M3_30", 30 }, { "M3_35", 35 }, { "M3_40", 40 }, { "M3_45", 45 }, { "M3_50", 50 },
    { "M3_55", 55 }, { "M3_60", 60 }, { "M3_65", 65 }, { "M3_70", 70 }, { "M3_75", 75 },
    { "M3_80", 80 }, { "M3_85", 85 }, { "M3_90", 90 }, { "M3_95", 95 },
    { "M4_5", 5 }, { "M4_10", 10 }, { "M4_15", 15 }, { "M4_20", 20 }, { "M4_25", 25 },
    { "M4_30", 30 }, { "M4_35", 35 }, { "M4_40", 40 }, { "M4_45", 45 }, { "M4_50", 50 },
    { "M4_55", 55 }, { "M4_60", 60 }, { "M4_65", 65 }, { "M4_70", 70 }, { "M4_75", 75 },
    { "M4_80", 80 }, { "M4_85", 85 }, { "M4_90", 90 }, { "M4_95", 95 },
    { "M3M4_5", 5 }, { "M3M4_10", 10 }, { "M3M4_15", 15 }, { "M3M4_20", 20 }, { "M3M4_25", 25 },
    { "M3M4_30", 30 }, { "M3M4_35", 35 }, { "M3M4_40", 40 }, { "M3M4_45", 45 }, { "M3M4_50", 50 },
    { "M3M4_55", 55 }, { "M3M4_60", 60 }, { "M3M4_65", 65 }, { "M3M4_70", 70 }, { "M3M4_75", 75 },
    { "M3M4_80", 80 }, { "M3M4_85", 85 }, { "M3M4_90", 90 }, { "M3M4_95", 95 }
};

static const std::unordered_map<std::string, int> s_PCD = {
    { "M3_5", 5 }, { "M3_10", 10 }, { "M3_15", 15 }, { "M3_20", 20 }, { "M3_25", 25 },
    { "M3_30", 30 }, { "M3_35", 35 }, { "M3_40", 40 }, { "M3_45", 45 }, { "M3_50", 50 },
    { "M3_55", 55 }, { "M3_60", 60 }, { "M3_65", 65 }, { "M3_70", 70 }, { "M3_75", 75 },
    { "M3_80", 80 }, { "M3_85", 85 }, { "M3_90", 90 }, { "M3_95", 95 },
    { "M4_5", 5 }, { "M4_10", 10 }, { "M4_15", 15 }, { "M4_20", 20 }, { "M4_25", 25 },
    { "M4_30", 30 }, { "M4_35", 35 }, { "M4_40", 40 }, { "M4_45", 45 }, { "M4_50", 50 },
    { "M4_55", 55 }, { "M4_60", 60 }, { "M4_65", 65 }, { "M4_70", 70 }, { "M4_75", 75 },
    { "M4_80", 80 }, { "M4_85", 85 }, { "M4_90", 90 }, { "M4_95", 95 },
    { "M3M4_5", 5 }, { "M3M4_10", 10 }, { "M3M4_15", 15 }, { "M3M4_20", 20 }, { "M3M4_25", 25 },
    { "M3M4_30", 30 }, { "M3M4_35", 35 }, { "M3M4_40", 40 }, { "M3M4_45", 45 }, { "M3M4_50", 50 },
    { "M3M4_55", 55 }, { "M3M4_60", 60 }, { "M3M4_65", 65 }, { "M3M4_70", 70 }, { "M3M4_75", 75 },
    { "M3M4_80", 80 }, { "M3M4_85", 85 }, { "M3M4_90", 90 }, { "M3M4_95", 95 }
};

// ---- Module-level cache: filled by GenDataFile (TIF path), consumed by populateInputsDirect ----

struct InstanceCache
{
    std::vector<std::string> GFuelType;
    std::vector<int>         GFuelTypeN;
    std::vector<float>       Elevation;
    std::vector<float>       SAZ;
    std::vector<float>       PS;
    std::vector<float>       Curing;
    std::vector<float>       CBD;
    std::vector<float>       CBH;
    std::vector<float>       CCF;
    std::vector<float>       ProbMap;
    std::vector<float>       FMC;
    std::vector<float>       TreeHeight;
    bool valid = false;
};
static InstanceCache g_instanceCache;

// ---- End file-scope tables and cache ----

void
writeDataFileDirect(const std::vector<std::string>& GFuelType,
                    const std::vector<int>& GFuelTypeN,
                    const std::vector<float>& Elevation,
                    const std::vector<float>& PS,
                    const std::vector<float>& SAZ,
                    const std::vector<float>& Curing,
                    const std::vector<float>& CBD,
                    const std::vector<float>& CBH,
                    const std::vector<float>& CCF,
                    const std::vector<float>& ProbMap,
                    const std::vector<float>& FMC,
                    const std::vector<float>& TreeHeight,
                    const std::string& InFolder)
{

    const auto& GFLD = s_GFLD;
    const auto& PDFD = s_PDFD;
    const auto& PCD  = s_PCD;

    std::ofstream dataFile(InFolder + separator() + "Data.csv");
    if (!dataFile.is_open())
    {
        std::cerr << "Error: Unable to open data file for writing" << std::endl;
        return;
    }

    // Write header
    dataFile << "fueltype,lat,lon,elev,ws,waz,ps,saz,cur,cbd,cbh,ccf,ftypeN,fmc,probability,"
                "jd,jd_min,pc,pdf,time,ffmc,bui,gfl,pattern,tree_height,\n";

    // Write one row per cell directly to file — no intermediate storage
    for (size_t i = 0; i < GFuelType.size(); ++i)
    {
        // fueltype 0
        dataFile << GFuelType[i] << ',';
        // lat 1, lon 2 (hardcoded)
        dataFile << "51.621244,-115.608378,";
        // elev 3
        std::isnan(Elevation[i]) ? dataFile << ',' : dataFile << Elevation[i] << ',';
        // ws, waz 4,5 (blank)
        dataFile << ",,";
        // ps 6
        std::isnan(PS[i]) ? dataFile << ',' : dataFile << PS[i] << ',';
        // saz 7
        std::isnan(SAZ[i]) ? dataFile << ',' : dataFile << SAZ[i] << ',';
        // cur 8
        if (std::isnan(Curing[i]) && (GFuelType[i] == "O1a" || GFuelType[i] == "O1b"))
            dataFile << "60,";
        else
            dataFile << ',';
        // cbd 9
        std::isnan(CBD[i]) ? dataFile << ',' : dataFile << CBD[i] << ',';
        // cbh 10
        std::isnan(CBH[i]) ? dataFile << ',' : dataFile << CBH[i] << ',';
        // ccf 11
        std::isnan(CCF[i]) ? dataFile << ',' : dataFile << CCF[i] << ',';
        // ftypeN 12 (int, can't be NaN)
        dataFile << GFuelTypeN[i] << ',';
        // fmc 13
        std::isnan(FMC[i]) ? dataFile << ',' : dataFile << FMC[i] << ',';
        // probMap 14
        std::isnan(ProbMap[i]) ? dataFile << ',' : dataFile << ProbMap[i] << ',';
        // jd, jd_min 15,16 (blank)
        dataFile << ",,";
        // pc 17
        auto pcIt = PCD.find(GFuelType[i]);
        pcIt != PCD.end() ? dataFile << pcIt->second << ',' : dataFile << ',';
        // pdf 18
        auto pdfIt = PDFD.find(GFuelType[i]);
        pdfIt != PDFD.end() ? dataFile << pdfIt->second << ',' : dataFile << ',';
        // time 19
        dataFile << "20,";
        // ffmc, bui 20,21 (blank)
        dataFile << ",,";
        // gfl 22
        auto gflIt = GFLD.find(GFuelType[i]);
        gflIt != GFLD.end() ? dataFile << gflIt->second << ',' : dataFile << ',';
        // pattern 23 (blank)
        dataFile << ',';
        // tree_height 24
        std::isnan(TreeHeight[i]) ? dataFile << ',' : dataFile << TreeHeight[i] << ',';
        dataFile << '\n';
    }
}

// ---- Helpers used only within GenDataFile --------------------------------

struct RasterMeta
{
    int ncols = 0;
    int nrows = 0;
    double cellsize = 0.0;
};

// Read ncols, nrows, cellsize from an ASC header (first 5 key-value pairs).
static RasterMeta
readAscMeta(const std::string& filename)
{
    std::ifstream file(filename);
    if (!file.is_open())
        throw std::runtime_error("Cannot open raster file: '" + filename + "'");
    RasterMeta m{};
    std::string key, value;
    for (int i = 0; i < 5; ++i)
    {
        if (!(file >> key >> value))
            throw std::runtime_error("Malformed ASC header in '" + filename + "'");
        if (key == "ncols")
            m.ncols = std::stoi(value);
        else if (key == "nrows")
            m.nrows = std::stoi(value);
        else if (key == "cellsize")
            m.cellsize = std::stod(value);
    }
    return m;
}

// Read ncols, nrows, cellsize from a GeoTIFF.
static RasterMeta
readTifMeta(const std::string& filename)
{
    TIFF* tif = TIFFOpen(filename.c_str(), "r");
    if (!tif)
        throw std::runtime_error("Cannot open raster file: '" + filename + "'");
    RasterMeta m{};
    uint32_t w = 0, h = 0;
    TIFFGetField(tif, TIFFTAG_IMAGEWIDTH, &w);
    TIFFGetField(tif, TIFFTAG_IMAGELENGTH, &h);
    m.ncols = static_cast<int>(w);
    m.nrows = static_cast<int>(h);
    double* scale = nullptr;
    uint32_t count = 0;
    if (TIFFGetField(tif, 33550, &count, &scale) && scale)
        m.cellsize = scale[0];
    TIFFClose(tif);
    return m;
}

// Resolve a fuels path to a concrete file path (with extension).
// If the path already ends in .tif or .asc the file must exist.
// Otherwise .tif is probed first, then .asc.
// Throws std::runtime_error when no file can be found.
static std::string
resolveFuelsPath(const std::string& path)
{
    auto endsWith = [&](const std::string& s) {
        return path.size() >= s.size() &&
               path.compare(path.size() - s.size(), s.size(), s) == 0;
    };
    if (endsWith(".tif") || endsWith(".asc"))
    {
        if (!fileExists(path))
            throw std::runtime_error("Fuels file not found: '" + path + "'");
        return path;
    }
    if (fileExists(path + ".tif")) return path + ".tif";
    if (fileExists(path + ".asc")) return path + ".asc";
    throw std::runtime_error(
        "Fuels file not found: tried '" + path + ".tif' and '" + path + ".asc'");
}

// Throw a descriptive error if an auxiliary raster's grid dimensions or
// cellsize differ from the fuels raster.
static void
checkMetaMatch(int fuelsCols, int fuelsRows, double fuelsCellSize,
               const RasterMeta& aux,
               const std::string& fuelsFile, const std::string& auxFile)
{
    std::string mismatch;
    if (fuelsCols != aux.ncols)
        mismatch = "ncols " + std::to_string(fuelsCols) + " (fuels) vs " +
                   std::to_string(aux.ncols) + " (" + auxFile + ")";
    else if (fuelsRows != aux.nrows)
        mismatch = "nrows " + std::to_string(fuelsRows) + " (fuels) vs " +
                   std::to_string(aux.nrows) + " (" + auxFile + ")";
    else if (std::fabs(fuelsCellSize - aux.cellsize) > 1e-6 * fuelsCellSize)
        mismatch = "cellsize " + std::to_string(fuelsCellSize) + " (fuels) vs " +
                   std::to_string(aux.cellsize) + " (" + auxFile + ")";
    if (!mismatch.empty())
        throw std::runtime_error(
            "Raster metadata mismatch between '" + fuelsFile + "' and '" +
            auxFile + "': " + mismatch);
}

// Read all 11 bands from a multi-band instance GeoTIFF (produced by pack_instance.py).
// Band layout: 0=fuels, 1=elevation, 2=saz, 3=slope, 4=cur, 5=cbd, 6=cbh,
//              7=ccf, 8=probabilityMap, 9=fmc, 10=hm
static void
readInstanceTif(const std::string& tifPath,
                const std::unordered_map<std::string, std::string>& FBPDict,
                std::vector<int>& GFuelTypeN,
                std::vector<std::string>& GFuelType,
                int& NRows, int& NCols, float& cellSide,
                std::vector<float>& Elevation,
                std::vector<float>& SAZ,
                std::vector<float>& PS,
                std::vector<float>& Curing,
                std::vector<float>& CBD,
                std::vector<float>& CBH,
                std::vector<float>& CCF,
                std::vector<float>& ProbMap,
                std::vector<float>& FMC,
                std::vector<float>& TreeHeight)
{
    TIFF* tif = TIFFOpen(tifPath.c_str(), "r");
    if (!tif)
        throw std::runtime_error("Cannot open instance TIF: '" + tifPath + "'");

    uint32_t w = 0, h = 0;
    uint16_t nSamples = 1, planarConfig = PLANARCONFIG_CONTIG;
    uint16_t bitsPerSample = 32, sampleFormat = SAMPLEFORMAT_UINT;
    TIFFGetField(tif, TIFFTAG_IMAGEWIDTH, &w);
    TIFFGetField(tif, TIFFTAG_IMAGELENGTH, &h);
    TIFFGetField(tif, TIFFTAG_SAMPLESPERPIXEL, &nSamples);
    TIFFGetField(tif, TIFFTAG_PLANARCONFIG, &planarConfig);
    TIFFGetField(tif, TIFFTAG_BITSPERSAMPLE, &bitsPerSample);
    TIFFGetField(tif, TIFFTAG_SAMPLEFORMAT, &sampleFormat);

    if (planarConfig != PLANARCONFIG_SEPARATE)
    {
        TIFFClose(tif);
        throw std::runtime_error(
            "Instance TIF must use PLANARCONFIG_SEPARATE (interleave='band'): '" + tifPath + "'");
    }
    if (nSamples < 11)
    {
        TIFFClose(tif);
        throw std::runtime_error(
            "Instance TIF must have at least 11 bands, found " +
            std::to_string(nSamples) + ": '" + tifPath + "'");
    }

    NCols = static_cast<int>(w);
    NRows = static_cast<int>(h);
    int NCells = NCols * NRows;

    double* scale = nullptr;
    uint32_t count = 0;
    if (TIFFGetField(tif, 33550, &count, &scale) && scale)
        cellSide = static_cast<float>(scale[0]);
    else
        cellSide = 1.0f;

    bool is_float64 = (bitsPerSample == 64 && sampleFormat == SAMPLEFORMAT_IEEEFP);
    bool is_float32 = (bitsPerSample == 32 && sampleFormat == SAMPLEFORMAT_IEEEFP);

    // Nodata convention: pack_instance.py always writes -9999 for missing cells.
    // We do not read the GDAL_NODATA tag (42113) directly because libtiff does
    // not register it as a known tag, which leads to undefined behaviour.
    static constexpr float INSTANCE_NODATA = -9999.0f;
    bool has_nodata = true;
    float nodata_value = INSTANCE_NODATA;

    // For PLANARCONFIG_SEPARATE, TIFFScanlineSize gives ncols * bytes_per_sample.
    tsize_t scanlineSize = TIFFScanlineSize(tif);
    void* buf = _TIFFmalloc(scanlineSize);
    if (!buf)
    {
        TIFFClose(tif);
        throw std::runtime_error("Could not allocate scanline buffer");
    }

    GFuelType.resize(NCells);
    GFuelTypeN.resize(NCells);
    Elevation.assign(NCells, static_cast<float>(std::nanf("")));
    SAZ.assign(NCells, static_cast<float>(std::nanf("")));
    PS.assign(NCells, static_cast<float>(std::nanf("")));
    Curing.assign(NCells, static_cast<float>(std::nanf("")));
    CBD.assign(NCells, static_cast<float>(std::nanf("")));
    CBH.assign(NCells, static_cast<float>(std::nanf("")));
    CCF.assign(NCells, static_cast<float>(std::nanf("")));
    ProbMap.assign(NCells, static_cast<float>(std::nanf("")));
    FMC.assign(NCells, static_cast<float>(std::nanf("")));
    TreeHeight.assign(NCells, static_cast<float>(std::nanf("")));

    // Helper: read one float band (sample index) into a vector.
    // Nodata values are converted to NaN.
    auto readFloatBand = [&](uint16_t sample, std::vector<float>& data) {
        for (int row = 0; row < NRows; ++row)
        {
            if (TIFFReadScanline(tif, buf, row, sample) != 1)
            {
                _TIFFfree(buf);
                TIFFClose(tif);
                throw std::runtime_error(
                    "Read error on row " + std::to_string(row) +
                    " band " + std::to_string(sample) + " in '" + tifPath + "'");
            }
            for (int col = 0; col < NCols; ++col)
            {
                float val;
                if (is_float64)
                    val = static_cast<float>(((double*)buf)[col]);
                else if (is_float32)
                    val = ((float*)buf)[col];
                else
                    val = static_cast<float>(((int32_t*)buf)[col]);
                if (has_nodata && val == nodata_value)
                    val = std::numeric_limits<float>::quiet_NaN();
                data[row * NCols + col] = val;
            }
        }
    };

    // Band 0: fuels — integer lookup via FBPDict.
    for (int row = 0; row < NRows; ++row)
    {
        if (TIFFReadScanline(tif, buf, row, 0) != 1)
        {
            _TIFFfree(buf);
            TIFFClose(tif);
            throw std::runtime_error(
                "Read error on fuels band, row " + std::to_string(row) + " in '" + tifPath + "'");
        }
        for (int col = 0; col < NCols; ++col)
        {
            float pixVal;
            if (is_float64)
                pixVal = static_cast<float>(((double*)buf)[col]);
            else if (is_float32)
                pixVal = ((float*)buf)[col];
            else
                pixVal = static_cast<float>(((int32_t*)buf)[col]);

            int idx = row * NCols + col;
            bool isMissing = (pixVal != pixVal) || (has_nodata && pixVal == nodata_value);
            std::string token = isMissing ? "" : std::to_string(static_cast<int>(pixVal));
            if (isMissing || FBPDict.find(token) == FBPDict.end())
            {
                GFuelType[idx] = "NF";
                GFuelTypeN[idx] = 0;
            }
            else
            {
                GFuelType[idx] = FBPDict.at(token);
                GFuelTypeN[idx] = static_cast<int>(pixVal);
            }
        }
    }

    // Bands 1-10: float data layers.
    readFloatBand(1, Elevation);
    readFloatBand(2, SAZ);
    readFloatBand(3, PS);
    readFloatBand(4, Curing);
    readFloatBand(5, CBD);
    readFloatBand(6, CBH);
    readFloatBand(7, CCF);
    readFloatBand(8, ProbMap);
    readFloatBand(9, FMC);
    readFloatBand(10, TreeHeight);

    _TIFFfree(buf);
    TIFFClose(tif);
}

// ---- End helpers ---------------------------------------------------------

/**
 * @brief Reads all available input raster files and generates a new file called 'Data.csv' in which each row contains
 * the input data for a cell.
 *
 * If the fuels raster is in `tif` format, then it assumes all other rasters are also in that format.
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
 *
 * The generated file contains the following columns: fuel type, latitude, longitude, elevation, wind speed (always
 * blank), wind direction (always blank), slope, slope azimuth, curing level, canopy bulk density, canopy base height,
 * canopy cover fraction, fuel type number ,foliage moisture content, ignition probability.
 * @param InFolder Input data directory
 * @param fuelsPath Resolved path to the fuels raster (from --fuels-path, or InFolder/fuels by default)
 * @param instanceTif Path to a packed multi-band instance GeoTIFF (from --instance-tif). When non-empty,
 *                    all raster data is read from this single file and individual rasters are ignored.
 */

/**
 * @brief Populate the inputs[] array directly from the in-memory instance cache.
 *
 * Called instead of parseDFDirect when --instance-tif is used. Replicates the
 * exact field mapping produced by writeDataFileDirect + parseDFDirect, but
 * operates purely in memory — no Data.csv is written or read.
 */
void
populateInputsDirect(inputs* df_ptr, int nCells, std::vector<float>& ignProb, arguments* args_ptr)
{
    if (!g_instanceCache.valid)
        throw std::runtime_error("populateInputsDirect: instance cache is empty");

    const auto& GT  = g_instanceCache.GFuelType;
    const auto& GTN = g_instanceCache.GFuelTypeN;
    const auto& EL  = g_instanceCache.Elevation;
    const auto& SAZ = g_instanceCache.SAZ;
    const auto& PS  = g_instanceCache.PS;
    const auto& CUR = g_instanceCache.Curing;
    const auto& CBD = g_instanceCache.CBD;
    const auto& CBH = g_instanceCache.CBH;
    const auto& CCF = g_instanceCache.CCF;
    const auto& PM  = g_instanceCache.ProbMap;
    const auto& FMC = g_instanceCache.FMC;
    const auto& TH  = g_instanceCache.TreeHeight;

    for (int i = 0; i < nCells; ++i, ++df_ptr)
    {
        const std::string& ft = GT[i];

        // fueltype: char[5], same truncation as parseDFDirect
        std::string faux = ft + " ";
        std::strncpy(df_ptr->fueltype, faux.c_str(), 4);
        df_ptr->fueltype[4] = '\0';

        // hardcoded coords (match writeDataFileDirect)
        df_ptr->lat = 51.621244f;
        df_ptr->lon = -115.608378f;

        // terrain
        df_ptr->elev = std::isnan(EL[i])  ? 0.f : EL[i];
        df_ptr->ps   = std::isnan(PS[i])  ? 0.f : PS[i];
        df_ptr->saz  = std::isnan(SAZ[i]) ? 0.f : SAZ[i];

        // weather (always blank in Data.csv → defaults)
        df_ptr->ws  = 0.f;
        df_ptr->waz = 0;

        // curing — writeDataFileDirect only writes "60" for O1a/O1b with no data
        if (std::isnan(CUR[i]) && (ft == "O1a" || ft == "O1b"))
            df_ptr->cur = 60.f;
        else
            df_ptr->cur = 0.f;

        // canopy
        df_ptr->cbd = std::isnan(CBD[i]) ? -9999.f : CBD[i];
        df_ptr->cbh = std::isnan(CBH[i]) ? -9999.f : CBH[i];
        df_ptr->ccf = std::isnan(CCF[i]) ? 0.f     : CCF[i];

        // fuel type number
        df_ptr->nftype = GTN[i];

        // FMC (integer in inputs)
        df_ptr->FMC = std::isnan(FMC[i]) ? args_ptr->FMC : static_cast<int>(FMC[i]);

        // ignition probability — stored in ignProb, not in inputs
        ignProb[i] = std::isnan(PM[i]) ? 1.f : PM[i];

        // calendar / time (blank in Data.csv → 0 except time=20)
        df_ptr->jd     = 0;
        df_ptr->jd_min = 0;
        df_ptr->time   = 20;
        df_ptr->ffmc   = 0.f;
        df_ptr->bui    = 0.f;
        df_ptr->pattern = 0;

        // fuel-model lookups
        auto pcIt  = s_PCD.find(ft);
        df_ptr->pc  = (pcIt  != s_PCD.end())  ? pcIt->second  : 0;
        auto pdfIt = s_PDFD.find(ft);
        df_ptr->pdf = (pdfIt != s_PDFD.end()) ? pdfIt->second : 0;
        auto gflIt = s_GFLD.find(ft);
        df_ptr->gfl = (gflIt != s_GFLD.end()) ? gflIt->second : 0.f;

        // tree height
        df_ptr->tree_height = std::isnan(TH[i]) ? -9999.f : TH[i];
    }
}

void
GenDataFile(const std::string& InFolder, const std::string& fuelsPath, const std::string& instanceTif)
{
    std::cout << "\n------ Reading input data ------\n\n";
    std::unordered_map<std::string, std::string> FBPDict;
    std::unordered_map<std::string, std::tuple<float, float, float, float>> ColorsDict;

    std::string lookupTable = InFolder + separator() + "kitral_lookup_table.csv";

    if (!fileExists(lookupTable))
    {
        std::cerr << "Error: Lookup table '" << lookupTable << "' not found" << std::endl;
        return;
    }

    std::tie(FBPDict, ColorsDict) = Dictionary(lookupTable);

    std::vector<int> GFuelTypeN;
    std::vector<std::string> GFuelType;
    int FBPDicts = 0, Cols = 0;
    float CellSide = 1.0f;

    std::vector<float> Elevation;
    std::vector<float> SAZ;
    std::vector<float> PS;
    std::vector<float> Curing;
    std::vector<float> CBD;
    std::vector<float> CBH;
    std::vector<float> CCF;
    std::vector<float> ProbMap;
    std::vector<float> FMC;
    std::vector<float> TreeHeight;

    if (!instanceTif.empty())
    {
        // ---- Single multi-band instance TIF path -------------------------
        std::cout << "Reading instance TIF: " << instanceTif << '\n';
        readInstanceTif(instanceTif, FBPDict,
                        GFuelTypeN, GFuelType,
                        FBPDicts, Cols, CellSide,
                        Elevation, SAZ, PS, Curing,
                        CBD, CBH, CCF, ProbMap, FMC, TreeHeight);

        // If --fuels-path was explicitly provided (i.e. it differs from the
        // instance TIF itself), override the fuel types read from the TIF with
        // those from the separate fuels file.  This lets the caller apply a
        // modified fuel map (e.g. cells set to non-fuel) on top of the aux
        // rasters packed in the TIF.
        if (fuelsPath != instanceTif)
        {
            std::cout << "Overriding fuels from: " << fuelsPath << '\n';
            std::string FGrid = resolveFuelsPath(fuelsPath);
            bool isTif = FGrid.size() >= 4 && FGrid.substr(FGrid.size() - 4) == ".tif";
            int tmpFBPDicts = 0, tmpCols = 0;
            float tmpCellSide = 1.0f;
            if (isTif)
                std::tie(GFuelTypeN, GFuelType, tmpFBPDicts, tmpCols, tmpCellSide) =
                    ForestGridTif(FGrid, FBPDict);
            else
                std::tie(GFuelTypeN, GFuelType, tmpFBPDicts, tmpCols, tmpCellSide) =
                    ForestGrid(FGrid, FBPDict);
        }

        // Store in module-level cache so populateInputsDirect can fill
        // inputs[] directly without writing/reading Data.csv.
        g_instanceCache.GFuelType  = GFuelType;
        g_instanceCache.GFuelTypeN = GFuelTypeN;
        g_instanceCache.Elevation  = Elevation;
        g_instanceCache.SAZ        = SAZ;
        g_instanceCache.PS         = PS;
        g_instanceCache.Curing     = Curing;
        g_instanceCache.CBD        = CBD;
        g_instanceCache.CBH        = CBH;
        g_instanceCache.CCF        = CCF;
        g_instanceCache.ProbMap    = ProbMap;
        g_instanceCache.FMC        = FMC;
        g_instanceCache.TreeHeight = TreeHeight;
        g_instanceCache.valid      = true;

        std::cout << "Instance data cached (" << GFuelType.size() << " cells)\n";
        return;  // skip Data.csv write entirely
    }
    else
    {
        // ---- Individual raster files path --------------------------------

        // Resolve fuels file path; throws if the file cannot be found.
        std::string FGrid = resolveFuelsPath(fuelsPath);

        std::string extension = (FGrid.size() >= 4 && FGrid.substr(FGrid.size() - 4) == ".tif")
                                 ? ".tif" : ".asc";
        std::cout << "Using " << extension << '\n';

        if (extension == ".tif")
            std::tie(GFuelTypeN, GFuelType, FBPDicts, Cols, CellSide) = ForestGridTif(FGrid, FBPDict);
        else
            std::tie(GFuelTypeN, GFuelType, FBPDicts, Cols, CellSide) = ForestGrid(FGrid, FBPDict);

        int NCells = static_cast<int>(GFuelType.size());

        Elevation.assign(NCells, static_cast<float>(std::nanf("")));
        SAZ.assign(NCells, static_cast<float>(std::nanf("")));
        PS.assign(NCells, static_cast<float>(std::nanf("")));
        Curing.assign(NCells, static_cast<float>(std::nanf("")));
        CBD.assign(NCells, static_cast<float>(std::nanf("")));
        CBH.assign(NCells, static_cast<float>(std::nanf("")));
        CCF.assign(NCells, static_cast<float>(std::nanf("")));
        ProbMap.assign(NCells, static_cast<float>(std::nanf("")));
        FMC.assign(NCells, static_cast<float>(std::nanf("")));
        TreeHeight.assign(NCells, static_cast<float>(std::nanf("")));

        std::vector<std::string> basenames
            = { "elevation", "saz", "slope", "cur", "cbd", "cbh", "ccf", "probabilityMap", "fmc", "hm" };

        for (const auto& base : basenames)
        {
            std::string filePath;
            std::string auxExt;

            std::string tryAsc = InFolder + separator() + base + ".asc";
            std::string tryTif = InFolder + separator() + base + ".tif";

            if (fileExists(tryAsc))
            {
                filePath = tryAsc;
                auxExt = ".asc";
            }
            else if (fileExists(tryTif))
            {
                filePath = tryTif;
                auxExt = ".tif";
            }
            else
            {
                std::cout << "No " << base << " file, filling with NaN" << std::endl;
                continue;
            }

            RasterMeta auxMeta = (auxExt == ".tif") ? readTifMeta(filePath) : readAscMeta(filePath);
            checkMetaMatch(Cols, FBPDicts, static_cast<double>(CellSide), auxMeta, FGrid, filePath);

            auto readGrid = [&](std::vector<float>& data) {
                if (auxExt == ".asc")
                    DataGrids(filePath, data, NCells);
                else
                    DataGridsTif(filePath, data, NCells);
            };

            if      (base == "elevation")      readGrid(Elevation);
            else if (base == "saz")            readGrid(SAZ);
            else if (base == "slope")          readGrid(PS);
            else if (base == "cur")            readGrid(Curing);
            else if (base == "cbd")            readGrid(CBD);
            else if (base == "cbh")            readGrid(CBH);
            else if (base == "ccf")            readGrid(CCF);
            else if (base == "probabilityMap") readGrid(ProbMap);
            else if (base == "fmc")            readGrid(FMC);
            else if (base == "hm")             readGrid(TreeHeight);
        }
    }

    writeDataFileDirect(GFuelType, GFuelTypeN, Elevation, PS, SAZ, Curing, CBD, CBH, CCF, ProbMap, FMC, TreeHeight, InFolder);
    std::cout << "Generated data file" << std::endl;
}
