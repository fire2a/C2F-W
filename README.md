# C2F-W
##  Beta version by David Palacios M.
C2F-W is a joint version of the open-source wildfire simulator Cell2Fire developed by Jaime Carrasco, Cristobal Pais and David Woodroof. Currently supports Kitral and Scott & Burgan fuel models

# Disclaimer
This software is for research use only. There is no warranty of any kind; there is not even the implied warranty of fitness for use.

# Introduction
C2F-W is a joint version of the fire spread simulators [C2F+K](https://github.com/fire2a/C2FK) and [C2F+S&B](https://github.com/fire2a/C2FSB), both based upon [Cell2Fire](https://github.com/cell2fire/Cell2Fire). As Cell2Fire, i) the landscape is characterized as a grid, in which every cell has the same size, and the necessary information to simulate fire spread, i.e. fuel type, elevation and ignition probability, among others; ii) exploits parallel computation methods which allows users to run large-scale simulations in short periods of time; iii) the fires spread within each cell is assumed to be elliptical; and iv) it includes powerful statistical, graphical output, and spatial analysis features to facilitate the display and analysis of projected fire growth.

Work in progress documentation is available at [readthedocs](https://fdobad.github.io/docs/).

# Installation
Installation may require some familiarity with C++, make, and Python.
* cd C2F-W/Cell2FireC
* (edit Makefile to have the correct path to Eigen)
* make
* cd .. 
* pip install -r requirements.txt

# Requirements
- Boost (C++)
- Eigen (C++)
- Python 3.6
- numpy
- pandas
- matplotlib
- seaborn
- tqdm
- rasterio
- networkx (for stats module)

# Usage
In order to run the simulator and process the results, the following command can be used:
```
$ python main.py --input-instance-folder /data/Vilopriu_2013/ --output-folder results/test --sim-years 1 --nsims 10 --grids --weather rows --nweathers 1 --Fire-Period-Length 1.0 --output-messages --ROS-CV 0.8 --seed 123 --stats --allPlots --sim S
```
For the full list of arguments and their explanation use:
```
$ python main.py -h
```
A tutorial can be found at [C2F+S&B Tutorial](https://github.com/fire2a/C2FSB/blob/main/C2FS%26B_Tutorial.pdf)

# Simulator option
To use the correspondant version of the simulator, choice version with the following instruction:
```
$ --sim XXX
```
Where XXX can be *S*, for Scott and Burgan Fuel Model, or *K*, for Kitral Fuel Model

# Output examples
## Previncat's Zone 60 (Catalonian Instance): forest and a simulated fire spread with its corresponding scar and growth propagation tree. 
![Example-Instance_Scar](Output/example-scar.png)
## Risk metrics: Burn Probability (BP), Betweenness Centrality (BC), Downstream Protection Value (DPV), and Growth Propagation Tree (GPT). 
![Example-Risck_Metrics](Output/example-metrics.png)
