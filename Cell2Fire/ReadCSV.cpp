#include "ReadCSV.h"
#include "Cells.h"
#include "DataGenerator.h"
#include "ReadArgs.h"

#include "tiffio.h"
#include <algorithm>
#include <boost/algorithm/string.hpp>
#include <fstream>
#include <iostream>
#include <iterator>
#include <math.h>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

/**
 * Creates an instance of CSVReader.
 * @param filename name of file to read
 * @param delm delimiter of columns in file.
 */
CSVReader::CSVReader(std::string filename, std::string delm)
{
    this->fileName = filename;
    this->delimeter = delm;
}

CSVReader::CSVReader()
{
    this->fileName = "";
    this->delimeter = ",";
}

void
CSVReader::setFilename(std::string name)
{
    this->fileName = name;
}

/**
 * @brief Reads and parses data from a CSV, ASCII or TIFF file into a 2D vector.
 *
 * Checks for the existence of the input file with either a `.tif` or `.asc` extension
 * and processes it accordingly.
 * If the file is in CSV format, it is read line by line and split using
 * the specified delimiter. If the file is in ASCII format, the function reads the header separately
 * before parsing the data. If the file is in TIFF format, it extracts metadata (e.g., grid dimensions,
 * cell size, and coordinates) and reads raster data row by row.
 *
 * @return A 2D vector of strings containing the parsed data.
 *         - For ASC and CSV files, each row is stored as a vector of strings.
 *         - For TIFF files, metadata is stored in the first few rows, followed by pixel values.
 *
 * @throws std::runtime_error If the file type is unsupported, memory allocation fails, or an error occurs during file
 * reading.
 */
