FROM debian:stable-slim

RUN apt-get update && \
    apt-get install -y --no-install-recommends \
        g++ \
        make \
        libboost-all-dev \
        && rm -rf /var/lib/apt/lists/*

WORKDIR /workspace

COPY split_any_of_compare.cpp .

CMD bash -c "g++ -std=c++17 -O2 split_any_of_compare.cpp -o split_any_of_compare && ./split_any_of_compare"
