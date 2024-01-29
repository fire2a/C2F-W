#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

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

            // Remove unwanted characters
            size_t pos;
            while ((pos = line.find('-')) != std::string::npos) line.erase(pos, 1);
            while ((pos = line.find('\n')) != std::string::npos) line.erase(pos, 1);
            while ((pos = line.find("No")) != std::string::npos) line.replace(pos, 2, "NF");

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


int main() {
    // Example usage
    Dictionary("fbp_lookup_table.csv");

    // Print the contents of dictionaries for comparison
    for (const auto& entry : row) {
        std::cout << "row[" << entry.first << "] = " << entry.second << std::endl;
    }

    for (const auto& entry : colors) {
        std::cout << "colors[" << entry.first << "] = "
                  << "(" << std::get<0>(entry.second) << ", "
                  << std::get<1>(entry.second) << ", "
                  << std::get<2>(entry.second) << ", "
                  << std::get<3>(entry.second) << ")" << std::endl;
    }

    return 0;
}
