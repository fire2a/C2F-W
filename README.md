# Cell2Fire W

[![build & test debian-stable](https://github.com/fire2a/C2F-W/actions/workflows/build-debian-stable.yml/badge.svg)](https://github.com/fire2a/C2F-W/actions/workflows/build-debian-stable.yml)
[![build & test manylinux & ubuntu-jammy](https://github.com/fire2a/C2F-W/actions/workflows/build-manylinux.yml/badge.svg)](https://github.com/fire2a/C2F-W/actions/workflows/build-manylinux.yml)
[![build & test windows](https://github.com/fire2a/C2F-W/actions/workflows/build-windows.yml/badge.svg)](https://github.com/fire2a/C2F-W/actions/workflows/build-windows.yml)
[![build macos all](https://github.com/fire2a/C2F-W/actions/workflows/build-macos.yml/badge.svg)](https://github.com/fire2a/C2F-W/actions/workflows/build-macos.yml)
[![release](https://github.com/fire2a/C2F-W/actions/workflows/release.yml/badge.svg)](https://github.com/fire2a/C2F-W/actions/workflows/release.yml)
![License](https://img.shields.io/github/license/fire2a/C2F-W.svg)

A big-scale, grid, forest fire simulator; parallel and fast (c++) nevertheless with a friendly graphical user interface for QGIS.

_Originally forked from [Cell2Fire](https://github.com/cell2fire/Cell2Fire), thanks to the work of Cristobal Pais, Jaime Carrasco, David Martell, [David L. Woodruff](https://github.com/DLWoodruff), Andres Weintraub, et al._

This fire spread simulator, supports 3 fire models:
* Kitral, formerly: [C2F+K](https://github.com/fire2a/C2FK)
* Scott&Burgan, formerly [C2F+S&B](https://github.com/fire2a/C2FSB) 
* Canadian Forest Fire Behavior Prediction System, OG [Cell2Fire](https://github.com/cell2fire/Cell2Fire)
  
As Cell2Fire: i) the landscape is characterized as a grid, in which every cell has the same size, and the necessary information to simulate fire spread, i.e. fuel type, elevation and ignition probability, among others; ii) exploits parallel computation methods which allows users to run large-scale simulations in short periods of time; iii) the fires spread within each cell is assumed to be elliptical; and iv) it includes powerful statistical, graphical output, and spatial analysis features to facilitate the display and analysis of projected fire growth.

Installation, usage, specifications available by [reading the friendly manual here](https://fire2a.github.io/docs/). 

[O leyendo el manual acá](https://fire2a.github.io/documentacion/)

# Usage (TL;DR)

- [Easy](https://fire2a.github.io/docs/docs/qgis-toolbox/README.html#usage-alternatives): Use through QGIS desktop application
- [Moderate: CLI](#moderate-cli): Download a release, unzip, and run the CLI
- [Collaborative](##collaborative): Clone & [compile](https://fire2a.github.io/docs/docs/Cell2Fire/README.html)

# Output examples
## Previncat's Zone 60 (Catalonian Instance): forest and a simulated fire spread with its corresponding scar and growth propagation tree. 
![Example-Instance_Scar](output/example-scar.png)
## Risk metrics: Burn Probability (BP), Betweenness Centrality (BC), Downstream Protection Value (DPV), and Growth Propagation Tree (GPT). 
![Example-Risk_Metrics](output/example-metrics.png)

## Easy: Graphical User Interface for QGIS
1. Install QGIS >= LTR version
2. Add [this](https://fire2a.github.io/fire-analytics-qgis-processing-toolbox-plugin/plugins.xml) as plugin server repo in QGIS
3. Pip install python dependendiencies to use other related algorithms such as risk metrics and landscape decision optimization
4. Install the plugin from the QGIS plugin manager
5. Use the plugin from the processing toolbox: Use the instance downloader to get ready instance, and then simulate your first wildfire!

Full guide, how to [fire2a.io/docs](https://fire2a.github.io/docs/)
Plugin repo: [fire2a/fire-analytics-qgis-processing-toolbox-plugin](https://github.com/fire2a/fire-analytics-qgis-processing-toolbox-plugin) repo

## Moderate: CLI 
Go to releases (right vertical tab bar):
1. Download the latest (non-draft) release version of the binary, choose platform:

    - Microsoft Windows (10 or 11) also includes .dll dependencies  
        `Cell2FireW_v<x.y.z>-Windows-x86_64-binary.zip`
    - Linux, targeting distribution-codename and dependent runtime libraries:  
        `Cell2FireW_v<x.y.z>-Debian.bookworm.x86_64-binary.zip` needs `apt install libtiff6 libgomp1`  
        `Cell2FireW_v<x.y.z>-Ubuntu.jammy.x86_64-binary.zip` needs `apt install libtiff5 libstdc++6 libgomp1 libgcc1`  
    - Most other linuxes, no dependencies. _Although it may not run parallel!_  
        `Cell2FireW_v<x.y.z>-manylinux-x86_64-binary.zip`  
    - Contains them all and more, is meant for the QGIS plugin    
        `Cell2FireW_v<x.y.z>.zip` 

2. Jumpstart into downloading a prepared instance, choosing:

   - Fuel model: `Kitral`, `Scott&Burgan`, or `FBP-Canada` (--sim <K|S|C>)
   - Raster format: GeoTiff`.tif` or AIIGrid`.asc`

3. Open a terminal, type the following:
```bash
# [once] make it executable
chmod +x </path/to/Cell2FireW/binary>/Cell2Fire[.suffix]
# [optional] make it accessible from other directories
export PATH=$PATH:/path/to/Cell2FireW/binar/Cell2Fire[.suffix]

# needs an empty directory to store the results
mkdir results 

# run idea
Cell2Fire[.exe|Debian.boookworm|...] --input-instance-folder </path/to/instance> --output-folder </path/to/empty/results> --sim <K|S|C> --nthreads 16 > log.txt

# example
Cell2Fire --final-grid --output-messages --out-ros --sim S --nsims 2 --seed 123 --nthreads 3 --fmc 66 --scenario 2 --cros --weather rows --input-instance-folder /tmp/processing_cdcCGk/Vilopriu_2013 --output-folder /tmp/processing_cdcCGk/Vilopriu_2013/results | tee log.txt

# check the results: to convert to tiff or see the results in QGIS, use the plugin
```
## Collaborative
Compile it
```bash
# dependencies
sudo apt install g++-12 libboost-all-dev libeigen3-dev libtiff-dev
# or brew
brew install gcc@12 libomp eigen boost libtiff # llvm ?

# fork & clone 
git clone git@github.com:<YOU>/C2F-W.git
cd C2F-W/Cell2Fire

# compile
make 
# there area other makefiles for other platforms, e.g. makefile.macos

# [optional] copies Cell2Fire to /usr/local/bin
sudo make install  
```
Other platform details at `.github/workflows/build-*.yml` and `makefile.*`
