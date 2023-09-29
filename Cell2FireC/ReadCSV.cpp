#include "ReadCSV.h"
#include "Cells.h"
#include "ReadArgs.h"

#include <gdal/gdal_priv.h>
#include <iostream>
#include <fstream>
#include <vector>
#include <tuple>
#include <iterator>
#include <string>
#include <algorithm>
#include <unordered_map>
#include <unordered_set>
#include <boost/algorithm/string.hpp>
#include <boost/variant.hpp>
#include <errno.h>

/*
Read Tiff base function
*/

void getTifData(std::string df_member, inputs *df_ptr, const char *pszFilename, int number_of_arguments, int raster_band = 1)
{
	GDALDatasetUniquePtr poDataset;
	GDALAllRegister();
	const GDALAccess eAccess = GA_ReadOnly;
	poDataset = GDALDatasetUniquePtr(GDALDataset::FromHandle(GDALOpen(pszFilename, eAccess)));
	if (!poDataset)
	{
		printf("error\n"); // handle error
	}
	GDALRasterBand *poBand;
	poBand = poDataset->GetRasterBand(raster_band);
	int nXSize = poBand->GetXSize();
	int nYSize = poBand->GetYSize();
	if (number_of_arguments != 27)
	{
		std::cout << "nars: " << number_of_arguments << std::endl;
		printf("WARNING, change number of arguments in READCSV.cpp (getTifdataFunction) and Cells.h(inputs struct definition), ReadTifData function \n");
	}
	if (df_member == "fuelType")
	{
		CPLErr Err = poBand->RasterIO(GF_Read, 0, 0, nXSize, nYSize,
									  &df_ptr->nftype, number_of_arguments * nXSize, nYSize, GDT_Int32, // care with the GDT_Int32
									  0, 0);
	}
	else if (df_member == "elevation")
	{
		CPLErr Err = poBand->RasterIO(GF_Read, 0, 0, nXSize, nYSize,
									  &df_ptr->elev, number_of_arguments * nXSize, nYSize, GDT_Float32, // care with the GDT_Int32
									  0, 0);
	}

	else if (df_member == "windSpeed")
	{
		CPLErr Err = poBand->RasterIO(GF_Read, 0, 0, nXSize, nYSize,
									  &df_ptr->ws, number_of_arguments * nXSize, nYSize, GDT_Float32, // care with the GDT_Int32
									  0, 0);
	}

	else if (df_member == "windDirection")
	{
		CPLErr Err = poBand->RasterIO(GF_Read, 0, 0, nXSize, nYSize,
									  &df_ptr->waz, number_of_arguments * nXSize, nYSize, GDT_Int32, // care with the GDT_Int32
									  0, 0);
	}
	else if (df_member == "slope")
	{
		CPLErr Err = poBand->RasterIO(GF_Read, 0, 0, nXSize, nYSize,
									  &df_ptr->ps, number_of_arguments * nXSize, nYSize, GDT_Float32, // care with the GDT_Int32
									  0, 0);
	}

	else if (df_member == "aspect")
	{
		CPLErr Err = poBand->RasterIO(GF_Read, 0, 0, nXSize, nYSize,
									  &df_ptr->saz, number_of_arguments * nXSize, nYSize, GDT_Float32, // care with the GDT_Int32
									  0, 0);
	}

	else if (df_member == "curing")
	{
		CPLErr Err = poBand->RasterIO(GF_Read, 0, 0, nXSize, nYSize,
									  &df_ptr->cur, number_of_arguments * nXSize, nYSize, GDT_Float32, // care with the GDT_Int32
									  0, 0);
	}

	else if (df_member == "cbd")
	{
		CPLErr Err = poBand->RasterIO(GF_Read, 0, 0, nXSize, nYSize,
									  &df_ptr->cbd, number_of_arguments * nXSize, nYSize, GDT_Float32, // care with the GDT_Int32
									  0, 0);
	}
	else if (df_member == "cbh")
	{
		CPLErr Err = poBand->RasterIO(GF_Read, 0, 0, nXSize, nYSize,
									  &df_ptr->cbh, number_of_arguments * nXSize, nYSize, GDT_Float32, // care with the GDT_Int32
									  0, 0);
	}
	else if (df_member == "ccf")
	{
		CPLErr Err = poBand->RasterIO(GF_Read, 0, 0, nXSize, nYSize,
									  &df_ptr->ccf, number_of_arguments * nXSize, nYSize, GDT_Float32, // care with the GDT_Int32
									  0, 0);
	}
	else if (df_member == "fmc")
	{
		CPLErr Err = poBand->RasterIO(GF_Read, 0, 0, nXSize, nYSize,
									  &df_ptr->FMC, number_of_arguments * nXSize, nYSize, GDT_Int32, // care with the GDT_Int32
									  0, 0);
	}

	else if (df_member == "jd")
	{
		CPLErr Err = poBand->RasterIO(GF_Read, 0, 0, nXSize, nYSize,
									  &df_ptr->jd, number_of_arguments * nXSize, nYSize, GDT_Int32, // care with the GDT_Int32
									  0, 0);
	}
	else if (df_member == "jd_min")
	{
		CPLErr Err = poBand->RasterIO(GF_Read, 0, 0, nXSize, nYSize,
									  &df_ptr->jd_min, number_of_arguments * nXSize, nYSize, GDT_Int32, // care with the GDT_Int32
									  0, 0);
	}
	else if (df_member == "bui")
	{
		CPLErr Err = poBand->RasterIO(GF_Read, 0, 0, nXSize, nYSize,
									  &df_ptr->bui, number_of_arguments * nXSize, nYSize, GDT_Float32, // care with the GDT_Int32
									  0, 0);
	}
	else if (df_member == "ffmc")
	{
		CPLErr Err = poBand->RasterIO(GF_Read, 0, 0, nXSize, nYSize,
									  &df_ptr->ffmc, number_of_arguments * nXSize, nYSize, GDT_Float32, // care with the GDT_Int32
									  0, 0);
	}
	else if (df_member == "pattern")
	{
		CPLErr Err = poBand->RasterIO(GF_Read, 0, 0, nXSize, nYSize,
									  &df_ptr->pattern, number_of_arguments * nXSize, nYSize, GDT_Int32, // care with the GDT_Int32
									  0, 0);
	}
}

