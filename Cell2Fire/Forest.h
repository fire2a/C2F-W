#ifndef FOREST
#define FOREST

// Include libraries
#include <cmath>
#include <iostream>
#include <math.h>
#include <stdio.h>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

using namespace std;

// Class definition
class Forest
{
  public:
    int id;
    int nCells;
    double area;
    double vol;
    double age;
    double perimeter;
    std::string location;
    std::vector<int> coord;
    std::unordered_map<std::string, int> fTypes;
    std::vector<int> availCells;
    std::vector<int> burntCells;

    // Constructor & methods
    Forest(int _id,
           std::string _location,
           std::vector<int> _coord,
           int _nCells,
           double _area,
           double _vol,
           double _age,
           double _perimeter,
           std::unordered_map<std::string, int>& _fTypes);

    void print_info();
};

#endif
