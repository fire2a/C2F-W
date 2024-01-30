#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <cmath>
#include <unordered_map>
#include <array>
#include <algorithm>
#include <any>



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

// ForestGrid function
std::tuple<std::vector<int>, std::vector<std::string>, int, int, float>
ForestGrid(const std::string& filename, const std::unordered_map<std::string, std::string>& Dictionary) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        throw std::runtime_error("Error: Could not open file '" + filename + "'");
    }

    std::vector<std::string> filelines;

    // Read all lines from the file
    std::string line;
    while (std::getline(file, line)) {
        filelines.push_back(line);
    }

    // Extract cellsize from line 5
    line = filelines[4];
    line.erase(std::remove(line.begin(), line.end(), '\n'), line.end());
    std::istringstream iss(line);
    std::string key, value;

    if (!(iss >> key >> value) || key != "cellsize") {
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
    for (size_t i = 6; i < filelines.size(); ++i) {
    line = filelines[i];

    // Remove newline characters
    line.erase(std::remove(line.begin(), line.end(), '\n'), line.end());

    // Remove leading and trailing whitespaces
    line.erase(line.begin(), std::find_if(line.begin(), line.end(), [](char c) { return !std::isspace(c); }));
    line.erase(std::find_if(line.rbegin(), line.rend(), [](char c) { return !std::isspace(c); }).base(), line.end());

    std::istringstream iss(line);
    std::string token;

    while (iss >> token) {
        if (Dictionary.find(token) == Dictionary.end()) {
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

// Function to check if a file exists
bool fileExists(const std::string& filename) {
    std::ifstream file(filename);
    return file.good();
}

// Function to read grid data from ASCII file
void DataGrids(const std::string& filename, std::vector<float>& data, int nCells) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Error: Could not open file '" << filename << "'" << std::endl;
        return;
    }

    std::vector<std::string> filelines;

    // Read all lines from the file
    std::string line;
    while (std::getline(file, line)) {
        filelines.push_back(line);
    }

    // Extract cellsize from line 5
    line = filelines[4];
    line.erase(std::remove(line.begin(), line.end(), '\n'), line.end());
    std::istringstream iss(line);
    std::string key, value;

    if (!(iss >> key >> value) || key != "cellsize") {
        std::cerr << "Error: Expected 'cellsize' on line 5 of " << filename << std::endl;
        return;
    }

    float cellsize = std::stof(value);

    int aux = 0;

    // Read the ASCII file with the grid structure
    for (size_t i = 6; i < filelines.size(); ++i) {
        line = filelines[i];
        line.erase(std::remove(line.begin(), line.end(), '\n'), line.end());
        line = ' ' + line;
        std::istringstream iss(line);
        std::string token;

        while (iss >> token) {
            data[aux++] = std::stof(token);
            if (aux == nCells) {
                return;  // Stop reading if we've filled the data vector
            }
        }
    }

    
}

std::vector<std::vector<std::any>> GenerateDat(const std::vector<std::string>& GFuelType, const std::vector<int>& GFuelTypeN,
                 const std::vector<float>& Elevation, const std::vector<float>& PS,
                 const std::vector<float>& SAZ, const std::vector<float>& Curing,
                 const std::vector<float>& CBD, const std::vector<float>& CBH,
                 const std::vector<float>& CCF, const std::vector<float>& PY,
                 const std::vector<float>& FMC, const std::string& InFolder) {
    // DF columns
    std::vector<std::string> Columns = {"fueltype", "lat", "lon", "elev", "ws", "waz", "ps", "saz", "cur", "cbd", "cbh", "ccf", "ftypeN", "fmc", "py",
                                        "jd", "jd_min", "pc", "pdf", "time", "ffmc", "bui", "gfl", "pattern"};

    // GFL dictionary (FBP)
    std::unordered_map<std::string, float> GFLD = {
        {"C1", 0.75}, {"C2", 0.8}, {"C3", 1.15}, {"C4", 1.2}, {"C5", 1.2}, {"C6", 1.2}, {"C7", 1.2},
        {"D1", std::nanf("")}, {"D2", std::nanf("")},
        {"S1", std::nanf("")}, {"S2", std::nanf("")}, {"S3", std::nanf("")},
        {"O1a", 0.35}, {"O1b", 0.35},
        {"M1", std::nanf("")}, {"M2", std::nanf("")}, {"M3", std::nanf("")}, {"M4", std::nanf("")}, {"NF", std::nanf("")},
        {"M1_5", 0.1}, {"M1_10", 0.2}, {"M1_15", 0.3}, {"M1_20", 0.4}, {"M1_25", 0.5}, {"M1_30", 0.6},
        {"M1_35", 0.7}, {"M1_40", 0.8}, {"M1_45", 0.8}, {"M1_50", 0.8}, {"M1_55", 0.8}, {"M1_60", 0.8},
        {"M1_65", 1.0}, {"M1_70", 1.0}, {"M1_75", 1.0}, {"M1_80", 1.0}, {"M1_85", 1.0}, {"M1_90", 1.0}, {"M1_95", 1.0}};

    // PDF dictionary (CANADA)
    std::unordered_map<std::string, int> PDFD = {
        {"M3_5", 5}, {"M3_10", 10}, {"M3_15", 15}, {"M3_20", 20}, {"M3_25", 25}, {"M3_30", 30}, {"M3_35", 35}, {"M3_40", 40}, {"M3_45", 45}, {"M3_50", 50},
        {"M3_55", 55}, {"M3_60", 60}, {"M3_65", 65}, {"M3_70", 70}, {"M3_75", 75}, {"M3_80", 80}, {"M3_85", 85}, {"M3_90", 90}, {"M3_95", 95}, {"M4_5", 5},
        {"M4_10", 10}, {"M4_15", 15}, {"M4_20", 20}, {"M4_25", 25}, {"M4_30", 30}, {"M4_35", 35}, {"M4_40", 40}, {"M4_45", 45}, {"M4_50", 50}, {"M4_55", 55},
        {"M4_60", 60}, {"M4_65", 65}, {"M4_70", 70}, {"M4_75", 75}, {"M4_80", 80}, {"M4_85", 85}, {"M4_90", 90}, {"M4_95", 95}, {"M3M4_5", 5}, {"M3M4_10", 10},
        {"M3M4_15", 15}, {"M3M4_20", 20}, {"M3M4_25", 25}, {"M3M4_30", 30}, {"M3M4_35", 35}, {"M3M4_40", 40}, {"M3M4_45", 45}, {"M3M4_50", 50}, {"M3M4_55", 55},
        {"M3M4_60", 60}, {"M3M4_65", 65}, {"M3M4_70", 70}, {"M3M4_75", 75}, {"M3M4_80", 80}, {"M3M4_85", 85}, {"M3M4_90", 90}, {"M3M4_95", 95}};

    // PCD dictionary (CANADA)
    std::unordered_map<std::string, int> PCD = {
        {"M1_5", 5}, {"M1_10", 10}, {"M1_15", 15}, {"M1_20", 20}, {"M1_25", 25}, {"M1_30", 30}, {"M1_35", 35}, {"M1_40", 40}, {"M1_45", 45},
        {"M1_50", 50}, {"M1_55", 55}, {"M1_60", 60}, {"M1_65", 65}, {"M1_70", 70}, {"M1_75", 75}, {"M1_80", 80}, {"M1_85", 85}, {"M1_90", 90},
        {"M1_95", 95}, {"M2_5", 5}, {"M2_10", 10}, {"M2_15", 15}, {"M2_20", 20}, {"M2_25", 25}, {"M2_30", 30}, {"M2_35", 35}, {"M2_40", 40},
        {"M2_45", 45}, {"M2_50", 50}, {"M2_55", 55}, {"M2_60", 60}, {"M2_65", 65}, {"M2_70", 70}, {"M2_75", 75}, {"M2_80", 80}, {"M2_85", 85},
        {"M2_90", 90}, {"M2_95", 95}, {"M1M2_5", 5}, {"M1M2_10", 10}, {"M1M2_15", 15}, {"M1M2_20", 20}, {"M1M2_25", 25}, {"M1M2_30", 30},
        {"M1M2_35", 35}, {"M1M2_40", 40}, {"M1M2_45", 45}, {"M1M2_50", 50}, {"M1M2_55", 55}, {"M1M2_60", 60}, {"M1M2_65", 65}, {"M1M2_70", 70},
        {"M1M2_75", 75}, {"M1M2_80", 80}, {"M1M2_85", 85}, {"M1M2_90", 90}, {"M1M2_95", 95}};

    // Dataframe
    std::vector<std::vector<std::any>> DF(GFuelType.size(), std::vector<std::any>(Columns.size()));

    // Populate DF
    for (size_t i = 0; i < GFuelType.size(); ++i) {
        DF[i][0] = GFuelType[i];  // "fueltype"
        DF[i][1] = 51.621244f;      // "lat (use f to indicate floats)"
        DF[i][2] = -115.608378f;    // "lon (use f to indicate floats)"
        DF[i][3] = Elevation[i];  // "elev"
        DF[i][6] = PS[i];          // "ps"
        DF[i][7] = SAZ[i];         // "saz"
        DF[i][9] = CBD[i];         // "cbd"
        DF[i][10] = CBH[i];        // "cbh"
        DF[i][11] = CCF[i];        // "ccf"
        DF[i][12] = PY[i];         // "py"
        DF[i][13] = FMC[i];        // "fmc"
        DF[i][17] = 20;            // "time"
        DF[i][21] = std::nanf(""); // "pattern"
        // Populate fuel type number
        DF[i][14] = GFuelTypeN[i];  // "ftypeN"

        // Handle special cases
        if (std::isnan(Curing[i])) {
            DF[i][8] = 60.0;  // "cur"
        }

        // Populate GFL
        if (GFLD.find(GFuelType[i]) != GFLD.end()) {
            DF[i][22] = GFLD[GFuelType[i]];  // "gfl"
        }

        // Populate PDF
        if (PDFD.find(GFuelType[i]) != PDFD.end()) {
            DF[i][16] = PDFD[GFuelType[i]];  // "pdf"
        }

        // Populate PC
        if (PCD.find(GFuelType[i]) != PCD.end()) {
            DF[i][15] = PCD[GFuelType[i]];  // "pc"
        }

        // Replace "nan" with an empty string
        if (DF[i][8].has_value() && DF[i][8].type() == typeid(float) && std::isnan(std::any_cast<float>(DF[i][8]))) {
            DF[i][8] = std::string("");  // "cur"
        }
    }

    /*
    // Print DF for debugging
    std::cout << "\nDataFrame (DF) for Debugging:\n";
    for (size_t i = 0; i < 5; ++i) {
        for (size_t j = 0; j < 5; ++j) {
            std::cout << Columns[j] << ": ";
            if (DF[i][j].has_value()) {
                if (DF[i][j].type() == typeid(float)) {
                    std::cout << std::any_cast<float>(DF[i][j]);
                } else if (DF[i][j].type() == typeid(int)) {
                    std::cout << std::any_cast<int>(DF[i][j]);
                } else if (DF[i][j].type() == typeid(std::string)) {
                    std::cout << std::any_cast<std::string>(DF[i][j]);
                }
            }
            std::cout << " | ";
        }
        std::cout << "\n";
    }
    */

    // /*
    // Data File
    std::ofstream dataFile(InFolder + "/Data.csv");
    if (dataFile.is_open()) {
        // Write header
        for (const auto& col : Columns) {
            dataFile << col << ",";
        }
        dataFile << "\n";

        // Write data
        for (const auto& FBPDict : DF) {
            for (const auto& item : FBPDict) {
                if (item.has_value()) {
                    if (item.type() == typeid(float)) {
                        dataFile << std::any_cast<float>(item);
                    } else if (item.type() == typeid(int)) {
                        dataFile << std::any_cast<int>(item);
                    } else if (item.type() == typeid(std::string)) {
                        dataFile << std::any_cast<std::string>(item);
                    }
                }
                dataFile << ",";
            }
            dataFile << "\n";
        }

        std::cout << "Data file generated successfully: " << InFolder << "/Data.csv" << std::endl;
        dataFile.close();
    } else {
        std::cerr << "Error: Unable to open data file for writing" << std::endl;
    }
    // */

    return DF;
}

// Main function
void GenDataFile(const std::string& InFolder, const std::string& Simulator) {
    std::unordered_map<std::string, std::string> FBPDict;
    std::unordered_map<std::string, std::tuple<float, float, float, float>> ColorsDict;

    // Determine the lookup table based on the Simulator
    std::string FBPlookup;
    if (Simulator == "K") {
        FBPlookup = InFolder + "/kitral_lookup_table.csv";
    } else if (Simulator == "S") {
        FBPlookup = InFolder + "/spain_lookup_table.csv";
    } else { // beta version
        FBPlookup = InFolder + "/fbp_lookup_table.csv";
    }

    // Call Dictionary function to read lookup table
    std::tie(FBPDict, ColorsDict) = Dictionary(FBPlookup);

    // Call ForestGrid function
    std::string FGrid = InFolder + "/fuels.asc";
    std::vector<int> GFuelTypeN;
    std::vector<std::string> GFuelType;
    int FBPDicts, Cols;
    float CellSide;
    std::tie(GFuelTypeN, GFuelType, FBPDicts, Cols, CellSide) = ForestGrid(FGrid, FBPDict);

    // RESULTS PRINTING FOR DEBUGING ----------------------------------------------------------
    /*
    
    // Print FBPDict
    std::cout << "FBPDict:\n";
    for (const auto& entry : FBPDict) {
        std::cout << "  " << entry.first << ": " << entry.second << std::endl;
    }

    // Print ColorsDict
    std::cout << "ColorsDict:\n";
    for (const auto& entry : ColorsDict) {
        std::cout << "  " << entry.first << ": " << std::get<0>(entry.second) << ", "
                  << std::get<1>(entry.second) << ", " << std::get<2>(entry.second) << ", "
                  << std::get<3>(entry.second) << std::endl;
    }

    // Print ForestGrid outputs
    std::cout << "\nForestGrid Outputs:\n";
    std::cout << "GFuelTypeN: ";
    for (const auto& value : GFuelTypeN) {
        std::cout << value << " ";
    }
    std::cout << "\nGFuelType: ";
    for (const auto& value : GFuelType) {
        std::cout << value << " ";
    }

    std::cout << "\n" << "FBPDicts:" << FBPDicts << "\n";
    std::cout << "\n" << "Cols:" << Cols << "\n";
    std::cout << "\n" << "CellSide:" << CellSide << "\n";
    
    */
    // RESULTS PRINTING FOR DEBUGING ENDS HERE-----------------------------------------------------

    int NCells = GFuelType.size();

    // Call DataGrids function (formerly DataGrids)
    std::vector<float> Elevation(NCells, std::nanf(""));
    std::vector<float> SAZ(NCells, std::nanf(""));
    std::vector<float> PS(NCells, std::nanf(""));
    std::vector<float> Curing(NCells, std::nanf(""));
    std::vector<float> CBD(NCells, std::nanf(""));
    std::vector<float> CBH(NCells, std::nanf(""));
    std::vector<float> CCF(NCells, std::nanf(""));
    std::vector<float> PY(NCells, std::nanf(""));
    std::vector<float> FMC(NCells, std::nanf(""));

    std::vector<std::string> filenames = {
        "elevation.asc", "saz.asc", "slope.asc", "cur.asc",
        "cbd.asc", "cbh.asc", "ccf.asc", "py.asc", "fmc.asc"
    };

    for (const auto& name : filenames) {
        std::string filePath = InFolder + "/" + name;
        if (fileExists(filePath)) {
            if (name == "slope.asc") {
                // Assuming "slope.asc" corresponds to PS (Pitch or Slope)
                DataGrids(filePath, PS, NCells);
            } else {
                // Read other grid data
                DataGrids(filePath, Elevation, NCells);
                DataGrids(filePath, SAZ, NCells);
                DataGrids(filePath, Curing, NCells);
                DataGrids(filePath, CBD, NCells);
                DataGrids(filePath, CBH, NCells);
                DataGrids(filePath, CCF, NCells);
                DataGrids(filePath, PY, NCells);
                DataGrids(filePath, FMC, NCells);
            }
        } else {
            //std::cout << "No " << name << " file, filling with NaN" << std::endl;
        }
    }

    // Call GenerateDat function
    std::vector<std::vector<std::any>> result = GenerateDat(GFuelType, GFuelTypeN, Elevation, PS, SAZ, Curing, CBD, CBH, CCF, PY, FMC, InFolder);

    /*
    // Access the result for debugging purposes
    for (const auto& FBPDict : result) {
        for (const auto& item : FBPDict) {
            if (item.has_value()) {
                if (item.type() == typeid(float)) {
                    std::cout << std::any_cast<float>(item) << " ";
                } else if (item.type() == typeid(int)) {
                    std::cout << std::any_cast<int>(item) << " ";
                } else if (item.type() == typeid(std::string)) {
                    std::cout << std::any_cast<std::string>(item) << " ";
                }
            } else {
                std::cout << "NaN ";
            }
        }
        std::cout << std::endl;
    }
    */
}

/*
int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <InFolder>" << std::endl;
        return 1;
    }

    std::string InFolder = argv[1];
    std::string Simulator = "C"; // Change to "S" or other values based on your needs

    GenDataFile(InFolder, Simulator);

    return 0;
}
*/