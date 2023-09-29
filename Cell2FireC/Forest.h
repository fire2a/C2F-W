#ifndef FOREST
#define FOREST

// Include libraries
#include <stdio.h>
#include <string>
#include <vector>
#include <math.h>
#include <cmath>
#include <iostream>
#include <unordered_map>
#include <unordered_set>
#include <string>

using namespace std;

// Class definition
class Forest
{
public:
    int id;
    int nCells;
    float area;
    double vol;
    double age;
    float perimeter;
    std::string location;
    std::vector<int> coord;
    std::unordered_map<std::string, int> fTypes;
    std::vector<int> availCells;
    std::vector<int> burntCells;

    // Constructor & methods
    Forest(int _id, std::string _location, std::vector<int> _coord, int _nCells, float _area,
           double _vol, double _age, float _perimeter, std::unordered_map<std::string, int> &_fTypes);

    void print_info();
};

#endif
