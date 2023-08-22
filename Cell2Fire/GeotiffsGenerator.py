# -*- coding: utf-8 -*-
"""
Created on Sun May 29 05:56:30 2022

@author: Rodrigo Mahaluf Recasens
"""

import glob
import rasterio
import os
import numpy as np
from matplotlib import pyplot

def InputGeotiff(InFolder):
    # creatting a list with the ascii files in the data directory
    #files = glob.glob(InFolder+"*.asc")
    files=glob.glob(os.path.join(InFolder+'*.asc'))

    # Sorting the list of files, so that they always have the same order
    files.sort()
    
    # getting the meta data from the first file
    src = rasterio.open(files[0])
    meta = src.meta
    meta["dtype"]="float32"
    # updating the meta data to set the correct number of bands
    meta.update(count=len(files))
    meta.update(driver="GTiff")
    # writting every file into a band into the same file

    with rasterio.open(os.path.join(InFolder+"Input_Geotiff.tif"), "w", **meta) as dst:
        for id, layer in enumerate(files, start=1):
            with rasterio.open(layer) as src:
                dst.write(src.read(1), id)
    src.close()
#dst.descriptions
#dst.met
def OutputGeotiff(InFolder,OutFolder,Nsims):
    input_tif = rasterio.open(os.path.join(InFolder+"Input_Geotiff.tif"))
    meta=input_tif.meta
    input_tif.close()
    metrics_folder=["Intensity","RateOfSpread","CrownFire","Grids"]
    metrics_name=["Intensity","ROSFile","Crown","Grids"]
    for metric_counter in range(len(metrics_folder)):
        metrics_folder[metric_counter]=os.path.join(OutFolder,metrics_folder[metric_counter])
    data_dictionary={}
    #os.getthreads() POR DEFECTO EN ESE NUMERO-1
    for i in range(1,Nsims+1): ###EN PARALELO
        tif_dictionary={}
        id_counter=0
        geotiffs_folder=os.path.join(OutFolder,"Geotiffs")
        if i==1 and not os.path.exists(geotiffs_folder):
            os.mkdir(geotiffs_folder)
        for j in range(len(metrics_folder)):
            if os.path.exists(metrics_folder[j]):
                if metrics_name[j]!="Grids":
                    id_counter,tif_dictionary,data_dictionary=GenerateAsciiTif(metrics_folder[j], i, metrics_name[j], id_counter, tif_dictionary, data_dictionary)
                else:                    
                    id_counter,tif_dictionary,data_dictionary=GenerateGridsTif(metrics_folder[j], i, metrics_name[j], id_counter, tif_dictionary, data_dictionary)
        meta.update(count=len(metrics_folder)) 
        meta.update(driver="GTiff")
        with rasterio.open(os.path.join(geotiffs_folder,"Out_Geotiff_"+str(i)+".tif"), "w", **meta) as dst:
            for key in tif_dictionary:
                dst.write(tif_dictionary[key], key)
                dst.set_band_description(key,metrics_name[key-1])
        meta_hourly_grids=meta
        hours=len(data_dictionary[i,"Grids"])
        meta_hourly_grids.update(count=hours)
        with rasterio.open(os.path.join(geotiffs_folder,"Hourly_Grids_"+str(i)+".tif"), "w", **meta_hourly_grids) as dst:
            for id, key in enumerate(data_dictionary[i,"Grids"], start=1):
                dst.write(key,id)
                dst.set_band_description(id,f"Grids_{id}")
    for i in range(len(metrics_folder)):
        if os.path.exists(metrics_folder[i]):
            GenerateSummaryTif(OutFolder,metrics_name[i],data_dictionary,meta,Nsims)

def GenerateAsciiTif(file,i,metric,id_counter,tif_dictionary,data_dictionary,metadata=False): #return metadata para extraer aca la metadata del input
    metric_file=metric+"0"+str(i)+".asc" if i<10 else metric+str(i)+".asc"
    layer_file=os.path.join(file,metric_file)
    layer=rasterio.open(layer_file)      
    numpyobject=layer.read(1)
    layer.close()
    id_counter=id_counter+1
    tif_dictionary[id_counter]=numpyobject
    data_dictionary[i,metric]=numpyobject
    return id_counter,tif_dictionary,data_dictionary

