# vodozemac-bindings

Collection of language bindings for the [vodozemac] cryptographic library.

[vodozemac]: https://github.com/matrix-org/vodozemac

Documentation on how to use the c++ bindings can be found at [IronLily](https://iron.lily-is.land/w/from-libolm-to-vodozemac/).

## Installation

It has the following build-time dependencies:

- cargo
- perl
- g++
- make
- cp, ln, install

If you want to build tests, you will also need:

- boost-json
- Catch2-3
- libolm

On Windows, the build dependencies can be satisfied by rustup (Please make sure you are targeting the GNU abi, **not MSVC**), perl, mingw, and msys. Make sure the tools you need are in your PATH or override them with the absolute path.

To build, run:

```
make -C cpp
```

To install, run:

```
make -C cpp PREFIX=somewhere install
```

https://lily-is.land/kazv/craft-blueprints-project-kazv/-/blob/servant/libs/vodozemac-bindings/vodozemac-bindings.py?ref_type=heads has an example of how to build it on Windows.
