#ifndef DATAGENERATOR_H
#define DATAGENERATOR_H

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

std::tuple<std::unordered_map<std::string, std::string>,
           std::unordered_map<std::string, std::tuple<float, float, float, float>>>
Dictionary(const std::string& filename);

std::tuple<std::vector<int>, std::vector<std::string>, int, int, float>
ForestGrid(const std::string& filename, const std::unordered_map<std::string, std::string>& Dictionary);

bool fileExists(const std::string& filename);

void DataGrids(const std::string& filename, std::vector<float>& data, int nCells);

std::vector<std::vector<std::unique_ptr<std::string>>> GenerateDat(const std::vector<std::string>& GFuelType,
                                                                   const std::vector<int>& GFuelTypeN,
                                                                   const std::vector<float>& Elevation,
                                                                   const std::vector<float>& PS,
                                                                   const std::vector<float>& SAZ,
                                                                   const std::vector<float>& Curing,
                                                                   const std::vector<float>& CBD,
                                                                   const std::vector<float>& CBH,
                                                                   const std::vector<float>& CCF,
                                                                   const std::vector<float>& PY,
                                                                   const std::vector<float>& FMC,
                                                                   const std::vector<float>& TreeHeight,

                                                                   const std::string& InFolder);

void writeDataToFile(const std::vector<std::vector<std::unique_ptr<std::string>>>& dataGrids,
                     const std::string& InFolder);

void GenDataFile(const std::string& InFolder, const std::string& Simulator);

#endif  // DataGenerator.h
