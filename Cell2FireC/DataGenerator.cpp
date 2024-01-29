#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>
#include <array>
#include <algorithm>

std::unordered_map<std::string, std::string> row;
std::unordered_map<std::string, std::tuple<float, float, float, float>> colors;

// Reads fbp_lookup_table.csv and creates dictionaries for the fuel types and cells' colors
void Dictionary(const std::string& filename) {
    int aux = 1;
    std::ifstream file(filename);
    std::string line;

    // Read file and save colors and ftypes dictionaries
    while (std::getline(file, line)) {
        if (aux > 1) {
            aux += 1;

            // Split the line into tokens
            std::istringstream ss(line);
            std::vector<std::string> tokens;
            std::string token;
            while (std::getline(ss, token, ',')) {
                tokens.push_back(token);
            }

            if (tokens[3].substr(0, 3) == "FM1") {
                row[tokens[0]] = tokens[3].substr(0, 4);
            } else if (tokens[3].substr(0, 3) == "Non" || tokens[3].substr(0, 3) == "NFn") {
                row[tokens[0]] = "NF";
            } else {
                row[tokens[0]] = tokens[3].substr(0, 3);
            }

            colors[tokens[0]] = std::make_tuple(
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
    int trows = 0;
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

int main(int argc, char* argv[]) {
    // Check if a command-line argument is provided
    if (argc != 3) {
        std::cerr << "Usage: " << argv[0] << " <fbp_lookup_table_path>" << std::endl;
        return 1;
    }

    // Example usage with command-line argument
    try {

        Dictionary(argv[1]);
        auto result = ForestGrid(argv[2], row);

        std::cerr << "Rows -------------------------------------------------------------- \n";

        for (const auto& entry : row) {
            std::cout << "row[" << entry.first << "] = " << entry.second << std::endl;
        }

        // Access the elements of the result tuple
        std::vector<int> gridcell3 = std::get<0>(result);
        std::vector<std::string> gridcell4 = std::get<1>(result);
        int lenGrid = std::get<2>(result);
        int tCols = std::get<3>(result);
        float cellSize = std::get<4>(result);

        std::cerr << "\n gridcell3:\n ";
        for (const auto& value : gridcell3) {
            std::cerr << value << " ";
        }

        std::cerr << "\n gridcell4:\n ";
        for (const auto& value : gridcell4) {
            std::cerr << value << " ";
        }

        std::cerr << "\n lenGrid: " << lenGrid << "\n";
        std::cerr << " tCols: " << tCols << "\n";
        std::cerr << " cellSize: " << cellSize << "\n";

        // Now you can use gridcell3, gridcell4, lenGrid, tCols, and cellSize as needed
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}