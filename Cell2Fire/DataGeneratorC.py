
# coding: utf-8
from argparse import ArgumentParser
import numpy as np
import pandas as pd
import os
pd.options.mode.chained_assignment = None


# Reads fbp_lookup_table.csv and creates dictionaries for the fuel types and cells' colors
def Dictionary(filename):
    aux = 1
    file = open(filename, "r") 
    row = {}
    colors = {} 
    all = {}
    
    # Read file and save colors and ftypes dictionaries
    for line in file: 
        if aux > 1:
            aux +=1
            line = line.replace("-","")
            line = line.replace("\n","")
            line = line.replace("No","NF")
            line = line.split(",")
            
            if line[3][0:3] in ["FM1"]:
                row[line[0]] = line[3][0:4]
            elif line[3][0:3] in ["Non", "NFn"]:
                row[line[0]] = "NF"
            else:    
                row[line[0]] = line[3][0:3]
                
            colors[line[0]] = (float(line[4]) / 255.0, 
                               float(line[5]) / 255.0,
                               float(line[6]) / 255.0,
                               1.0)
            all[line[0]] = line
    
        if aux == 1:
            aux +=1
            
    return row, colors

# Reads the ASCII file with the forest grid structure and returns an array with all the cells and grid dimensions nxm
# Modified Feb 2018 by DLW to read the forest params (e.g. cell size) as well
def ForestGrid(filename, Dictionary):
    with open(filename, "r") as f:
        filelines = f.readlines()

    line = filelines[4].replace("\n","")
    parts = line.split()
    
    if parts[0] != "cellsize":
        print ("line=",line)
        raise RuntimeError("Expected cellsize on line 5 of "+ filename)
    cellsize = float(parts[1])
    
    cells = 0
    row = 1
    trows = 0 
    tcols = 0
    gridcell1 = []
    gridcell2 = []
    gridcell3 = []
    gridcell4 = []
    grid = []
    grid2 = []
    
    # Read the ASCII file with the grid structure
    for row in range(6, len(filelines)):
        line = filelines[row]
        line = line.replace("\n","")
        line = ' '.join(line.split())
        line = line.split(" ")
        #print(line)
        
        
        for c in line: #range(0,len(line)-1):
            if c not in Dictionary.keys():
                gridcell1.append("NF")
                gridcell2.append("NF")
                gridcell3.append(int(0))
                gridcell4.append("NF")
            else:
                gridcell1.append(c)
                gridcell2.append(Dictionary[c])
                gridcell3.append(int(c))
                gridcell4.append(Dictionary[c])
            tcols = np.max([tcols,len(line)])

        grid.append(gridcell1)
        grid2.append(gridcell2)
        gridcell1 = []
        gridcell2 = []
    
    # Adjacent list of dictionaries and Cells coordinates
    CoordCells = np.empty([len(grid)*(tcols), 2]).astype(int)
    n = 1
    tcols += 1
    
    return gridcell3, gridcell4, len(grid), tcols-1, cellsize

