# The docker image

Kcov has a docker image: [kcov/kcov](https://hub.docker.com/r/kcov/kcov/) and tags since v31.

There is two variants (tags):
- `latest` or `vXX` that are Debian based
- `latest-alpine` or `vXX-alpine` that are Alpine based

## Permissions

kcov needs access the following system calls:
- [`ptrace`](https://linux.die.net/man/2/ptrace)
- [`process_vm_readv`](https://linux.die.net/man/2/process_vm_readv)/[`process_vm_writev`](https://linux.die.net/man/2/process_vm_writev)
- [`personality`](https://linux.die.net/man/2/personality)

You may need to use `--security-opt seccomp=unconfined` as a docker run option to attach to processes.

## Copy into your image

You may want to copy kcov into your image to avoid building it.

```Dockerfile
# Copy kcov (use kcov/kcov:latest-alpine for Alpine based images)
COPY --from=kcov/kcov:latest /usr/local/bin/kcov* /usr/local/bin/
# If you need documentation
COPY --from=kcov/kcov:latest /usr/local/share/doc/kcov /usr/local/share/doc/kcov
```

### Python + bats example

```Dockerfile
# The base layer of your image
FROM python:3.11-slim-bookworm

# Install kcov run-time dependencies and bats.
RUN apt-get update && \
    apt-get install --yes --no-install-suggests --no-install-recommends \
      libbfd-dev \
      libcurl4 \
      libdw1 \
      zlib1g \
      bats \
      && \
    apt-get clean && \
    rm -rf /var/lib/apt/lists/*

# Copy kcov (use kcov/kcov:latest-alpine for Alpine based images)
COPY --from=kcov/kcov:latest /usr/local/bin/kcov* /usr/local/bin/
COPY --from=kcov/kcov:latest /usr/local/share/doc/kcov /usr/local/share/doc/kcov

WORKDIR /code

CMD ["bats"]
```

#### Test it

##### `test.sh`

```sh
#!/usr/bin/env bats

source script.sh

@test "test outputNumber function" {
  result="$( outputNumber )"
  [ "$result" -eq 7 ]
}

@test "kcov version" {
  kcov --version | grep -q -c -F "v"
  [ "$?" -eq 0 ]
}
```

##### `script.sh`

```sh
outputNumber() {
  echo 7
}
```

##### Run tests

```sh
# build our image
docker build ./ -t py-bats
# test the built image
docker run --rm -it -v $PWD:/code py-bats kcov --include-path=/code --dump-summary ./coverage bats ./test.sh
# See the coverage result in ./coverage
# Browse the file coverage/index.html in your browser
```