/*
 * Constructur
 */
CSVReader::CSVReader(std::string filename, std::string delm)
{
	this->fileName = filename;
	this->delimeter = delm;
}

void CSVReader::readLookUpTable(inputs *df_ptr, int nCells)
{
	// Abre el archivo CSV en modo lectura.
	ifstream archivo(this->fileName);
	std::string line = "";
	std::unordered_map<int, std::string> fueltype_map;
	std::string::size_type sz; // alias of size_t

	int i = 0;
	while (getline(archivo, line))
	{
		if (i > 0)
		{
			// Crea un lector CSV.
			std::vector<std::string> vec;
			boost::algorithm::split(vec, line, boost::is_any_of(this->delimeter));
			int fueltype = std::stoi(vec[0], &sz);
			std::string fuelname = vec[3];
			fuelname = fuelname.substr(0, 4);
			fuelname.erase(
				std::remove_if(fuelname.begin(), fuelname.end(), [](char chr)
							   { return chr == '-' || chr == '/'; }),
				fuelname.end());
			if (fuelname == "Non")
			{
				fuelname = "NF";
			}
			fueltype_map.insert(std::make_pair(fueltype, fuelname));
		}
		i++;
	}

	for (i = 1; i <= nCells; i++)
	{
		int nfueltype = df_ptr->nftype;
		if (fueltype_map.find(nfueltype) == fueltype_map.end()) // if nftype not in fuel model
		{
			std::string fueltype_string = "NF"; // assign as non fuel
			const char *fueltype_char = fueltype_string.c_str();
			strncpy(df_ptr->fueltype, fueltype_char, 4);
		}
		else // if nftype in fuel model
		{
			std::string fueltype_string = fueltype_map[nfueltype];
			const char *fueltype_char = fueltype_string.c_str();
			strncpy(df_ptr->fueltype, fueltype_char, 4);
		}
		df_ptr++;
	}

	// strncpy(df_ptr->fueltype, faux, 4)
}

/*
 * Parses through csv file line by line and returns the data
 * in vector of vector of strings.
 */
