FROM debian:stable
LABEL authors="matilde"

ENV DEBIAN_FRONTEND=noninteractive
RUN apt-get update && apt-get install -y g++-12 libboost-all-dev libeigen3-dev libtiff-dev git make

RUN git clone https://github.com/fire2a/C2F-W.git

RUN cd C2F-W/Cell2Fire && make

RUN chmod +x C2F-W/Cell2Fire/Cell2Fire

ENTRYPOINT ["C2F-W/Cell2Fire/Cell2Fire"]