//
// Created by mati on 13-08-25.
//

#ifndef C2F_W_FUELMODELUTILS_H
#define C2F_W_FUELMODELUTILS_H

#include "Cells.h"
#include <cmath>
#include <iostream>
#include <unordered_map>
#include <vector>

// Angle of the flame w.r.t. horizontal surface (Putnam's)
float angleFL(const float ws, main_outs* at);

// Transformation from FL to FH using angle
float flame_height(main_outs* at);
#endif  // C2F_W_FUELMODELUTILS_H