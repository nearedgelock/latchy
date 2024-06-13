#!/bin/bash

# For compiling on Alpine and produce a fully static binary
# See https://github.com/microsoft/vcpkg/issues/21218 for issues running vcpkg on Alpine

cd build/Alpine/build
strip -o latchystripped latchy
docker build --tag latchy -f ../../../Dockerfile_dist .

