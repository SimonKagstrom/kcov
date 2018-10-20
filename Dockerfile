FROM debian:stable AS builder

RUN apt-get update && \
    apt-get install -y --fix-missing pkg-config && \
    apt-get install -y \
        binutils-dev \
        build-essential \
        cmake \
        git \
        libcurl4-openssl-dev \
        libdw-dev \
        libiberty-dev \
        ninja-build \
        python \
        zlib1g-dev \
        ;

ADD . /src/

RUN mkdir -p /src/build && \
    cd /src/build && \
    cmake -G 'Ninja' .. && \
    cmake --build . && \
    cmake --build . --target install

# ensure we don't copy any unneeded libs into final image
RUN apt-get purge -y \
        binutils-dev \
        build-essential \
        cmake \
        git \
        libcurl4-openssl-dev \
        libdw-dev \
        libiberty-dev \
        ninja-build \
        python \
        zlib1g-dev \
        && \
    apt-get autoremove -y && \
    apt-get autoclean -y

RUN apt-get update && \
    apt-get install -y \
        binutils \
        libcurl3 \
        libdw1 \
        zlib1g \
        ;

FROM debian:stable-slim
COPY --from=builder /lib/x86_64-linux-gnu/*.so* /lib/x86_64-linux-gnu/
COPY --from=builder /usr/lib/x86_64-linux-gnu/*.so* /usr/lib/x86_64-linux-gnu/
COPY --from=builder /usr/local/bin/kcov* /usr/local/bin/
COPY --from=builder /usr/local/share/doc/kcov /usr/local/chare/doc/kcov

CMD ["/usr/local/bin/kcov"]
