# CSA

## Build

From the root folder, run `make build` to build the executable.

## Run

At first, make sure that the dataset directory is at the same level as this repository's directory.

    .
    ├── CSA
    └── Public-Transit-Data

Then `cd` into the `build` folder and run the `csa` executable.

    usage:
      csa [<name>] options
    
    where options are:
      --hl              Unrestricted walking with hub labelling
      -p, --profile     Run profile query
      -r, --ranked      Use ranked queries
      -?, -h, --help    display usage information

By default, the basic CSA will be run using 10000 pre-generated queries, whose sources, targets, and departures are selected
uniformly at random.
