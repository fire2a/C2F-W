# C2F+S&B
## Jaime Carrasco, Cristóbal Pais, José Ramón González, Jordi Garcia, David Palacios, Rodrigo Mahaluf, and Andrés Weintraub
C2F+S&B is an open-source wildfire simulator based on Cell2Fire and the Scott And Burgan Fuel Models

# Disclaimer
This software is for research use only. There is no warranty of any kind; there is not even the implied warranty of fitness for use.

# Introduction
C2F+S&B is a version of the fire spread simulator [Cell2Fire](https://github.com/cell2fire/Cell2Fire) for Scott and Burgan fuel system. As Cell2Fire, i) the landscape is characterized as a grid, in which every cell has the same size, and the necessary information to simulate fire spread, i.e. fuel type, elevation and ignition probability, among others; ii) exploits parallel computation methods which allows users to run large-scale simulations in short periods of time; iii) the fires spread within each cell is assumed to be elliptical; and iv) it includes powerful statistical, graphical output, and spatial analysis features to facilitate the display and analysis of projected fire growth.

Unlike Cell2Fire, the fire spread is governed by spread rates predicted by the Scott and Burgan fuel system, and uses its weathers scenarios to take into account the moisture of fine and dead particles. 

Work in progress documentation is available at [readthedocs](https://cell2fire.readthedocs.io/en/latest/) and there is an original draft of a paper on [frontiers](https://www.frontiersin.org/articles/10.3389/ffgc.2023.1071484/full).

# Installation
Installation may require some familiarity with C++, make, and Python.
* cd C2FSB/Cell2FireC
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
$ python main.py --input-instance-folder /data/Vilopriu_2013/ --output-folder results/test --sim-years 1 --nsims 10 --grids --weather rows --nweathers 1 --Fire-Period-Length 1.0 --output-messages --ROS-CV 0.8 --seed 123 --stats --allPlots
```
For the full list of arguments and their explanation use:
```
$ python main.py -h
```
A tutorial can be found at [C2F+S&B Tutorial](https://github.com/fire2a/C2FSB/blob/main/C2FS%26B_Tutorial.pdf)

# Console usage (on development)
To use the simulator with a console (via python), the following command can be used:
```
$ python window.py
```
Important: not all arguments are implemented on the console

# Output examples
## Previncat's Zone 60 (Catalonian Instance): forest and a simulated fire spread with its corresponding scar and growth propagation tree. 
![Example-Instance_Scar](Output/example-scar.png)
## Risk metrics: Burn Probability (BP), Betweenness Centrality (BC), Downstream Protection Value (DPV), and Growth Propagation Tree (GPT). 
![Example-Risck_Metrics](Output/example-metrics.png)
