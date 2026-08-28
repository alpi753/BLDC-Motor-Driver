# Third-party dependencies

## TinyCBOR

`TinyCBOR` is a Git submodule pinned to TinyCBOR v0.6.1 (commit
`c0aad2fb2137a31b9845fbaae3653540c410f215`) and is licensed under MIT.

The firmware compiles only its allocation-free encoder and parser core. JSON
conversion, validation, and pretty-printing sources are excluded. Application
code can include it with `#include "cbor.h"`.

Clone this project with its dependencies using:

```sh
git clone --recurse-submodules <repository-url>
```

For an existing checkout, run:

```sh
git submodule update --init --recursive
```