# Reads the ASCII files with forest data elevation, saz, slope, and (future) curing degree and returns arrays
# with values
def DataGrids(InFolder, NCells):
    filenames = ["elevation.asc", "saz.asc", "slope.asc", "cur.asc", "cbd.asc", "cbh.asc", "ccf.asc","py.asc","fmc.asc"]
    Elevation =  np.full(NCells, np.nan)
    SAZ = np.full(NCells, np.nan)
    PS = np.full(NCells, np.nan)
    Curing = np.full(NCells, np.nan)
    CBD = np.full(NCells, np.nan)
    CBH = np.full(NCells, np.nan)
    CCF = np.full(NCells, np.nan)
    PY = np.full(NCells, np.nan)
    FMC=np.full(NCells,np.nan)
    
    for name in filenames:
        ff = os.path.join(InFolder, name)
        if os.path.isfile(ff) == True:
            aux = 0
            with open(ff, "r") as f:
                filelines = f.readlines()

                line = filelines[4].replace("\n","")
                parts = line.split()

                if parts[0] != "cellsize":
                    print ("line=",line)
                    raise RuntimeError("Expected cellsize on line 5 of "+ ff)
                cellsize = float(parts[1])

                row = 1

                # Read the ASCII file with the grid structure
                for row in range(6, len(filelines)):
                    line = filelines[row]
                    line = line.replace("\n","")
                    line = ' '.join(line.split())
                    line = line.split(" ")
                    #print(line)
                    
                    for c in line: 
                        if name == "elevation.asc":
                            Elevation[aux] = float(c)
                            aux += 1
                        if name == "saz.asc":
                            SAZ[aux] = float(c)
                            aux += 1
                        if name == "slope.asc":
                            PS[aux] = float(c)
                            aux += 1
                        if name == "cbd.asc":
                            CBD[aux] = float(c)
                            aux += 1
                        if name == "cbh.asc":
                            CBH[aux] = float(c)
                            aux += 1
                        if name == "ccf.asc":
                            CCF[aux] = float(c)
                            aux += 1
                        if name == "curing.asc":
                            Curing[aux] = float(c)
                            aux += 1
                        if name == "py.asc":
                            PY[aux] = float(c)
                            aux += 1
                        if name == "fmc.asc":
                            FMC[aux] = float(c)
                            aux += 1

        else:
            print("   No", name, "file, filling with NaN")
            
    return Elevation, SAZ, PS, Curing, CBD, CBH, CCF,PY,FMC

# Generates the Data.dat file (csv) from all data files (ready for the simulator)
def GenerateDat(GFuelType, GFuelTypeN, Elevation, PS, SAZ, Curing, CBD, CBH, CCF,PY,FMC, InFolder):
    # DF columns
    Columns = ["fueltype", "lat", "lon", "elev", "ws", "waz", "ps", "saz", "cur", "cbd", "cbh", "ccf","ftypeN","fmc","py"]
    
    # Dataframe
    DF = pd.DataFrame(columns=Columns)
    DF["fueltype"] = [x for x in GFuelType]
    DF["elev"] = Elevation
    DF["ps"] = PS
    DF["saz"] = SAZ
    DF["cbd"] = CBD
    DF["cbh"] = CBH
    DF["ccf"] = CCF
    DF["py"] = PY
    DF["fmc"] = FMC
    DF["lat"] = np.zeros(len(GFuelType)) + 51.621244
    DF["lon"] = np.zeros(len(GFuelType)).astype(int) - 115.608378
    
    # Populate fuel type number 
    DF["ftypeN"] = GFuelTypeN
    #print(np.asarray(GFuelTypeN).flatten())
    
    # Data File
    filename = os.path.join(InFolder, "Data.csv")
    print(filename)
    DF.to_csv(path_or_buf=filename, index=False, index_label=False, header=True)
    
    return DF

# Main function 
def GenDataFile(InFolder,Simulator):
    if Simulator=="K":
        FBPlookup=os.path.join(InFolder, "kitral_lookup_table.csv")
    elif Simulator=="S":
        FBPlookup = os.path.join(InFolder, "spain_lookup_table.csv")
    else: #beta version
        FBPlookup = os.path.join(InFolder, "spain_lookup_table.csv")

    FBPDict, ColorsDict =  Dictionary(FBPlookup)
    
    FGrid = os.path.join(InFolder, "fuels.asc")
    GFuelTypeN, GFuelType, Rows, Cols, CellSide = ForestGrid(FGrid, FBPDict)
    
    NCells = len(GFuelType)
    Elevation, SAZ, PS, Curing, CBD, CBH, CCF,PY,FMC = DataGrids(InFolder, NCells)
    GenerateDat(GFuelType, GFuelTypeN, Elevation, PS, SAZ, Curing, CBD, CBH, CCF,PY,FMC, InFolder)
