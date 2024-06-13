# latchy
Reimplementation of clevis, supporting only decryption and tang, as a binary. See [clevis](https://github.com/latchset/clevis)

## Purpose
This project was created for the following reasons:
* Remove the dependency on bash and support distro-less containers
* Add the ability to retry the tang /rec api call untill success
* Output the PT to a named pipe.

### Dependencies
The library requires a number of external module and are directly included herein via git's submodule OR via vcpkg (which itself is included as a submodule).

### Compiling using the local tool chain
```bash
make bootstrap    <-- Only on first time
make clean        <-- Not necessary when the first time
make configure    <-- First time, or if you change any of the cmake related files.
make build
```
### Cross-compiling via an Alpine container
This will produce a fully static bianry based on the MUSL tool-chain.

```bash
 make buildImage
```

You can also create a local docker image using (which will implicitely call buildImage)

```bash
make packageDocker
```