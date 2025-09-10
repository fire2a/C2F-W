#include "FuelModelUtils.h"
#include "Cells.h"
#include "ReadArgs.h"
#include <cmath>
#include <iostream>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <unordered_map>
#include <vector>

// Angle of the flame w.r.t. horizontal surface (Putnam's)
float
angleFL(const float ws, main_outs* at)
{
    float angle, fl, y;
    fl = at->fl;
    y = 10.0 / 36.0 * ws;

    angle = atan(2.24 * sqrt(fl / pow(y, 2)));
    return angle;
}

// Transformation from FL to FH using angle
float
flame_height(main_outs* at)
{
    float fh, phi;
    phi = at->angle;
    fh = at->fl * sin(phi);
    return fh;
}