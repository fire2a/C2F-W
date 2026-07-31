/* coding: utf-8
__version__ = "3.0"
__author__ = "Jaime Carrasco-Barra"
__maintainer__ = "Jaime Carrasco-Barra, Matilde Rivas, David Palacios"
*/
#ifndef FUELMODELSPAIN
#define FUELMODELSPAIN
#include "Cells.h"
#include "FuelModelUtils.h"
#include "ReadArgs.h"
#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>

/*
 *  Input, Fire, and output structures
 */

/*
               Functions
*/
// Initialize coefficients

// Calculate flank fire
float flankfire_ros_s(float ros, float bros, float lb);

// Calculate rate of spread

// Length-to-Breadth ratio
float l_to_b(float ws);

// BROS
float backfire_ros_s(main_outs* at, snd_outs* sec);

// Flame length [m])

// byram intensity
float byram_intensity(inputs* data, fuel_coefs* ptr);

// Type of fire (Crown = 1, SROS = 0)
bool fire_type(inputs* data, fuel_coefs* ptr);

// CROS adjustements
float rate_of_spread10(inputs* data, arguments* args, float ws);

bool checkActive(inputs* data, main_outs* at);
float crownfractionburn(inputs* data, main_outs* at);
float final_rate_of_spread10(inputs* data);
// Back fire with CROS
float backfire_ros10_s(fire_struc* hptr, snd_outs* sec);

// Slope effect
float slope_effect(inputs* data);

// Main function to populate spread outputs based on inputs provided from main
// class
void calculate_s(inputs* data,
                 fuel_coefs* ptr,
                 arguments* args,
                 main_outs* at,
                 snd_outs* sec,
                 fire_struc* hptr,
                 fire_struc* fptr,
                 fire_struc* bptr,
                 bool& activeCrown,
                 weatherDF* wdf_ptr);

void determine_destiny_metrics_s(inputs* data, fuel_coefs* ptr, arguments* args, main_outs* at, weatherDF* wdf_ptr);

// Todos los codigos de combustible en sbTable (para lookup identidad opcional)
std::vector<int> sbAllFuelCodes();

#endif