def GenerateGridsTif(file,i,metric,id_counter,tif_dictionary,data_dictionary,metadata=False,setMaxGrids=1000000): #return metadata para extraer aca la metadata del input
    folder_grid=os.path.join(file,"Grids"+str(i))      
    grids=os.listdir(folder_grid)
    id_counter+=1
    max_grid=0
    hours=len(grids)
    data_dictionary[i,metric]=[[]]*hours
    #if len(grids)>setMaxGrids: #FOR SETTING MAX GRIDS
     #   new_list=list(range(1,len(grids),int(grids/setMaxGrids)))
      #  str_new_list=[str(value) for value in new_list]
       # grids=[grid for grid in grids if any(x in str_new_list for x in grid)]
    for csvgrid in grids:
        grid_number=""
        for value in csvgrid:
            if value.isnumeric():
                grid_number=grid_number+value
        grid_layer=os.path.join(folder_grid,csvgrid)
        grid_layer=open(grid_layer)
        grid_array=np.loadtxt(grid_layer, delimiter=",")
        data_dictionary[i,metric][int(grid_number)]=grid_array
        if int(grid_number)>max_grid:
            tif_dictionary[id_counter]=grid_array
            max_grid=int(grid_number)

    return id_counter,tif_dictionary,data_dictionary

def GenerateSummaryTif(OutFolder,metric,data_dictionary,meta,nsims):
    mean_layer= np.mean(np.array([data_dictionary[i,metric] for i in range(1,nsims+1)]),axis=0) if metric!="Grids" else np.mean(np.array([data_dictionary[i,metric][-1] for i in range(1,nsims+1)]),axis=0)
    std_layer=np.std(np.array([data_dictionary[i,metric] for i in range(1,nsims+1)]),axis=0) if metric!="Grids" else np.std(np.array([data_dictionary[i,metric][-1] for i in range(1,nsims+1)]),axis=0)
    min_layer=np.min(np.array([data_dictionary[i,metric] for i in range(1,nsims+1)]),axis=0) if metric!="Grids" else np.min(np.array([data_dictionary[i,metric][-1] for i in range(1,nsims+1)]),axis=0)
    max_layer=np.max(np.array([data_dictionary[i,metric] for i in range(1,nsims+1)]),axis=0) if metric!="Grids" else np.max(np.array([data_dictionary[i,metric][-1] for i in range(1,nsims+1)]),axis=0)
    meta.update(count=4)
    with rasterio.open(os.path.join(OutFolder,metric+"_Summary.tif"), "w", **meta) as dst:
        dst.write(mean_layer, 1)
        dst.set_band_description(1,'mean')
        dst.write(std_layer,2)
        dst.set_band_description(2,"std")
        dst.write(min_layer,3)
        dst.set_band_description(3,"min")
        dst.write(max_layer,4)
        dst.set_band_description(4,"max")
        dst.close()
#def CsvToGeotiff(InFolder,OutFolder,Nsims,file): #take a csv file and transform to Geotiff
 #   return 0
#test="C:\\Users\\david\\projects\\Cell2Fire_Spain\\data\\Vilopriu_2013_40x40\\cbd.asc"
#output=openrasterio(test)
#InFolder="C:\\Users\\david\\projects\\Cell2Fire_Spain\\data\\Vilopriu_2013_40x40\\"
#print(output)
#test2="C:\\Users\\david\\projects\\Cell2Fire_Spain\\results\\Vilopriu"
#InputGeotiff(InFolder)
#InFolder="D:\\Cell2Fire\\Cell2Fire_Spain_Linux\\data\\Hom_Fuel_101_40x40\\"
#OutFolder="D:\\Cell2Fire\\Cell2Fire_Spain_Linux\\results\\tifff\\"
#OutputGeotiff(InFolder, OutFolder, 10)
#tif=rasterio.open("C:\\Users\\david\\projects\\Cell2Fire_Spain\\results\\Vilopriu\\Geotiffs\\Out_Geotiff_3.tif")
#tif=rasterio.open("C:\\Users\\david\\projects\\Cell2Fire_Spain\\results\\Vilopriu\\Geotiffs\\Hourly_Grids_5.tif")
#tif=rasterio.open("C:\\Users\\david\\projects\\Cell2Fire_Spain\\results\\Vilopriu\\ROSFile_Summary.tif")
#tif=rasterio.open("D:\\Cell2Fire\\Cell2Fire_Spain_Linux\\results\\tifff\\Geotiffs\\Out_Geotiff_6.tif")
#print(tif.descriptions)
#tif.close()
#pyplot.imshow(tif.read(2), cmap='pink')#,vmin=0,vmax=185)