std::vector<std::vector<std::string>>
CSVReader::getData(string filename)
{
    /*
    Check wheather fuels.tif or .asc is in InFolder and add corresponding
    extension
    */
    std::string extension;

    if (fileExists(filename + ".tif"))
    {
        extension = ".tif";
        std::cout << filename + extension << '\n';
    }
    else if (fileExists(filename + ".asc"))
    {
        extension = ".asc";
        std::cout << filename + extension << '\n';
    }
    else
    {
        extension = "";
    }
    filename = filename + extension;
    std::ifstream file(filename);
    std::vector<std::vector<std::string>> dataList;
    std::string line = "";
    // Iterate through each line and split the content using delimeter

    if (filename.substr(filename.find_last_of(".") + 1) == "asc")
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
    else if (filename.substr(filename.find_last_of(".") + 1) == "tif")
    {
        TIFF* fuelsDataset = TIFFOpen(filename.c_str(), "r");
        uint32_t nXSize, nYSize;
        TIFFGetField(fuelsDataset, TIFFTAG_IMAGEWIDTH, &nXSize);
        TIFFGetField(fuelsDataset, TIFFTAG_IMAGELENGTH, &nYSize);
        double* modelPixelScale;
        uint32_t count;
        // TIFFGetField(tiff, 33424, &count, &data);
        TIFFGetField(fuelsDataset, 33550, &count, &modelPixelScale);
        // Gets cell size
        double cellSizeX{ modelPixelScale[0] };
        double cellSizeY{ modelPixelScale[1] };
        const double epsilon = std::numeric_limits<double>::epsilon();
        tsize_t scan_size = TIFFRasterScanlineSize(fuelsDataset);
        int n_bits = (scan_size / nXSize) * 8;
        if (fabs(cellSizeX - cellSizeY) > epsilon)
        {
            throw std::runtime_error("Error: Cells are not square in: '" + fileName + "'");
        }
        double* positions;
        TIFFGetField(fuelsDataset, 33922, &count, &positions);
        double xllcorner{ positions[3] };
        double yllcorner{ positions[4] };
        std::vector<std::string> vec;
        vec.push_back("ncols");
        vec.push_back(std::to_string(nXSize));
        dataList.push_back(vec);
        std::vector<std::string> vec2;
        vec2.push_back("nrows");
        vec2.push_back(std::to_string(nYSize));
        dataList.push_back(vec2);
        std::vector<std::string> vec3;
        vec3.push_back("xllcorner");
        vec3.push_back(std::to_string(int(xllcorner)));
        dataList.push_back(vec3);
        std::vector<std::string> vec4;
        vec4.push_back("yllcorner");
        vec4.push_back(std::to_string(int(yllcorner)));
        dataList.push_back(vec4);
        std::vector<std::string> vec5;
        vec5.push_back("cellsize");
        vec5.push_back(std::to_string(cellSizeX));
        dataList.push_back(vec5);
        void* buf;
        if (n_bits == 64)
        {
            buf = (double*)_TIFFmalloc(nXSize * sizeof(double));
        }
        else if (n_bits == 32)
        {
            buf = (int32_t*)_TIFFmalloc(nXSize * sizeof(int32_t));
        }
        else
        {
            throw std::runtime_error("Error: file type is not supported: '" + fileName + "'");
        }
        if (!buf)
        {
            TIFFClose(fuelsDataset);
            throw std::runtime_error("Could not allocate memory");
        }
        // For each row
        for (int i = 0; i < nYSize; i++)
        {
            std::vector<std::string> vec_rows;
            // Read pixel values for the current row
            if (TIFFReadScanline(fuelsDataset, buf, i) != 1)
            {
                _TIFFfree(buf);
                TIFFClose(fuelsDataset);
                throw std::runtime_error("Read error on row " + std::to_string(i));
            }
            // For each column
            std::string token;
            for (int j = 0; j < nXSize; j++)
            {
                // Access the pixel value at position (i, j)
                if (n_bits == 64)
                {
                    token = std::to_string(static_cast<int>(((double*)buf)[j]));
                }
                else
                {
                    token = std::to_string(static_cast<int>(((int32_t*)buf)[j]));
                }
                vec_rows.push_back(token);
            }
            dataList.push_back(vec_rows);
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

/**
 * @brief print data contained in 2D vector to console row by row
 * @param DF 2D vector of strings
 */
void
CSVReader::printData(std::vector<std::vector<std::string>>& DF)
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

/**
 * @brief Populates a vector of size NCells with fuel type number per cell
 * @param NFTypes vector of fuel type per cell
 * @param DF 2D vector of input data
 * @param NCells number of cells
 */
void
CSVReader::parseNDF(std::vector<int>& NFTypes, std::vector<std::vector<std::string>>& DF, int NCells)
{
    // TODO: change this to use dfptr, only used in bbo
    int i;
    // Ints
    int FType;
    // CChar
    const char* faux;
    std::string::size_type sz;  // alias of size_t

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

/**
 * @brief Not currently supported. Populates a vector of size NCells with ignition probability per cell.
 *
 * This is not currently supported because ignition probability is not stored in `DF`.
 * @param probabilities vector of ignition probability per cell
 * @param DF 2D vector of input data
 * @param NCells number of cells
 */
void
CSVReader::parsePROB(std::vector<float>& probabilities, std::vector<std::vector<std::string>>& DF, int NCells)
{
    int i;

    // Ints
    float Prob;

    // CChar
    const char* faux;
    std::string::size_type sz;  // alias of size_t

    // Loop over cells (populating per row)
    for (i = 1; i <= NCells; i++)
    {
        // printf("Populating DF for cell %d\n", i);
        if (DF[i][14].compare("") == 0)
            // TODO: change to df_ptr? or directly read file
            Prob = 1;
        else
            Prob = std::stof(DF[i][14], &sz);

        // Set values
        probabilities[i - 1] = Prob;
        // probabilities.push_back(Prob);
    }
}

/*
 * Populate Weather DF Spain
 */
void
CSVReader::parseWeatherDF(std::vector<weatherDF>& wdf,
                          arguments* args_ptr,
                          std::vector<std::vector<std::string>>& DF,
                          int WPeriods)
{
    std::string::size_type sz;  // alias of size_t

    float ws = 0, waz = 0, tmp = 27, rh = 40;
    float apcp = 0, ffmc = 0, dmc = 0, dc = 0, isi = 0, bui = 0, fwi = 0;

    // One element per weather period
    if (static_cast<int>(wdf.size()) < WPeriods)
        wdf.resize(WPeriods);

    // Loop over weather periods (populating per row)
    for (int i = 1; i <= WPeriods; i++)
    {
        // Reference to current weatherDF element
        weatherDF& w = wdf[i - 1];

        if (DF[i][3].empty())
            waz = 0;
        else
        {
            waz = std::stoi(DF[i][3], &sz);
            waz = fmod((waz + 180.0), 360.0);
        }

        ws = DF[i][2].empty() ? 0 : std::stof(DF[i][2], &sz);

        if (args_ptr->Simulator == "K")
        {
            tmp = DF[i][4].empty() ? 0 : std::stof(DF[i][4], &sz);
            rh = DF[i][5].empty() ? 0 : std::stof(DF[i][5], &sz);
        }
        else if (args_ptr->Simulator == "C")
        {
            waz = DF[i][6].empty() ? 0 : std::stoi(DF[i][6], &sz);
            if (waz >= 360)
                waz -= 360;

            apcp = DF[i][2].empty() ? 0 : std::stof(DF[i][2], &sz);
            tmp = DF[i][3].empty() ? 0 : std::stof(DF[i][3], &sz);
            rh = DF[i][4].empty() ? 0 : std::stof(DF[i][4], &sz);
            ws = DF[i][5].empty() ? 0 : std::stof(DF[i][5], &sz);
            ffmc = DF[i][7].empty() ? 0 : std::stof(DF[i][7], &sz);
            dmc = DF[i][8].empty() ? 0 : std::stof(DF[i][8], &sz);
            dc = DF[i][9].empty() ? 0 : std::stof(DF[i][9], &sz);
            isi = DF[i][10].empty() ? 0 : std::stof(DF[i][10], &sz);
            bui = DF[i][11].empty() ? 0 : std::stof(DF[i][11], &sz);
            fwi = DF[i][12].empty() ? 0 : std::stof(DF[i][12], &sz);
        }

        // Assign parsed values
        w.ws = ws;
        w.waz = waz;
        w.tmp = tmp;
        w.rh = rh;
        w.apcp = apcp;
        w.ffmc = ffmc;
        w.dmc = dmc;
        w.dc = dc;
        w.isi = isi;
        w.bui = bui;
        w.fwi = fwi;
    }
}

/*
 * Populate IgnitionDF
 */
void
CSVReader::parseIgnitionDF(std::vector<int>& ig, std::vector<std::vector<std::string>>& DF, int IgPeriods)
{
    // Integers
    int i, igcell;
    std::string::size_type sz;  // alias of size_t

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
void
CSVReader::parseHarvestedDF(std::unordered_map<int, std::vector<int>>& hc,
                            std::vector<std::vector<std::string>>& DF,
                            int HPeriods)
{
    // Integers
    int i, j, hcell;
    std::vector<int> toHarvestCells;
    std::string::size_type sz;  // alias of size_t

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
void
CSVReader::parseBBODF(std::unordered_map<int, std::vector<float>>& bbo,
                      std::vector<std::vector<std::string>>& DF,
                      int NFTypes)
{
    // TODO: change this to use dfptr
    //  Integers
    int i, j, ftype;
    int ffactors = 4;
    std::vector<float> bboFactors;
    std::string::size_type sz;  // alias of size_t

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

void
CSVReader::parseForestDF(forestDF* frt_ptr, std::vector<std::vector<std::string>>& DF)
{
    // Ints
    int cellside, rows, cols;
    int i, j;
    double xllcorner, yllcorner;
    // std::string xllcorner;
    std::string::size_type sz;  // alias of size_t
    std::unordered_map<std::string, int> Aux;
    std::vector<int> Aux2;
    cols = std::stoi(DF[0][1], &sz);
    rows = std::stoi(DF[1][1], &sz);

    // Others
    // std::vector<std::unordered_map<std::string, int>> adjCells; //Change this
    // to a function for memory improvement
    std::vector<std::vector<int>> coordCells;

    std::string North = "N";
    std::string South = "S";
    std::string East = "E";
    std::string West = "W";
    std::string NorthEast = "NE";
    std::string NorthWest = "NW";
    std::string SouthEast = "SE";
    std::string SouthWest = "SW";
    // Maybe parse only for avail cells
    //  Filling DF
    // DEBUGprintf("Populating Forest DF\n");

    xllcorner = std::stod(DF[2][1], &sz);
    yllcorner = std::stod(DF[3][1], &sz);
    cellside = std::stoi(DF[4][1], &sz);

    // DEBUGprintf("cols: %d,  rows:  %d,   cellside:  %d\n", cols, rows,
    // cellside);

    // CoordCells and Adjacents
    int n = 1;
    int r, c;
    // std::cout  << "CoordCells Debug" << std::endl;
    for (r = 0; r < rows; r++)
    {
        for (c = 0; c < cols; c++)
        {

            /*   CoordCells  */
            Aux2 = std::vector<int>();
            Aux2.push_back(c);
            Aux2.push_back(rows - r - 1);
            coordCells.push_back(Aux2);
            // printf("i,j = %d,%d\n", r,c);
            // std::cout << "x: " << coordCells[c + r*(cols)][0] <<  "  y: " <<
            // coordCells[c + r*(cols)][1]  <<   std::endl;

            /*   Adjacents  */
            // if we have rows (not a forest = line)
        }
    }
    // Set values
    frt_ptr->cellside = cellside;
    frt_ptr->rows = rows;
    frt_ptr->cols = cols;
    frt_ptr->coordCells = coordCells;
    // frt_ptr->adjCells = adjCells;
    frt_ptr->xllcorner = xllcorner;
    frt_ptr->yllcorner = yllcorner;
}

void
CSVReader::printDF(inputs df)
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

void
CSVReader::printWeatherDF(weatherDF wdf)
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
