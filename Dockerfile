FROM debian:stable
LABEL authors="matilde"

RUN apt-get update && apt-get install -y g++-12 libboost-all-dev libeigen3-dev libtiff-dev git cmake unzip
RUN git clone https://github.com/fire2a/C2F-W.git

WORKDIR ${HOME}/C2F-W
RUN cd Cell2Fire && make

COPY entrypoint.sh /entrypoint.sh

ENTRYPOINT ["/bin/bash", "/entrypoint.sh"]

#--input-instance-folder model/kitral-asc --output-folder test_results/$model-$format --nsims 113
#--output-messages --grids --out-ros --out-intensity --sim K --seed 123
#--ignitionsLog > test/test_results/kitral-asc/log.txt