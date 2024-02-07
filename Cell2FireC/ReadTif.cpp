#include "DataGenerator.h"
#include <gdal/gdal.h>
#include <gdal/gdal_priv.h>
#include <gdal/cpl_conv.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <cmath>
#include <unordered_map>
#include <array>
#include <algorithm>
#include <memory>


// Reads fbp_lookup_table.csv and creates dictionaries for the fuel types and cells' ColorsDict
std::tuple<std::unordered_map<std::string, std::string>, std::unordered_map<std::string, std::tuple<float, float, float, float>>> Dictionary(const std::string& filename) {

    std::unordered_map<std::string, std::string> FBPDict;
    std::unordered_map<std::string, std::tuple<float, float, float, float>> ColorsDict;

    int aux = 1;
    std::ifstream file(filename);
    std::string line;

    // Read file and save ColorsDict and ftypes dictionaries
    while (std::getline(file, line)) {
        if (aux > 1) {
            aux += 1;

            // Replace hyphen
            size_t hyphenPos = line.find('-');
            while (hyphenPos != std::string::npos) {
                line.replace(hyphenPos, 1, "");
                hyphenPos = line.find('-');
            }

            // Replace newline
            size_t newlinePos = line.find('\n');
            while (newlinePos != std::string::npos) {
                line.replace(newlinePos, 1, "");
                newlinePos = line.find('\n');
            }

            // Replace "No" with "NF"
            size_t noPos = line.find("No");
            while (noPos != std::string::npos) {
                line.replace(noPos, 2, "NF");
                noPos = line.find("No");
            }

            // Split the line into tokens
            std::istringstream ss(line);
            std::vector<std::string> tokens;
            std::string token;
            while (std::getline(ss, token, ',')) {
                tokens.push_back(token);
            }

            if (tokens[3].substr(0, 3) == "FM1") {
                FBPDict[tokens[0]] = tokens[3].substr(0, 4);
            } else if (tokens[3].substr(0, 3) == "Non" || tokens[3].substr(0, 3) == "NFn") {
                FBPDict[tokens[0]] = "NF";
            } else {
                FBPDict[tokens[0]] = tokens[3].substr(0, 3);
            }

            ColorsDict[tokens[0]] = std::make_tuple(
                std::stof(tokens[4]) / 255.0f,
                std::stof(tokens[5]) / 255.0f,
                std::stof(tokens[6]) / 255.0f,
                1.0f
            );
        }

        if (aux == 1) {
            aux += 1;
        }

    }

    return std::make_tuple(FBPDict, ColorsDict);
}


