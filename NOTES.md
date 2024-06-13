# C++ with cmake and vcpkg

An example project that demonstrates a setup using CMake + vcpkg for a
reasonable build with reasonable dependency management:
1. `CMakeLists.txt` contains a project definition and `vcpkg` integration bits.
2. `vcpkg.json` is a manifest file that defines a list of project dependencies.
3. `vcpkg` itself is checked-out as a submodule, so don't forget to run:
   1. `git submodule update --init`.
   2. `vcpkg/bootstrap-vcpkg.sh`.


The top-level `Makefile` defines a number of phony targets that act as shortcuts for specific cmake invocations:
* `bootstrap` sets up `vcpkg`.
* `configure/reconfigure` prepare cmake build.
* `build/run` to compile and run the binary.
* `clean`.


# Addition
The skeleton project was https://github.com/artempyanykh/vcpkg-cmake-example

Tutorial about using manifest mode and adding external dependencies
https://learn.microsoft.com/en-us/vcpkg/consume/manifest-mode?tabs=cmake%2Cbuild-cmake

# Botan
Botan is not well supported by vcpkg, a FindBotan.cmake file needs to be created. See modules/FindBotan.cmake
