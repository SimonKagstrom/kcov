FROM debian:bullseye-slim AS builder

RUN apt-get update && \
    apt-get install -y \
        binutils-dev \
        build-essential \
        cmake \
        git \
        libcurl4-openssl-dev \
        libdw-dev \
        libiberty-dev \
    libssl-dev \
        ninja-build \
        python3 \
        zlib1g-dev \
        ;

ADD . /src/

RUN mkdir /src/build && \
    cd /src/build && \
    cmake -G 'Ninja' .. && \
    cmake --build . && \
    cmake --build . --target install

FROM debian:bullseye-slim

RUN apt-get update && \
    apt-get install -y \
        binutils \
        libcurl4 \
        libdw1 \
        zlib1g \
        && \
    apt-get clean && \
    rm -rf /var/lib/apt/lists/*

COPY --from=builder /usr/local/bin/kcov* /usr/local/bin/
COPY --from=builder /usr/local/share/doc/kcov /usr/local/share/doc/kcov

CMD ["/usr/local/bin/kcov"]
