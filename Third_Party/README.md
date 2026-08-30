# Third-party dependencies

## nanopb

`nanopb` is a Git submodule pinned to nanopb v0.4.9.2 (commit
`160d4f09e5fabb2b66aa2dea32d4f38ace2c4b3f`) and is licensed under the zlib
license.

The firmware compiles only nanopb's allocation-free protobuf runtime:
`pb_common.c`, `pb_encode.c`, and `pb_decode.c`. Protocol definitions live in
`protocol/`; their generated `.pb.c` and `.pb.h` files are checked in so normal
firmware builds do not require the generator.

Clone this project with its dependencies using:

```sh
git clone --recurse-submodules <repository-url>
```

For an existing checkout, run:

```sh
git submodule update --init --recursive
```