std::vector<std::vector<std::string>> CSVReader::getData()
{
	std::ifstream file(this->fileName);
	std::vector<std::vector<std::string>> dataList;
	std::string line = "";
	// Iterate through each line and split the content using delimeter
	if (this->fileName.substr(this->fileName.find_last_of(".") + 1) == "asc")
	{
		int header = 0;
		while (getline(file, line))
		{
			if (header < 5)
			{
				std::vector<std::string> vec;
				int start = 0;
				int end = 0;
				while ((start = line.find_first_not_of(this->delimeter, end)) != std::string::npos)
				{
					end = line.find(this->delimeter, start);
					vec.push_back(line.substr(start, end - start));
				}
				dataList.push_back(vec);
				header++;
			}
			else
			{
				std::vector<std::string> vec;
				boost::algorithm::split(vec, line, boost::is_any_of(this->delimeter));
				dataList.push_back(vec);
			}
		}
	}

	else
	{
		while (getline(file, line))
		{
			std::vector<std::string> vec;
			boost::algorithm::split(vec, line, boost::is_any_of(this->delimeter));
			dataList.push_back(vec);
		}
	}

	// Close the File
	file.close();

	return dataList;
}

/*
 * Parses through csv file line by line and returns the data
 * in vector of vector of strings.
 */
std::tuple<const char *, const char *, int, int, float, float, float> CSVReader::readTifMetadata(arguments *args_ptr)
{
	// const char *c = this->fileName.c_str();
	const char *pszFilename = this->fileName.c_str();
	GDALDatasetUniquePtr poDataset;
	GDALAllRegister();
	const GDALAccess eAccess = GA_ReadOnly;
	poDataset = GDALDatasetUniquePtr(GDALDataset::FromHandle(GDALOpen(pszFilename, eAccess)));
	if (!poDataset)
	{
		printf("error\n"); // handle error
	}

	double adfGeoTransform[6];
	const char *description, *metadata, *projection, *data_type_name, *color_intepretation;
	int ncols, nrows, nlayers, nBlockXSize, nBlockYSize, bGotMin, bGotMax;
	float origin_x, origin_y, cellsize;
	double adfMinMax[2];
	description = poDataset->GetDriver()->GetDescription();
	metadata = poDataset->GetDriver()->GetMetadataItem(GDAL_DMD_LONGNAME);
	projection = poDataset->GetProjectionRef();
	nrows = poDataset->GetRasterXSize();
	ncols = poDataset->GetRasterYSize();
	nlayers = poDataset->GetRasterCount();
	origin_x = adfGeoTransform[0];
	origin_y = adfGeoTransform[3];
	cellsize = adfGeoTransform[1];
	GDALRasterBand *poBand;
	poBand = poDataset->GetRasterBand(1);
	poBand->GetBlockSize(&nBlockXSize, &nBlockYSize);
	data_type_name = GDALGetDataTypeName(poBand->GetRasterDataType());
	color_intepretation = GDALGetColorInterpretationName(poBand->GetColorInterpretation());
	adfMinMax[0] = poBand->GetMinimum(&bGotMin);
	adfMinMax[1] = poBand->GetMaximum(&bGotMax);
	if (!(bGotMin && bGotMax))
		GDALComputeRasterMinMax((GDALRasterBandH)poBand, TRUE, adfMinMax);

	if (args_ptr->verbose)
	{
		std::cout << "description raster: " << description << std::endl;
		std::cout << "metadata raster: " << metadata << std::endl;
		std::cout << "projection raster: " << projection << std::endl;
		std::cout << "nrows: " << nrows << std::endl;
		std::cout << "ncols: " << ncols << std::endl;
		std::cout << "nlayers: " << nlayers << std::endl;
		std::cout << "origin_x: " << origin_x << std::endl;
		std::cout << "origin_y: " << origin_y << std::endl;
		std::cout << "pixel resolution: " << cellsize << std::endl;
		std::cout << "data type of layer: " << data_type_name << std::endl;
		std::cout << "color interpretation of layer: " << color_intepretation << std::endl;
		std::cout << "X block size: " << nBlockXSize << std::endl;
		std::cout << "Y block size: " << nBlockYSize << std::endl;
		std::cout << "minimum value of layer: " << adfMinMax[0] << std::endl;
		std::cout << "maximum value of layer: " << adfMinMax[1] << std::endl;
	}

	return std::make_tuple(metadata, projection, nrows, ncols, origin_x, origin_y, cellsize);
}

