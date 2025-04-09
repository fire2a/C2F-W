#include "WriteCSV.h"

#include <algorithm>
#include <boost/algorithm/string.hpp>
#include <cmath>
#include <fstream>
#include <iostream>
#include <iterator>
#include <set>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#if defined _WIN32 || defined __CYGWIN__
#include <Windows.h>
#include <direct.h>
#include <sys/stat.h>
#include <sys/types.h>
#endif

/**
 * @brief Constructor for CSVWriter object.
 * @param filename name of output file
 * @param delm delimeter to use in output file
 */
CSVWriter::CSVWriter(std::string filename, std::string delm)
{
    this->fileName = filename;
    this->delimeter = delm;
    this->linesCount = 0;
}

/**
 * @brief Writes a row of data to a file
 * Takes a range of values and appends them as a single row to the output file. If it's the first line being
 * written, the file is truncated (cleared), otherwise, new rows are appended.
 *
 * Each element in the range is written to the file, separated by the configured delimiter.
 *
 * @tparam T An input iterator type pointing to elements that can be streamed to a file (e.g.,
 * std::vector<std::string>::iterator).
 * @param first Iterator to the beginning of the range.
 * @param last Iterator to the end of the range.
 */
template <typename T>
void
CSVWriter::addDatainRow(T first, T last)
{
    std::fstream file;
    // Open the file in truncate mode if first line else in Append Mode
    file.open(this->fileName, std::ios::out | (this->linesCount ? std::ios::app : std::ios::trunc));

    // Iterate over the range and add each element to file seperated by
    // delimeter.
    for (; first != last;)
    {
        file << *first;
        if (++first != last)
            file << this->delimeter;
    }
    file << "\n";
    this->linesCount++;

    // Close the file
    file.close();
}

/**
 * @brief Writes the ascii header onto a raster file.
 *
 * Opens the output file and writes the first few lines of metadata. These indicate the number of rows and columns, the
 * coordinates of the lower left corner cell, cellside length and which value indicates there's no data for a cell.
 * @param rows number of rows
 * @param cols number of columns
 * @param xllcorner X-coordinate of the lower left corner cell
 * @param yllcorner Y-coordinate of the lower left corner cell
 * @param cellside length of the side of a cell
 */
void
CSVWriter::asciiHeader(int rows, int cols, double xllcorner, double yllcorner, int cellside)
{
    std::fstream file;
    // Open the file in truncate mode if first line else in Append Mode
    file.open(this->fileName, std::ios::out | (this->linesCount ? std::ios::app : std::ios::trunc));
    // first line: coles
    file << "ncols";
    file << this->delimeter;
    file << cols;
    file << "\n";
    // second line: rows
    file << "nrows";
    file << this->delimeter;
    file << rows;
    file << "\n";
    // third line: xllcorner
    file << "xllcorner";
    file << this->delimeter;
    file << xllcorner;
    file << "\n";
    // fourth line: yllcorner
    file << "yllcorner";
    file << this->delimeter;
    file << yllcorner;
    file << "\n";
    // cellsize
    file << "cellsize";
    file << this->delimeter;
    file << cellside;
    file << "\n";
    // NODATA_value
    file << "NODATA_value";
    file << this->delimeter;
    file << -9999;
    file << "\n";
    this->linesCount++;
    // Close the file
    file.close();
}

/**
 * @brief Writes a 2D grid of cell statuses to a CSV file.
 *
 * Converts a flattened 1D vector representing cell statuses in row-major order
 * into a 2D CSV format and writes it using the configured delimiter.
 *
 * Each row in the grid corresponds to one line in the CSV file.
 *
 * @param rows Number of rows in the grid.
 * @param cols Number of columns in the grid.
 * @param statusCells Flattened vector of size (rows * cols) storing the status of each cell.
 */
void
CSVWriter::printCSV(int rows, int cols, std::vector<int> statusCells)
{
    // Create a rowVector for printing
    std::vector<int> rowVector;

    // Adding vector to CSV File
    int r, c;

    // Printing rows (output)
    for (r = 0; r < rows; r++)
    {
        for (c = 0; c < cols; c++)
        {

            std::vector<int>::const_iterator first = statusCells.begin() + c + r * cols;
            std::vector<int>::const_iterator last = statusCells.begin() + c + r * cols + cols;
            std::vector<int> rowVector(first, last);

            this->addDatainRow(rowVector.begin(), rowVector.end());
            c += cols;
        }
    }
}

/*
 *     Creates CSVDouble
 */
void
CSVWriter::printCSVDouble(int rows, int cols, std::vector<double> network)
{
    // Create a rowVector for printing
    std::vector<double> rowVector;

    // Adding vector to CSV File
    int r, c;
    bool out;
    out = false;

    // Printing rows (output)
    for (r = 0; r < rows; r++)
    {

        for (c = 0; c < cols; c++)
        {
            if (network[c + r * cols] < 1 || std::ceil(network[c + r * cols]) != network[c + r * cols])
            {
                out = true;
                break;
            }

            std::vector<double>::const_iterator first = network.begin() + c + r * cols;
            std::vector<double>::const_iterator last = network.begin() + c + r * cols + cols;
            std::vector<double> rowVector(first, last);

            this->addDatainRow(rowVector.begin(), rowVector.end());
            c += cols;
        }

        if (out)
        {
            break;
        }
    }
}

