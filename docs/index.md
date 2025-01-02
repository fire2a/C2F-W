# Cell2Fire W

[![build & test debian-stable](https://github.com/fire2a/C2F-W/actions/workflows/build-debian-stable.yml/badge.svg)](https://github.com/fire2a/C2F-W/actions/workflows/build-debian-stable.yml)
[![build & test manylinux & ubuntu-jammy](https://github.com/fire2a/C2F-W/actions/workflows/build-manylinux.yml/badge.svg)](https://github.com/fire2a/C2F-W/actions/workflows/build-manylinux.yml)
[![build & test windows](https://github.com/fire2a/C2F-W/actions/workflows/build-windows.yml/badge.svg)](https://github.com/fire2a/C2F-W/actions/workflows/build-windows.yml)
[![build macos all](https://github.com/fire2a/C2F-W/actions/workflows/build-macos.yml/badge.svg)](https://github.com/fire2a/C2F-W/actions/workflows/build-macos.yml)
[![release](https://github.com/fire2a/C2F-W/actions/workflows/release.yml/badge.svg)](https://github.com/fire2a/C2F-W/actions/workflows/release.yml)
![License](https://img.shields.io/github/license/fire2a/C2F-W.svg)

Cell2Fire is a 2D-grid-based forest and wildland landscape fire simulator, focused on large scale areas and fast simulations to provide robust risk spatial analytics, harnessing C++ parallel computation methods.
This is the documentation for the command line version of the simulator, a [user-friendly graphical toolbox](https://fire2a.github.io/docs/docs/qgis-toolbox/README.html) is also available.

_Originally forked from [Cell2Fire](https://github.com/cell2fire/Cell2Fire), thanks to the work of Cristobal Pais, Jaime Carrasco, David Martell, [David L. Woodruff](https://github.com/DLWoodruff), Andres Weintraub, et al._

The current version supports 3 fire models:
* Kitral, formerly: [C2F+K](https://github.com/fire2a/C2FK)
* Scott&Burgan, formerly [C2F+S&B](https://github.com/fire2a/C2FSB) 
* Canadian Forest Fire Behavior Prediction System, OG [Cell2Fire](https://github.com/cell2fire/Cell2Fire)
  
As Cell2Fire: i) the landscape is characterized as a grid, in which every cell has the same size, and the necessary information to simulate fire spread, i.e. fuel type, elevation and ignition probability, among others; ii) exploits parallel computation methods which allows users to run large-scale simulations in short periods of time; iii) the fires spread within each cell is assumed to be elliptical; and iv) it includes powerful statistical, graphical output, and spatial analysis features to facilitate the display and analysis of projected fire growth.

Installation, usage, specifications available by [reading the friendly manual here](https://fire2a.github.io/docs/). 

[O leyendo el manual acá](https://fire2a.github.io/documentacion/)

# Usage (TL;DR)

Clone & [compile](https://fire2a.github.io/docs/docs/Cell2Fire/README.html)

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

Cell2Fire --input-instance-folder <input_data_directory> --output-folder <results_directory> --nsims <number of simulations> --sim <fuel model type>  


```
Fuel models that are currently supported are Kitral (K), Scott&Burgan (S) and Canadian Forest Fire Behavior Prediction System (C).

Other platform details at `.github/workflows/build-*.yml` and `makefile.*`

# About Us
We are a research group that seeks solutions to complex problems arising from the terrestrial ecosystem and its natural and anthropogenic disturbances, such as wildfires.

Currently hosted at [ISCI](https://isci.cl/) offices.

Contact us at [fire2a@fire2a.com](fire2a@fire2a.com).

Visit our [public webpage](http://www.fire2a.com/).

If you have questions you can join our public [Discord Server](https://discord.gg/Ev5GPmVkvZ).