void CSVReader::readTifArgs(inputs *df_ptr, arguments *args_ptr, int number_of_members, int raster_band = 1)
{
	std::unordered_map<std::string, std::string> inputs_map = {{args_ptr->landscapeFilename,
																"fuelType"},
															   {args_ptr->elevationFilename, "elevation"},
															   {args_ptr->windSpeedFilename, "windSpeed"},
															   {args_ptr->windDirectionFilename, "windDirection"},
															   {args_ptr->slopeFilename, "slope"},
															   {args_ptr->aspectFilename,
																"aspect"},
															   {args_ptr->curingFilename, "curing"},
															   {args_ptr->cbdFilename, "cbd"},
															   {args_ptr->cbhFilename,
																"cbh"},
															   {args_ptr->ccfFilename, "ccf"},
															   {args_ptr->FMCFilename, "fmc"},
															   {args_ptr->jdFilename,
																"jd"},
															   {args_ptr->jdMinFilename, "jd_min"},
															   {args_ptr->buiFilename, "bui"},
															   {args_ptr->ffmcFilename,
																"ffmc"},
															   {args_ptr->patternFilename, "pattern"}};
	for (auto it = inputs_map.begin(); it != inputs_map.end(); it++)
	{
		if (it->first != "")
		{
			std::cout << "Filling " << it->first << " data" << std::endl;
			const char *arg_layer = it->first.c_str();
			getTifData(it->second, df_ptr, arg_layer, number_of_members, 1);
		}
	}
}

void CSVReader::readTifFile(inputs *df_ptr, const char *file, int raster_band = 1)
{
	std::cout << "deprecated" << std::endl;
	// getTifData(df_ptr, file, raster_band);
}

/*
 * Prints data to screen inside the DF obtained from the CSV file
 */
void CSVReader::printData(std::vector<std::vector<std::string>> &DF)
{
	// Print the content of row by row on screen
	for (std::vector<std::string> vec : DF)
	{
		for (std::string data : vec)
		{
			if (data.length() > 0)
				std::cout << " " << data << " ";
			else
				std::cout << " "
						  << "NaN"
						  << " ";
		}
		std::cout << std::endl;
	}
}

void CSVReader::printDataTif(inputs *df, int Ncells)
{

	for (int i = 0; i < Ncells; i++)
	{
		std::cout << "cell: " << i << " code of fuel: " << df[i].nftype << std::endl;
	}
}

/*
 * Populates the df input objects based on the DF csv file for each row/cell (spanish version)
 */
