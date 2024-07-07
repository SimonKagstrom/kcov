FROM alpine:3.20 AS builder

RUN apk add --update --no-cache \
        binutils-dev \
        # Debian: build-essential
        build-base \
        cmake \
        git \
        # Debian: libcurl4-openssl-dev
        curl-dev \
        curl-static \
        # Debian: libdw-dev
        libdw \
        # Debian, alpine bundled in binutils-dev
        # libiberty-dev \
        # Debian: libssl-dev
        openssl-dev \
        ninja-build \
        python3 \
        # Debian: zlib1g-dev
        zlib-dev \
        # Debian: libelf-dev
        elfutils-dev \
    ;

ADD . /src/

RUN mkdir /src/build && \
    cd /src/build && \
    export PATH="$PATH:/usr/lib/ninja-build/bin/" && \
    CXXFLAGS="-D__ptrace_request=int" cmake -G 'Ninja' .. && \
    cmake --build . && \
    cmake --build . --target install

FROM alpine:3.20

COPY --from=builder /usr/local/bin/kcov* /usr/local/bin/
COPY --from=builder /usr/local/share/doc/kcov /usr/local/share/doc/kcov

RUN set -eux; \
    apk add --update --no-cache \
        libcurl \
        libdw \
        zlib \
    ; \
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
