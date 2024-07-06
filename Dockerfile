FROM debian:bookworm-slim AS builder

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

FROM debian:bookworm-slim

COPY --from=builder /usr/local/bin/kcov* /usr/local/bin/
COPY --from=builder /usr/local/share/doc/kcov /usr/local/share/doc/kcov

RUN set -eux; \
    apt-get update; \
    apt-get install -y --no-install-recommends --no-install-suggests \
        libcurl4 \
        libdw1 \
        zlib1g \
    ; \
    apt-get clean; \
    rm -rf /var/lib/apt/lists/*; \
    # Write a test script
    echo '#!/usr/bin/env bash\nif [[ true ]]; then\necho "Hello, kcov!"\nfi' > /tmp/test-executable.sh; \
    # Test kcov
    kcov --include-pattern=/tmp/test-executable.sh /tmp/coverage /tmp/test-executable.sh; \
    # Display
    cat /tmp/coverage/test-executable.sh/coverage.json; \
    # Make assertions
    grep -q -c '"percent_covered": "100.00"' /tmp/coverage/test-executable.sh/coverage.json; \
    grep -q -c '"total_lines": 2' /tmp/coverage/test-executable.sh/coverage.json; \
    # Cleanup
    rm -r /tmp/coverage /tmp/test-executable.sh;

CMD ["/usr/local/bin/kcov"]
