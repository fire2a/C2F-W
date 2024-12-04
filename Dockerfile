FROM debian:stable
LABEL authors="matilde"

RUN apt-get update && apt-get install -y g++-12 libboost-all-dev libeigen3-dev libtiff-dev git cmake unzip
RUN git clone https://github.com/fire2a/C2F-W.git
#RUN cd C2F-W/Cell2Fire && make
WORKDIR ${HOME}/C2F-W
RUN cd Cell2Fire && make
