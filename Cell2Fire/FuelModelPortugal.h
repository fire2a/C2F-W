#ifndef FUELMODELPORTUGAL
#define FUELMODELPORTUGAL
#include "Cells.h"
#include "ReadArgs.h"
#include <iostream>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
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
void initialize_coeff_p(int scenario);

// Calculate flank fire
float flankfire_ros_p(float ros, float bros, float lb);

// Calculate rate of spread
float rate_of_spread_p(inputs* data, fuel_coefs* ptr, main_outs* at, float ws);

// Length-to-Breadth ratio
float l_to_b_p(float ws);

// BROS
float backfire_ros_p(main_outs* at, snd_outs* sec);

// Flame length [m])
float flame_length_p(inputs* data, fuel_coefs* ptr, float ws);

// byram intensity
float byram_intensity_p(inputs* data, fuel_coefs* ptr);

// Type of fire (Crown = 1, SROS = 0)
bool fire_type_p(inputs* data, fuel_coefs* ptr);

// CROS adjustements
float rate_of_spread10_p(inputs* data, arguments* args, float ws);

bool checkActive_p(inputs* data, main_outs* at);
float crownfractionburn_p(inputs* data, main_outs* at);
float final_rate_of_spread10_p(inputs* data);
// Back fire with CROS
float backfire_ros10_p(fire_struc* hptr, snd_outs* sec);

// Slope effect
float slope_effect_p(inputs* data);

// Main function to populate spread outputs based on inputs provided from main class
void calculate_p(inputs* data,
                 fuel_coefs* ptr,
                 arguments* args,
                 main_outs* at,
                 snd_outs* sec,
                 fire_struc* hptr,
                 fire_struc* fptr,
                 fire_struc* bptr,
                 bool& activeCrown,
                 weatherDF* wdf_ptr);

void determine_destiny_metrics_p(inputs* data, fuel_coefs* ptr, arguments* args, main_outs* at, weatherDF* wdf_ptr);

#endif