void CSVReader::parseDF(inputs *df_ptr, std::vector<std::vector<std::string>> &DF, arguments *args_ptr, int NCells)
{
	int i;

	// Floats
	float cur, elev, ws, waz, saz, cbd, cbh, ccf, ps, lat, lon, ffmc, bui, gfl;

	// Integers
	int nftype, FMC, jd, jd_min, pc, pdf, time, pattern;

	// CChar
	const char *faux;
	std::string::size_type sz; // alias of size_t

	// Loop over cells (populating per row)
	for (i = 1; i <= NCells; i++)
	{
		// printf("Populating DF for cell %d\n", i);

		faux = DF[i][0].append(" ").c_str();

		if (DF[i][3].compare("") == 0)
			elev = 0;
		else
			elev = std::stof(DF[i][3], &sz);

		if (DF[i][4].compare("") == 0)
			ws = 0;
		else
			ws = std::stof(DF[i][4], &sz);

		if (DF[i][5].compare("") == 0)
			waz = 0;
		else
			waz = std::stoi(DF[i][5], &sz) + 180.; // + 2*90;  // CHECK!!!!
		if (waz >= 360)
			waz = waz - 360;

		if (DF[i][6].compare("") == 0)
			ps = 0;
		else
			ps = std::stof(DF[i][6], &sz);

		if (DF[i][7].compare("") == 0)
			saz = 0;
		else
			saz = std::stof(DF[i][7], &sz);

		if (DF[i][8].compare("") == 0)
			cur = 0; // if no curing given,  DF["cur"][DF.fueltype == "O1a"] = 60.0 DF["cur"][DF.fueltype == "O1b"] = 60.0
		else
			cur = std::stof(DF[i][8], &sz);

		if (DF[i][9].compare("") == 0)
			cbd = 0;
		else
			cbd = std::stof(DF[i][9], &sz);

		if (DF[i][10].compare("") == 0)
			cbh = 0;
		else
			cbh = std::stof(DF[i][10], &sz);

		if (DF[i][11].compare("") == 0)
			ccf = 0;
		else
			ccf = std::stof(DF[i][11], &sz);

		if (DF[i][12].compare("") == 0)
			nftype = 0;
		else
			nftype = std::stoi(DF[i][12], &sz);

		if (DF[i][13].compare("") == 0)
			FMC = args_ptr->FMC;
		else
			FMC = std::stoi(DF[i][13], &sz);

		// DF[i][14] stores probability

		if (DF[i][15].compare("") == 0)
			jd = 0;
		else
			jd = std::stoi(DF[i][15], &sz);

		if (DF[i][16].compare("") == 0)
			jd_min = 0;
		else
			jd_min = std::stoi(DF[i][16], &sz);

		if (DF[i][17].compare("") == 0)
			pc = 0;
		else
			pc = std::stoi(DF[i][17], &sz);

		if (DF[i][18].compare("") == 0)
			pdf = 0;
		else
			pdf = std::stoi(DF[i][18], &sz);

		if (DF[i][19].compare("") == 0)
			time = 0;
		else
			time = std::stoi(DF[i][19], &sz);

		if (DF[i][1].compare("") == 0)
			lat = 0;
		else
			lat = std::stof(DF[i][1], &sz);

		if (DF[i][2].compare("") == 0)
			lon = 0;
		else
			lon = std::stof(DF[i][2], &sz);

		if (DF[i][20].compare("") == 0)
			ffmc = 0;
		else
			ffmc = std::stof(DF[i][20], &sz);

		if (DF[i][21].compare("") == 0)
			bui = 0;
		else
			bui = std::stof(DF[i][21], &sz);

		if (DF[i][22].compare("") == 0)
			gfl = 0;
		else
			gfl = std::stof(DF[i][22], &sz);

		if (DF[i][23].compare("") == 0)
			pattern = 0;
		else
			pattern = 1; // std::stoi (DF[i][18], &sz);

		// Set values
		strncpy(df_ptr->fueltype, faux, 4);
		df_ptr->elev = elev;
		df_ptr->ws = ws;
		df_ptr->waz = waz;
		df_ptr->ps = ps;
		df_ptr->saz = saz;
		df_ptr->cur = cur;
		df_ptr->cbd = cbd;
		df_ptr->cbh = cbh;
		df_ptr->ccf = ccf;
		df_ptr->nftype = nftype;
		df_ptr->FMC = FMC;
		df_ptr->jd = jd;
		df_ptr->jd_min = jd_min;
		df_ptr->pc = pc;
		df_ptr->pdf = pdf;
		df_ptr->time = time;
		df_ptr->lat = lat;
		df_ptr->lon = lon;
		df_ptr->ffmc = ffmc;
		df_ptr->bui = bui;
		df_ptr->gfl = gfl;
		df_ptr->pattern = pattern;

		// Next pointer
		df_ptr++;
	}
}

/*
 * Populates vector of size NCells with type number based on lookup table (Spain version)
 */
void CSVReader::parseNDF(std::vector<int> &NFTypes, std::vector<std::vector<std::string>> &DF, int NCells)
{
	int i;

	// Ints
	int FType;

	// CChar
	const char *faux;
	std::string::size_type sz; // alias of size_t

	// Loop over cells (populating per row)
	for (i = 1; i <= NCells; i++)
	{
		// printf("Populating DF for cell %d\n", i);
		if (DF[i][12].compare("") == 0)
			FType = 0;
		else
			FType = std::stoi(DF[i][12], &sz);

		// Set values
		NFTypes.push_back(FType);
	}
}

/*
 * Populates vector of size NCells with type number based on lookup table (Spain version)
 */
void CSVReader::parsePROB(std::vector<float> &probabilities, std::vector<std::vector<std::string>> &DF, int NCells)
{
	int i;

	// Ints
	float Prob;

	// CChar
	const char *faux;
	std::string::size_type sz; // alias of size_t

	// Loop over cells (populating per row)
	for (i = 1; i <= NCells; i++)
	{
		// printf("Populating DF for cell %d\n", i);
		if (DF[i][14].compare("") == 0)
			Prob = 1;
		else
			Prob = std::stof(DF[i][14], &sz);

		// Set values
		probabilities[i - 1] = Prob;
		// probabilities.push_back(Prob);
	}
}

/*
 * Populates vector of size NCells with type number based on lookup table (Spain version)
 */
