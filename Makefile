ARGS := $(wordlist 2,$(words $(MAKECMDGOALS)),$(MAKECMDGOALS))
$(eval $(ARGS):;@:)
makefileDir := $(dir $(firstword $(MAKEFILE_LIST)))

CMAKE := /usr/bin/cmake
USESTATIC := 1
default: build

#
# Build locally using either the native toolchain or emscripten
#

.PHONY: bootstrap
bootstrap:
	git submodule update --init vcpkg && vcpkg/bootstrap-vcpkg.sh --disableMetrics

.PHONY: clean
clean:
	rm -rf build
	rm -rf vcpkg/buildtrees

.PHONY: configureDebug
configureDebug:
	@$(CMAKE) cmake -S . -B build -DCMAKE_VERBOSE_MAKEFILE=ON -DCMAKE_TOOLCHAIN_FILE=vcpkg/scripts/buildsystems/vcpkg.cmake $(if $(USESTATIC),-D CMAKE_EXE_LINKER_FLAGS=\"-static\") -D CMAKE_BUILD_TYPE=Debug

.PHONY: configure
configure:
	@$(CMAKE) -S . -B build -DCMAKE_TOOLCHAIN_FILE=vcpkg/scripts/buildsystems/vcpkg.cmake -D CMAKE_BUILD_TYPE=Release $(if $(USESTATIC),-D CMAKE_EXE_LINKER_FLAGS=\"-static\")

.PHONY: build
build:
	@$(CMAKE) --build build

#
# Build in a container, mostly for producing a MUSL based static binary. Using Alpine as the base environment.
#
.PHONY: buildImage
buildImage:
	@$(makefileDir)/buildStatic.sh

.PHONY: packageDocker
packageDocker: buildImage
	strip -o build/Alpine/build/latchystripped build/Alpine/build/latchy
	docker build --tag latchy -f ./Dockerfile_dist build/Alpine/build

