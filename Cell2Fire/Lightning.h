#ifndef LIGHTNING
#define LIGHTNING

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

class Lightning
{
  public:
    // Constructor & Methods
    Lightning();
    int Lambda_Simple_Test(int period);
    bool Lambda_NH(int period, bool verbose);
    bool Lambda_H(int period, bool verbose);
};

#endif
