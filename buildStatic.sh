#!/bin/bash

# For compiling on Alpine and produce a fully static binary
# See https://github.com/microsoft/vcpkg/issues/21218 for issues running vcpkg on Alpine

mkdir -p build/Docker
cd build/Docker
docker build --tag buildlatchy -f ../../Dockerfile .
cd ../..

mkdir -p build/Alpine/build
mkdir -p build/Alpine/vcpkg
docker run --rm \
  -it \
  -v .:/Source \
  -v ./build/Alpine/build:/Source/build \
  -v ./build/Alpine/vcpkg:/Source/vcpkg \
  -v ./clevisLib:/Source/clevisLib \
  --name buildlatchy \
  buildlatchy
