ARG BUILD_OS
ARG BUILD_BASE

FROM ${BUILD_OS}:${BUILD_BASE} AS builder

ARG BUILD_OS
ARG BUILD_BASE

RUN set -eux; \
    echo "Building for: ${BUILD_OS}"; \
    if [ "${BUILD_OS}" = "alpine" ]; then \
    apk add --update --no-cache \
        binutils-dev \
        build-base \
        cmake \
        git \
        curl-dev \
        curl-static \
        libdw \
        openssl-dev \
        ninja-build \
        python3 \
        zlib-dev \
        elfutils-dev \
        libstdc++-dev \
    ; \
    elif [ "${BUILD_OS}" = "debian" ]; then \
    apt-get update; \
    apt-get install -y --no-install-recommends --no-install-suggests \
        binutils-dev \
        build-essential \
        cmake \
        git \
        libcurl4-openssl-dev \
        libdw-dev \
        # Alpine: bundled in binutils-dev
        libiberty-dev \
        libssl-dev \
        ninja-build \
        python3 \
        zlib1g-dev \
        libelf-dev \
        libstdc++-12-dev \
    ; \
    fi

ADD . /src/

RUN set -eux; \
    mkdir /src/build; \
    cd /src/build; \
    if [ "${BUILD_OS}" = "alpine" ]; then \
        export PATH="$PATH:/usr/lib/ninja-build/bin/"; \
    fi; \
    cmake -G 'Ninja' ..; \
    cmake --build .; \
    cmake --build . --target install;

ARG BUILD_OS
ARG BUILD_BASE

FROM ${BUILD_OS}:${BUILD_BASE}

ARG BUILD_OS
ARG BUILD_BASE

COPY --from=builder /usr/local/bin/kcov* /usr/local/bin/
COPY --from=builder /usr/local/share/doc/kcov /usr/local/share/doc/kcov

RUN set -eux; \
    echo "Building for: ${BUILD_OS}"; \
    if [ "${BUILD_OS}" = "alpine" ]; then \
    apk add --update --no-cache \
        bash \
        libcurl \
        libdw \
        zlib \
        libgcc \
        libstdc++ \
    ; \
    elif [ "${BUILD_OS}" = "debian" ]; then \
    apt-get update; \
    apt-get install -y --no-install-recommends --no-install-suggests \
        libcurl4 \
        libdw1 \
        zlib1g \
    ; \
    apt-get clean; \
    rm -rf /var/lib/apt/lists/*; \
    fi; \
    # Write a test script
    printf '#!/usr/bin/env bash\nif [[ true ]]; then\necho "Hello, kcov!"\nfi' > /tmp/test-executable.sh; \
    # Test kcov
    kcov --include-pattern=/tmp/test-executable.sh /tmp/coverage /tmp/test-executable.sh; \
    # Display
    cat /tmp/coverage/test-executable.sh/coverage.json; \
    # Make assertions
    grep -q -c '"percent_covered": "100.00"' /tmp/coverage/test-executable.sh/coverage.json; \
    grep -q -c '"total_lines": 2' /tmp/coverage/test-executable.sh/coverage.json; \
    # Cleanup
    rm -r /tmp/coverage /tmp/test-executable.sh;

# Metadata
LABEL org.label-schema.vendor="Kcov" \
    org.label-schema.url="https://github.com/SimonKagstrom/kcov#readme" \
    org.label-schema.name="The docker image for kcov" \
    org.label-schema.description="A docker image for kcov" \
    org.label-schema.version=${RELEASE_VERSION} \
    org.label-schema.vcs-url="https://github.com/SimonKagstrom/kcov.git" \
    org.label-schema.vcs-ref=${VCS_REF} \
    org.label-schema.build-date=${BUILD_DATE} \
    org.label-schema.docker.schema-version="1.0" \
    \
    com.docker.extension.publisher-url="https://github.com/SimonKagstrom" \
    \
    org.opencontainers.image.title="The docker image for kcov" \
    org.opencontainers.image.description="A docker image for kcov" \
    org.opencontainers.image.authors="https://github.com/SimonKagstrom" \
    org.opencontainers.image.url="https://github.com/SimonKagstrom/kcov#readme" \
    org.opencontainers.image.documentation="https://github.com/SimonKagstrom/kcov#readme" \
    org.opencontainers.image.source="https://github.com/SimonKagstrom/kcov" \
    org.opencontainers.image.vendor="Kcov" \
    org.opencontainers.image.licenses="GPL-2.0" \
    org.opencontainers.image.created=${BUILD_DATE} \
    org.opencontainers.image.version=${RELEASE_VERSION} \
    org.opencontainers.image.revision=${VCS_REF}

CMD ["/usr/local/bin/kcov"]
