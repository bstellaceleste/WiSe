
FROM ubuntu:22.04

RUN apt-get update && apt-get install -y \
    git cmake gcc g++ make python3 perl zlib1g-dev

RUN git clone https://github.com/DynamoRIO/dynamorio.git && \
    cd dynamorio && \
    git submodule update --init --recursive && \
    mkdir build && cd build && \
    cmake .. -DBUILD_DOCS=OFF -DBUILD_TESTS=OFF && \
    make -j$(nproc)

WORKDIR /app
COPY . .
RUN make all

CMD ["make", "test"]
