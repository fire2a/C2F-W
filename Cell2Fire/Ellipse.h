#ifndef ELLIPSE
#define ELLIPSE

// include stuff
#include <Eigen/Dense>
#include <math.h>
#include <stdio.h>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

using namespace Eigen;
using namespace std;

class Ellipse
{
  public:
    // mutable
    double _a;
    double _b;
    ArrayXXd _Coef;

    // constructor and methods here
    Ellipse(std::vector<double> _x, std::vector<double> _y);
    std::vector<double> get_parameters();
};

#endif
