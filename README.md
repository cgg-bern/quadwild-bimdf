# QuadWild with Bi-MDF solver

This repo is a fork of [nicopietroni/quadwild](https://github.com/nicopietroni/quadwild). The original readme is preserved as [README_orig.md](README_orig.md)

Its main purpose is to demonstrate the use of Bi-MDF quantization, see our [Project page](https://www.algohex.eu/publications/bimdf-quantization).

The main code for Bi-MDF modeling can be found in [libs/quadretopology/quadretopology/qr_flow.cpp](libs/quadretopology/quadretopology/qr_flow.cpp)

## Changes from original QuadWild

- Add Bi-MDF solver for quantization that can be used instead of Gurobi IQPs
- Changed to CMake build system
- Fixed alignment bug
- Changed commandline arguments
- Extended config file format
- Build without Gurobi -- controlled by CMake variable `QUADRETOPOLOGY_WITH_GUROBI`


## Building (classic)

```
cmake . -B build -DSATSUMA_ENABLE_BLOSSOM5=0
cmake --build build
```


## Usage

```
./build/Build/bin/quadwild path/to/input/mesh.obj  2 config/prep_config/basic_setup.txt
./build/Build/bin/quad_from_patches path/to/input/mesh_rem_p0.obj 123 config/main_config/flow_noalign_lemon.txt
```

Note that you need to add `_rem_p0.obj` to the filename of the input mesh; this is
the output from the main quadwild binary.

We included sample config files in `config/`. To improve output quality, adjust them to your needs :)


## Container-based usage with podman

The Dockerfile will most likely also work using Docker, but we didn't test this.

On MacOS, all podman containers live in a Linux VM; for a successful build, more memory is requried, and more cpus speed it up:
```
podman machine stop
podman machine set --memory 8192 podman-machine-default
podman machine set --cpus 4 podman-machine-default
podman machine start
```

Create the container:
```
podman build . --build-arg WITH_GUROBI=0 -t quadwild-bimdf
```

Run the binaries; you need to mount some volume into the container to access your data, for example mapping `/Users`:
```
podman run --rm  --volume /Users:/Users quadwild-bimdf quadwild arguments-as-described-above
podman run --rm  --volume /Users:/Users quadwild-bimdf quad_from_patches arguments-as-described-above
```



## License

QuadWild as well as our additions and changes are licensed under the [GPL3](LICENSE).


## Acknowledgements

This project has been developed as part of the [AlgoHex](https://www.algohex.eu/) project, which has received funding from the European Research Council (ERC) under the European Union’s Horizon 2020 research and innovation programme ([Grant agreement No. 853343](https://cordis.europa.eu/project/id/853343)).

