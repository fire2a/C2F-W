#!/bin/bash

cd ../Cell2FireC
pwd

#RUN CANADIAN FBP INSTANCE
./Cell2Fire.Linux.x86_64 --input-instance-folder ../data/CanadianFBP/400cellsC1 --output-instance-folder resutls/400cellsC1 --nsims 5 --out-messages --weather rows --sim C

#RUN KITRAL INSTANCE
#./Cell2Fire.Linux.x86_64 --input-instance-folder ../data/CanadianFBP/400cellsC1 --output-instance-folder resutls/400cellsC1 --nsims 5 --out-messages --weather rows --sim C

#RUN SCOTT AND BURGAN INSTANCE
#./Cell2Fire.Linux.x86_64 --input-instance-folder ../data/CanadianFBP/400cellsC1 --output-instance-folder resutls/400cellsC1 --nsims 5 --out-messages --weather rows --sim C
