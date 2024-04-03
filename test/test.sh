#!/bin/bash
cd ../Cell2FireC
pwd

#RUN CANADIAN FBP INSTANCE
./Cell2Fire.Linux.x86_64 --input-instance-folder ../test/FBP_Instance --output-folder ../test/test_results/FBP --sim-years 1 --nsims 100 --output-messages --out-ros --out-fl --out-intensity --grids --sim C

#RUN KITRAL INSTANCE
./Cell2Fire.Linux.x86_64 --input-instance-folder ../test/Kitral_Instance --output-folder ../test/test_results/Kitral --sim-years 1 --nsims 100 --output-messages --out-ros --out-fl --out-intensity --grids --sim K

#RUN SCOTT AND BURGAN INSTANCE
./Cell2Fire.Linux.x86_64 --input-instance-folder ../test/SB_Instance --output-folder ../test/test_results/SB --sim-years 1 --nsims 100 --output-messages --out-ros --out-fl --out-intensity --grids --sim S