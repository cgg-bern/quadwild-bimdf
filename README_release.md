# QuadWild-BiMDF

This is a binary distribution of https://github.com/cgg-bern/quadwild-bimdf/
Version 0.0.2

## Usage

You should run the binaries from the release folder and give the full path to the input mesh.
The programs will save a bunch of intermediate results in the folder where the mesh is located, with filenames based on the original filename.


## Step one: Preparation

The first step will remesh the input (this improves robustness and runtime, however it can excessively simplify the input. If detail is lost, you can disablee in the config .txt file (`do_remesh 0` instead of `do_remesh 1`).
- There are two example configs for this preparation step:
    - `config/prep_config/basic_setup_Mechanical.txt`: Tries to find sharp edges in the input (to be preserved in the quad mesh) based on a dihedral angle threshold
    - `config/prep_config/basic_setup_Organic.txt`: No sharp edges, useful for smooth inputs.

```
.\quadwild.exe path/to/input/mesh.obj 2 config/prep_config/basic_setup.txt
```

This will create a bunch of files in `path/to/input`, you will have to pass the correct `.obj` to the next step.


## Step two: Quad mesh generation


```
.\quad_from_patches.exe path/to/input/mesh_rem_p0.obj 1 config/main_config/flow.txt
```

Again, here you can choose between multiple example configs (which you can also adapt. Make sure not to change the order of the entries, the parser is a bit finicky).
We recommend `flow.txt` and `flow_noalign.txt`. The first one tries to achieve a better edge flow, see if it works well for your input :)