void CSVReader::parsePROB2(std::vector<float> &probabilities, std::string ignition_prob)
{
	GDALDatasetUniquePtr poDataset;
	GDALAllRegister();
	const GDALAccess eAccess = GA_ReadOnly;
	poDataset = GDALDatasetUniquePtr(GDALDataset::FromHandle(GDALOpen(ignition_prob.c_str(), eAccess)));
	if (!poDataset)
	{
		printf("error\n"); // handle error
	}
	GDALRasterBand *poBand;
	poBand = poDataset->GetRasterBand(1);
	int nXSize = poBand->GetXSize();
	int nYSize = poBand->GetYSize();
	CPLErr Err = poBand->RasterIO(GF_Read, 0, 0, nXSize, nYSize,
								  &probabilities, nXSize, nYSize, GDT_Int32, // care with the GDT_Int32
								  0, 0);
}

/*
 * Populate Weather DF Spain
 */
void CSVReader::parseWeatherDF(weatherDF *wdf_ptr, arguments *args_ptr, std::vector<std::vector<std::string>> &DF, int WPeriods)
{
	int i;

	// Strings
	std::string::size_type sz; // alias of size_t

	// Floats
	float ws, waz, tmp = 27, rh = 40;
	float apcp = 0, ffmc = 0, dmc = 0, dc = 0, isi = 0, bui = 0, fwi = 0;

	// Loop over cells (populating per row)
	for (i = 1; i <= WPeriods; i++)
	{
		if (DF[i][3].compare("") == 0)
			waz = 0;
		else
		{
			waz = std::stoi(DF[i][3], &sz); //+ 180/2;   // DEBUGGING THE ANGLE
			if (waz >= 360)
			{
				waz = waz - 360;
			}
		}

		if (DF[i][2].compare("") == 0)
			ws = 0;
		else
			ws = std::stof(DF[i][2], &sz);

		if (args_ptr->Simulator == "K")
		{
			if (DF[i][4].compare("") == 0)
				tmp = 0;
			else
				tmp = std::stof(DF[i][4], &sz);

			if (DF[i][5].compare("") == 0)
				rh = 0;
			else
				rh = std::stof(DF[i][5], &sz);
		}
		else if (args_ptr->Simulator == "C")
		{

			if (DF[i][6].compare("") == 0)
				waz = 0;
			else
			{
				waz = std::stoi(DF[i][6], &sz); //+ 180/2;   // DEBUGGING THE ANGLE
				if (waz >= 360)
				{
					waz = waz - 360;
				}
			}
			if (DF[i][2].compare("") == 0)
				apcp = 0;
			else
				apcp = std::stof(DF[i][2], &sz);

			if (DF[i][3].compare("") == 0)
				tmp = 0;
			else
				tmp = std::stof(DF[i][3], &sz);

			if (DF[i][4].compare("") == 0)
				rh = 0;
			else
				rh = std::stof(DF[i][4], &sz);

			if (DF[i][5].compare("") == 0)
				ws = 0;
			else
				ws = std::stof(DF[i][5], &sz);

			if (DF[i][7].compare("") == 0)
				ffmc = 0;
			else
				ffmc = std::stof(DF[i][7], &sz);

			if (DF[i][8].compare("") == 0)
				dmc = 0;
			else
				dmc = std::stof(DF[i][8], &sz);

			if (DF[i][9].compare("") == 0)
				dc = 0;
			else
				dc = std::stof(DF[i][9], &sz);

			if (DF[i][10].compare("") == 0)
				isi = 0;
			else
				isi = std::stof(DF[i][10], &sz);

			if (DF[i][11].compare("") == 0)
				bui = 0;
			else
				bui = std::stof(DF[i][11], &sz);

			if (DF[i][12].compare("") == 0)
				fwi = 0;
			else
				fwi = std::stof(DF[i][12], &sz);
		}

		// Set values
		wdf_ptr->ws = ws;
		wdf_ptr->waz = waz;
		wdf_ptr->tmp = tmp;
		wdf_ptr->rh = rh;
		wdf_ptr->apcp = apcp;
		wdf_ptr->ffmc = ffmc;
		wdf_ptr->dmc = dmc;
		wdf_ptr->dc = dc;
		wdf_ptr->isi = isi;
		wdf_ptr->bui = bui;
		wdf_ptr->fwi = fwi;
		// Next pointer
		wdf_ptr++;
	}
}

/*
 * Populate IgnitionDF
 */
