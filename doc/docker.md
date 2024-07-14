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
