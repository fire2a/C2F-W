#include "ReadCSV.h"
#include "CellsFBP.h"
#include "ReadArgs.h"

#include <iostream>
#include <fstream>
#include <vector>
#include <iterator>
#include <string>
#include <algorithm>
#include <unordered_map>
#include <unordered_set>
#include <boost/algorithm/string.hpp>
 
/*
 * Constructur
 */
CSVReader::CSVReader(std::string filename, std::string delm){
	this->fileName = filename;
	this->delimeter = delm;		
}
 
 
/*
* Parses through csv file line by line and returns the data
* in vector of vector of strings.
*/
std::vector<std::vector<std::string>> CSVReader::getData() {
	std::ifstream file(this->fileName);
	std::vector<std::vector<std::string> > dataList;
	std::string line = "";
	// Iterate through each line and split the content using delimeter
	if (this->fileName.substr(this->fileName.find_last_of(".") + 1) == "asc") {
		int header = 0;
		while (getline(file, line)) {
			if (header < 5) {
				std::vector<std::string> vec;
				int start = 0;
				int end = 0;
				while ((start = line.find_first_not_of(this->delimeter, end)) != std::string::npos) {
					end = line.find(this->delimeter, start);
					vec.push_back(line.substr(start, end - start));
				}
				dataList.push_back(vec);
				header++;
			}
			else {
				std::vector<std::string> vec;
				boost::algorithm::split(vec, line, boost::is_any_of(this->delimeter));
				dataList.push_back(vec);
			}

		}
	}

	else {
		while (getline(file, line)) {
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
* Prints data to screen inside the DF obtained from the CSV file
*/
void CSVReader::printData(std::vector<std::vector<std::string>> & DF){
	// Print the content of row by row on screen
	for(std::vector<std::string> vec : DF)
	{
		for(std::string data : vec)
		{
			if(data.length() >0) std::cout << " " << data << " ";
			else std::cout << " " <<  "NaN" << " ";
		}
		std::cout<<std::endl;
	}
}


/*
* Populates the df input objects based on the DF csv file for each row/cell (spanish version)
*/
void CSVReader::parseDF(inputs * df_ptr, std::vector<std::vector<std::string>> & DF,arguments* args_ptr,int NCells){
	int i;
	
	// Floats 
	float cur, elev, ws, waz, saz, cbd, cbh, ccf, ps;
	
	// Integers 
	int nftype,FMC;
	
	// CChar
	const char * faux;
	std::string::size_type sz;   // alias of size_t
	
	// Loop over cells (populating per row)
	for (i=1; i <= NCells; i++){
		//printf("Populating DF for cell %d\n", i);
		faux = DF[i][0].append(" ").c_str();
		
		if (DF[i][3].compare("") == 0) elev = 0;
		else elev = std::stof (DF[i][3] ,&sz);
		
		if (DF[i][4].compare("") == 0) ws = 0;
		else ws = std::stof (DF[i][4], &sz);
		
		if (DF[i][5].compare("") == 0) waz = 0;
		else waz = std::stoi (DF[i][5] ,&sz) + 180.;// + 2*90;  // CHECK!!!!
		if (waz >= 360) waz = waz - 360;
		
		if (DF[i][6].compare("") == 0) ps = 0;
		else ps = std::stof (DF[i][6] ,&sz);
		
		if (DF[i][7].compare("") == 0) saz = 0;
		else saz = std::stof (DF[i][7] ,&sz);
		
		if (DF[i][8].compare("") == 0) cur = 0;
		else cur = std::stof (DF[i][8] ,&sz);
			
		if (DF[i][9].compare("") == 0) cbd = 0;
		else cbd = std::stof (DF[i][9], &sz);
		
		if (DF[i][10].compare("") == 0) cbh = 0;
		else cbh = std::stof (DF[i][10], &sz);
		
		if (DF[i][11].compare("") == 0) ccf = 0;
		else ccf = std::stof (DF[i][11], &sz);
		
		if (DF[i][12].compare("") == 0) nftype = 0;
		else nftype = std::stoi (DF[i][12], &sz);
		
		if (DF[i][13].compare("") == 0) FMC = args_ptr->FMC;
		else FMC = std::stoi (DF[i][13], &sz);
		
		// Set values
		strncpy(df_ptr->fueltype, faux, 4);
		df_ptr->elev=elev; df_ptr->ws=ws; df_ptr->waz=waz;  
		df_ptr->ps=ps; df_ptr->saz=saz; df_ptr->cur=cur; 
		df_ptr->cbd=cbd;df_ptr->cbh=cbh; df_ptr->ccf=ccf;
		df_ptr->nftype=nftype;df_ptr->FMC=FMC;
			
		// Next pointer
		df_ptr++;
	}
}


/*
* Populates vector of size NCells with type number based on lookup table (Spain version)
*/
void CSVReader::parseNDF(std::vector<int> &NFTypes, std::vector<std::vector<std::string>> & DF, int NCells){
	int i;
	
	// Ints 
	int FType;
	
	// CChar
	const char * faux;
	std::string::size_type sz;   // alias of size_t
	
	// Loop over cells (populating per row)
	for (i=1; i <= NCells; i++){
		//printf("Populating DF for cell %d\n", i);
		if (DF[i][12].compare("") == 0) FType = 0;
		else FType = std::stoi (DF[i][12], &sz);
			
		// Set values
		NFTypes.push_back(FType);
		
	}
}



/*
* Populates vector of size NCells with type number based on lookup table (Spain version)
*/
void CSVReader::parsePROB(std::vector<float>& probabilities, std::vector<std::vector<std::string>>& DF, int NCells) {
	int i;

	// Ints 
	float Prob;

	// CChar
	const char* faux;
	std::string::size_type sz;   // alias of size_t

	// Loop over cells (populating per row)
	for (i = 1; i <= NCells; i++) {
		//printf("Populating DF for cell %d\n", i);
		if (DF[i][14].compare("") == 0) Prob = 1;
		else Prob = std::stof(DF[i][14], &sz);


		// Set values
		probabilities[i-1]=Prob;
		//probabilities.push_back(Prob);

	}
}


/*
* Populate Weather DF Spain
*/
void CSVReader::parseWeatherDF(weatherDF * wdf_ptr,arguments* args_ptr, std::vector<std::vector<std::string>> & DF, int WPeriods){
	int i;
	
	//Strings
	std::string::size_type sz;   // alias of size_t
	
	//Floats 
	float ws, waz, tmp=27, rh=40;
	
	
	// Loop over cells (populating per row)
	for (i=1; i <= WPeriods; i++){
		if (DF[i][3].compare("") == 0) waz = 0;
		else {waz = std::stoi (DF[i][3] ,&sz); //+ 180/2;   // DEBUGGING THE ANGLE 
			if (waz >= 360){
				waz = waz - 360;
			}
		}

		if (DF[i][2].compare("") == 0) ws = 0;
		else ws = std::stof (DF[i][2], &sz);
		if (args_ptr->Simulator=="K"){
			if (DF[i][4].compare("") == 0) tmp = 0;
			else tmp = std::stof(DF[i][4], &sz);

			if (DF[i][5].compare("") == 0) rh = 0;
			else rh = std::stof(DF[i][5], &sz);

		}


		
		// Set values
		wdf_ptr->ws=ws; wdf_ptr->waz=waz; 
		wdf_ptr->tmp = tmp; wdf_ptr->rh = rh;
		// Next pointer
		wdf_ptr++;
	}
	
}



/*
* Populate IgnitionDF
*/
void CSVReader::parseIgnitionDF(std::vector<int> & ig, std::vector<std::vector<std::string>> & DF, int IgPeriods){
	// Integers
	int i, igcell;
	std::string::size_type sz;   // alias of size_t
	
	// Loop over cells (populating per row)
	for (i=1; i <= IgPeriods; i++){
		//DEBUGprintf("Populating Ignition points: %d\n", i);
		igcell = std::stoi(DF[i][1], &sz);
		
		// Set values
		ig[i-1]= igcell;
			
		// Next pointer
		//ig_ptr++;
	}
	
	
}


/*
* Populate HarvestedDF
*/
void CSVReader::parseHarvestedDF(std::unordered_map<int, std::vector<int>> & hc, std::vector<std::vector<std::string>> & DF, int HPeriods){
	// Integers
	int i, j, hcell;
	std::vector<int> toHarvestCells;
	std::string::size_type sz;   // alias of size_t
	
	// Loop over cells (populating per row)
	for (i=1; i <= HPeriods; i++){
		// Clean the vector before the new year 
		toHarvestCells.clear();
		
		// Loop over years of the simulation
		for(j=1; j < DF[i].size(); j++){
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
void CSVReader::parseBBODF(std::unordered_map<int, std::vector<float>> & bbo, std::vector<std::vector<std::string>> & DF, int NFTypes){
	// Integers
	int i, j, ftype;
	int ffactors = 4;
	std::vector<float> bboFactors;
	std::string::size_type sz;   // alias of size_t
	
	// Loop over cells (populating per row)
	for (i=1; i <= NFTypes; i++){
		// Clean the vector before the fuels
		bboFactors.clear();
		
		//DEBUGprintf("Populating Ignition points: %d\n", i);
		ftype = std::stoi(DF[i][0], &sz);
		
		for (j=1; j <= ffactors; j++){
			bboFactors.push_back(std::stof(DF[i][j], &sz));
		}

		//Set values							 
		bbo.insert(std::make_pair(ftype, bboFactors));	
	}
	
	
}


void CSVReader::parseForestDF(forestDF * frt_ptr, std::vector<std::vector<std::string>> & DF){
	// Ints 
	int cellside, rows, cols;
	int i, j;
	double xllcorner, yllcorner;
	//std::string xllcorner;
	std::string::size_type sz;   // alias of size_t
	std::unordered_map<std::string, int> Aux;
	std::vector<int> Aux2;
	cols = std::stoi(DF[0][1], &sz);
	rows = std::stoi(DF[1][1], &sz);

	// Others 
	//std::vector<std::unordered_map<std::string, int>> adjCells; //Change this to a function for memory improvement
	std::vector<std::vector<int>> coordCells;
	
	std::string North = "N";
    std::string South = "S";
    std::string East = "E";
    std::string West = "W";
    std::string NorthEast = "NE";
    std::string NorthWest = "NW";
    std::string SouthEast = "SE";
    std::string SouthWest = "SW";
	//Maybe parse only for avail cells
	// Filling DF
	//DEBUGprintf("Populating Forest DF\n");

	xllcorner = std::stod(DF[2][1], &sz);
	yllcorner = std::stod(DF[3][1], &sz);
	cellside = std::stoi(DF[4][1], &sz);
	
	//DEBUGprintf("cols: %d,  rows:  %d,   cellside:  %d\n", cols, rows, cellside);
	
	// CoordCells and Adjacents
	int n = 1; 
	int r, c;
	//std::cout  << "CoordCells Debug" << std::endl;
	for (r=0; r<rows; r++){
		for (c=0; c < cols; c++){
			
			/*   CoordCells  */
			Aux2 = std::vector<int>();
			Aux2.push_back(c); 
            Aux2.push_back(rows-r-1);   
			coordCells.push_back(Aux2);                    
			//printf("i,j = %d,%d\n", r,c);
			//std::cout << "x: " << coordCells[c + r*(cols)][0] <<  "  y: " << coordCells[c + r*(cols)][1]  <<   std::endl;
					
			/*   Adjacents  */
			// if we have rows (not a forest = line)
		}
	}
	// Set values
	frt_ptr->cellside = cellside;
	frt_ptr->rows = rows;
	frt_ptr->cols = cols;
	frt_ptr->coordCells = coordCells;
	//frt_ptr->adjCells = adjCells;
	frt_ptr->xllcorner = xllcorner;
	frt_ptr->yllcorner = yllcorner;
		
	
}


void CSVReader::printDF(inputs df){
	std::cout << df.fueltype; std::cout << " ";
	std::cout << " " << df.elev; std::cout << " " << df.ws; std::cout << " " << df.waz; 
	std::cout << " " << df.ps; std::cout << " " << df.saz; std::cout << " " << df.cur; 
	std::cout << " " << df.cbd; std::cout << " " << df.cbh; std::cout << " " << df.ccf; std::cout << " " << df.FMC  << std::endl;
}


void CSVReader::printWeatherDF(weatherDF wdf){
	std::cout << " " << wdf.ws; std::cout << " " << wdf.waz; 
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
