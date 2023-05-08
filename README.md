# QuadWild with Bi-MDF solver

This repo is a fork of [nicopietroni/quadwild](https://github.com/nicopietroni/quadwild). The original readme is preserved as [README_orig.md](README_orig.md)

Its main purpose is to demonstrate the use of Bi-MDF quantization, see our [Project page](https://www.algohex.eu/publications/bimdf-quantization).

The main code for Bi-MDF modeling can be found in [libs/quadretopology/quadretopology/qr_flow.cpp](libs/quadretopology/quadretopology/qr_flow.cpp)

## Changes:
- Add Bi-MDF solver for quantization that can be used instead of Gurobi IQPs
- Changed to CMake build system
- Fixed alignment bug
- Changed commandline arguments
- Extended config file format


## License

QuadWild as well as our additions and changes are licensed under the [GPL3](LICENSE).