void CSVReader::parseIgnitionDF(std::vector<int> &ig, std::vector<std::vector<std::string>> &DF, int IgPeriods)
{
	// Integers
	int i, igcell;
	std::string::size_type sz; // alias of size_t

	// Loop over cells (populating per row)
	for (i = 1; i <= IgPeriods; i++)
	{
		// DEBUGprintf("Populating Ignition points: %d\n", i);
		igcell = std::stoi(DF[i][1], &sz);

		// Set values
		ig[i - 1] = igcell;

		// Next pointer
		// ig_ptr++;
	}
}

/*
 * Populate HarvestedDF
 */
void CSVReader::parseHarvestedDF(std::unordered_map<int, std::vector<int>> &hc, std::vector<std::vector<std::string>> &DF, int HPeriods)
{
	// Integers
	int i, j, hcell;
	std::vector<int> toHarvestCells;
	std::string::size_type sz; // alias of size_t

	// Loop over cells (populating per row)
	for (i = 1; i <= HPeriods; i++)
	{
		// Clean the vector before the new year
		toHarvestCells.clear();

		// Loop over years of the simulation
		for (j = 1; j < DF[i].size(); j++)
		{
			hcell = std::stoi(DF[i][j], &sz);

			// Set values
			toHarvestCells.push_back(hcell);
		}

		// Populate unordered set
		hc.insert(std::make_pair(i, toHarvestCells));
	}
}

/*
 * Populate BBO Tuning factors
 */
void CSVReader::parseBBODF(std::unordered_map<int, std::vector<float>> &bbo, std::vector<std::vector<std::string>> &DF, int NFTypes)
{
	// Integers
	int i, j, ftype;
	int ffactors = 4;
	std::vector<float> bboFactors;
	std::string::size_type sz; // alias of size_t

	// Loop over cells (populating per row)
	for (i = 1; i <= NFTypes; i++)
	{
		// Clean the vector before the fuels
		bboFactors.clear();

		// DEBUGprintf("Populating Ignition points: %d\n", i);
		ftype = std::stoi(DF[i][0], &sz);

		for (j = 1; j <= ffactors; j++)
		{
			bboFactors.push_back(std::stof(DF[i][j], &sz));
		}

		// Set values
		bbo.insert(std::make_pair(ftype, bboFactors));
	}
}

void CSVReader::parseForestDF(forestDF *frt_ptr, inputs *df_ptr, int rows, int cols, int cellside, double xllcorner, double yllcorner)
{

	std::vector<int> Aux;
	std::vector<std::vector<int>> coordCells;
	// CoordCells and Adjacents
	int n = 1;
	int r, c;
	// std::cout  << "CoordCells Debug" << std::endl;
	for (r = 0; r < rows; r++)
	{
		for (c = 0; c < cols; c++)
		{
			/*   CoordCells  */
			Aux = std::vector<int>();
			Aux.push_back(c);
			Aux.push_back(rows - r - 1);
			coordCells.push_back(Aux);
			df_ptr->lat = yllcorner + r * cellside;
			df_ptr->lon = xllcorner + c * cellside;
			df_ptr++;
			// printf("i,j = %d,%d\n", r,c);
			// std::cout << "x: " << coordCells[c + r*(cols)][0] <<  "  y: " << coordCells[c + r*(cols)][1]  <<   std::endl;
		}
	}
	frt_ptr->coordCells = coordCells;
}

void CSVReader::printDF(inputs df)
{
	std::cout << df.fueltype;
	std::cout << " ";
	std::cout << " " << df.elev;
	std::cout << " " << df.ws;
	std::cout << " " << df.waz;
	std::cout << " " << df.ps;
	std::cout << " " << df.saz;
	std::cout << " " << df.cur;
	std::cout << " " << df.cbd;
	std::cout << " " << df.cbh;
	std::cout << " " << df.ccf;
	std::cout << " " << df.FMC << std::endl;
}

void CSVReader::printWeatherDF(weatherDF wdf)
{
	std::cout << " " << wdf.ws;
	std::cout << " " << wdf.waz;
}

/*
int main()
{
	// Creating an object of CSVWriter
	CSVReader reader("example.csv");

	// Get the data from CSV File
	std::vector<std::vector<std::string> > dataList = reader.getData();

	// Print the content of row by row on screen
	for(std::vector<std::string> vec : dataList)
	{
		for(std::string data : vec)
		{
			std::cout<<data<< " ";
		}
		std::cout<<std::endl;
	}
	return 0;

}
*/