// Ofstream version to save faster
void
CSVWriter::printCSVDouble_V2(int rows, int cols, std::vector<double> network)
{
    bool outs = false;
    std::ofstream ofs(this->fileName, std::ofstream::out);
    for (int r = 0; r < rows; r++)
    {
        for (int c = 0; c < cols; c++)
        {
            if (network[c + r * cols] < 1 || network[c + r * cols + 1] < 1
                || std::ceil(network[c + r * cols]) != network[c + r * cols])
            {
                outs = true;
                break;
            }
            ofs << (int)network[c + r * cols] << this->delimeter << (int)network[c + r * cols + 1] << this->delimeter
                << (int)network[c + r * cols + 2] << this->delimeter << network[c + r * cols + 3] << "\n";
            c += cols;
        }

        if (outs)
        {
            break;
        }
    }
    // Close file
    ofs.close();
}

/**
 * @brief Writes a raster layer with float data in ASCII format onto a file.
 *
 * Creates a raster file in ASCII format from the contents of `statusCells`. It includes the corresponding ASCII header.
 * `statusCells` must be a vector of `float` type values.
 *
 * @param rows number of rows in grid
 * @param cols number of columns in grid
 * @param xllcorner x-coordinate of lower left corner cell
 * @param yllcorner y-coordiante of lower left corner cell
 * @param cellside length of the side of a cell
 * @param statusCells float vector with cell data to print
 */
void
CSVWriter::printASCII(
    int rows, int cols, double xllcorner, double yllcorner, int cellside, std::vector<float> statusCells)
{
    // Create a rowVector for printing
    std::vector<float> rowVector;

    // Adding vector to CSV File
    int r, c;
    // Add header to ascii file

    this->asciiHeader(rows, cols, xllcorner, yllcorner, cellside);

    // Printing rows (output)
    for (r = 0; r < rows; r++)
    {
        for (c = 0; c < cols; c++)
        {

            std::vector<float>::const_iterator first = statusCells.begin() + c + r * cols;
            std::vector<float>::const_iterator last = statusCells.begin() + c + r * cols + cols;
            std::vector<float> rowVector(first, last);

            this->addDatainRow(rowVector.begin(), rowVector.end());
            c += cols;
        }
    }
}

/**
 * @brief Writes a raster layer with int data in ASCII format onto a file.
 *
 * Creates a raster file in ASCII format from the contents of `statusCells`. It includes the corresponding ASCII header.
 * `statusCells` must be a vector of `int` type values.
 *
 * @param rows number of rows in grid
 * @param cols number of columns in grid
 * @param xllcorner x-coordinate of lower left corner cell
 * @param yllcorner y-coordiante of lower left corner cell
 * @param cellside length of the side of a cell
 * @param statusCells int vector with cell data to print
 */
void
CSVWriter::printASCIIInt(
    int rows, int cols, double xllcorner, double yllcorner, int cellside, std::vector<int> statusCells)
{
    // Create a rowVector for printing
    std::vector<int> rowVector;

    // Adding vector to CSV File
    int r, c;
    // Add header to ascii file

    this->asciiHeader(rows, cols, xllcorner, yllcorner, cellside);

    // Printing rows (output)
    for (r = 0; r < rows; r++)
    {
        for (c = 0; c < cols; c++)
        {

            std::vector<int>::const_iterator first = statusCells.begin() + c + r * cols;
            std::vector<int>::const_iterator last = statusCells.begin() + c + r * cols + cols;
            std::vector<int> rowVector(first, last);

            this->addDatainRow(rowVector.begin(), rowVector.end());
            c += cols;
        }
    }
}

/**
 * @brief writes the weather file history onto an output file
 *
 * Outputs the list of weather files that were randomly selected in each simulation.
 * @param weatherHistory String vector with weather file names
 */
void
CSVWriter::printWeather(std::vector<std::string> weatherHistory)
{
    std::ofstream ofs(this->fileName, std::ofstream::out);
    int i;

    for (i = 0; i < weatherHistory.size(); i++)
    {
        ofs << weatherHistory[i] << "\n";
    }
    // Close file
    ofs.close();
}

/**
 * @brief writes the ignition point history onto an output file
 *
 * Outputs the list of ignition points that were randomly selected for each simulation.
 * @param ignitionsHistory
 */
void
CSVWriter::printIgnitions(std::unordered_map<int, int> ignitionsHistory)
{
    std::ofstream ofs(fileName, std::ofstream::out);
    int i;

    // Print column titles
    ofs << "sim,point\n";

    // Iterate through the map and print key-value pairs
    for (i = 1; i < ignitionsHistory.size() + 1; i++)
    {
        ofs << i << "," << ignitionsHistory[i] << "\n";
        // ofs << i << "," << ignitionsHistory[i-1] << "\n";
    }

    // Close file
    ofs.close();
}

// not used
void
CSVWriter::printCSV_V2(int rows, int cols, std::vector<int> statusCells)
{
    std::ofstream ofs(this->fileName, std::ofstream::out);
    std::string toOut;
    int r, c, i;

    for (r = 0; r < rows; r++)
    {
        for (c = 0; c < cols; c++)
        {
            std::string toOut;
            for (i = 0; i < cols; i++)
            {
                toOut += std::to_string(statusCells[c + r * cols + i]) + this->delimeter;
            }
            ofs << toOut << "\n";
            c += cols;
        }
    }
    // Close file
    ofs.close();
}

/**
 * @brief Creates the directory for a given path
 *
 * Distinguishes between Linux/MacOS systems and Windows systems, and calls the appropriate command to create the given
 * directory. It can create only one new directory per call, so only the last component of `pathPlot` can name a new
 * directory.
 *
 * @param pathPlot Path to the new directory
 */
void
CSVWriter::MakeDir(std::string pathPlot)
{
    // Default folder simOuts
    const char* Dir;
#if defined _WIN32 || defined __CYGWIN__
    Dir = pathPlot.c_str();
    int ret = _mkdir(Dir);
#else
    std::string inst = "mkdir -p ";
    pathPlot = inst + pathPlot;
    Dir = pathPlot.c_str();
    int ret = system(Dir);
#endif
}