std::tuple<std::vector<int>, std::vector<std::string>, int, int, float>
ForestGridTif(const std::string& filename, const std::unordered_map<std::string, std::string>& Dictionary) {
    /*
    Reads fuel data from a .tif
    Args:
       filename (std::string): Name of .tif file.
       Dictionary (std::unordered_map<std::string, std::string>&): Reference to fuels dictionary

    Returns:
        Fuel vectors, number of cells y cell size (tuple[std::vector<int>, std::vector<std::string>)
    */
    // Tries to open file
    GDALAllRegister();
    GDALDataset *fuelsDataset;
    GDALRasterBand *fuelsBand;
    fuelsDataset = (GDALDataset *) GDALOpen(filename.c_str(), GA_ReadOnly);
    // If not succesfull, throws exception
    if (fuelsDataset == NULL) {
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
    // Get fuel band
    fuelsBand = fuelsDataset->GetRasterBand(1);
    // Get Raster dimentions
    int nXSize = fuelsBand->GetXSize();
    int nYSize = fuelsBand->GetYSize();
    // Extracts geotransform
    double adfGeoTransform[6];
    fuelsDataset->GetGeoTransform(adfGeoTransform);
    // Gets cell size from geotransform
    double cellSizeX = adfGeoTransform[1];
    double cellSizeY = adfGeoTransform[5];
    const double epsilon = std::numeric_limits<double>::epsilon();
    std::cout << fabs(cellSizeX + cellSizeY) << '-' << epsilon << '\n';
    if (fabs(cellSizeX + cellSizeY) > epsilon) {
        throw std::runtime_error("Error: Cells are not square in: '" + filename + "'");
    }
    // Read raster data
    float *pafScanline;
    pafScanline = (float *) CPLMalloc(sizeof(float) * nXSize * nYSize);
    // For each row
    for (int i = 0; i < nYSize; i++) {
        // Read pixel values for the current row
        CPLErr errcode = fuelsBand->RasterIO(GF_Read, 0, i, nXSize, 1, pafScanline, nXSize, 1, GDT_Float32, 0, 0);
        if (errcode != 0) {
            throw std::runtime_error("Error: Raster reading failed in: '" + filename + "'");
        }
        // For each column
        for (int j = 0; j < nXSize; j++) {
            // Access the pixel value at position (i, j)
            float pixelValue = pafScanline[j];
            std::string token = std::to_string(static_cast<int>(pafScanline[j]));
            if (pixelValue != pixelValue || Dictionary.find(token) == Dictionary.end()) {
                    // If fuel not in Dictionary:
                    gridcell1.push_back("NF");
                    gridcell2.push_back("NF");
                    gridcell3.push_back(0);
                    gridcell4.push_back("NF");
            } else {
                    gridcell1.push_back(token);
                    gridcell2.push_back(Dictionary.at(token));
                    gridcell3.push_back(std::stoi(token));
                    gridcell4.push_back(Dictionary.at(token));
            }
            tcols = std::max(tcols, static_cast<int>(gridcell1.size()));
        }  
    }
    std::vector<std::array<int, 2>> CoordCells;
    CoordCells.reserve(grid.size() * tcols);
    int n = 1;
    tcols += 1;
    return std::make_tuple(gridcell3, gridcell4, grid.size(), tcols - 1, cellSizeX);
}

// Function to read grid data from ASCII file
void DataGrids(const std::string& filename, std::vector<float>& data, int nCells) {
    /*
    Reads fuel data from a .tif
    Args:
       filename (std::string): Name of .tif file.
       Dictionary (std::unordered_map<std::string, std::string>&): Reference to fuels dictionary

    Returns:
        Fuel vectors, number of cells y cell size (tuple[std::vector<int>, std::vector<std::string>)
    */
    // Tries to open file
    GDALAllRegister();
    GDALDataset *fuelsDataset;
    GDALRasterBand *fuelsBand;
    fuelsDataset = (GDALDataset *) GDALOpen(filename.c_str(), GA_ReadOnly);
    // If not succesfull, throws exception
    if (fuelsDataset == NULL) {
        throw std::runtime_error("Error: Could not open file '" + filename + "'");
    }

    std::vector<std::string> filelines;

    
    float cellsize;
    // Extracts geotransform
    double adfGeoTransform[6];
    fuelsDataset->GetGeoTransform(adfGeoTransform);
    // Gets cell size from geotransform
    double cellSizeX = adfGeoTransform[1];
    double cellSizeY = adfGeoTransform[5];
    const double epsilon = std::numeric_limits<double>::epsilon();
    if (fabs(cellSizeX + cellSizeY) > epsilon) {
        throw std::runtime_error("Error: Cells are not square in: '" + filename + "'");
    }
    cellsize = static_cast<float>(cellSizeX);
    // Get fuel band
    fuelsBand = fuelsDataset->GetRasterBand(1);
    // Get Raster dimentions
    int nXSize = fuelsBand->GetXSize();
    int nYSize = fuelsBand->GetYSize();
    int aux = 0;
    // Read raster data
    float *pafScanline;
    pafScanline = (float *) CPLMalloc(sizeof(float) * nXSize * nYSize);
    // For each row
    for (int i = 0; i < nYSize; i++) {
        // Read pixel values for the current row
        CPLErr errcode = fuelsBand->RasterIO(GF_Read, 0, i, nXSize, 1, pafScanline, nXSize, 1, GDT_Float32, 0, 0);
        if (errcode != 0) {
            throw std::runtime_error("Error: Raster reading failed in: '" + filename + "'");
        }
        // For each column
        for (int j = 0; j < nXSize; j++) {
            // Access the pixel value at position (i, j)
            float pixelValue = pafScanline[j];
            if (pixelValue == pixelValue){
                std::string token = std::to_string(static_cast<int>(pafScanline[j]));
                std::cout << token << '\n';
                data[aux] = std::stof(token);
            }
            aux++;
            if (aux == nCells) {
                return;  // Stop reading if we've filled the data vector
            }
            }
    }
}


int main() {
    std::unordered_map<std::string, std::string> FBPDict;
    std::unordered_map<std::string, std::tuple<float, float, float, float>> ColorsDict;
    // Call Dictionary function to read lookup table
    std::tie(FBPDict, ColorsDict) = Dictionary("../data/CanadianFBP/dogrib/fbp_lookup_table.csv");
    // Call ForestGrid function
    std::string FGrid = "example.tif";
    std::vector<int> GFuelTypeN;
    std::vector<std::string> GFuelType;
    int FBPDicts, Cols;
    float CellSide;
    std::tie(GFuelTypeN, GFuelType, FBPDicts, Cols, CellSide) = ForestGridTif(FGrid, FBPDict);
    std::cout << CellSide;
    int NCells = GFuelType.size();
    std::vector<float> Elevation(NCells, std::nanf(""));
    DataGrids(FGrid, Elevation, NCells);
    return 0;
}
