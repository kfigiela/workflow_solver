# Workflow-based FEM solver

## Building

### Requirements

* C++11 compatibile compiler, we use [Clang](http://clang.llvm.org)
* [ATLAS](http://math-atlas.sourceforge.net)
* [Boost](http://www.boost.org)

### Steps

1. Clone this repo with submodules (`git clone --recurse-submodules`)
2. Build `graph_grammar_solver` in `graph_grammar_solver/build` directory (or adjust paths) with Cmake
   ```
   pushd .; cd graph_grammar_solver && mkdir build && ATLAS_DIR=`pwd` && cd build && cmake .. -DATLAS_DIR=$ATLAS_DIR && make; popd
   ```
3. Build this project 
   ```
   mkdir build && cd build && cmake .. && make
   ```
   
   
## Memcached configuration

```shell
memcached -I 10m -M -m 2048m  -r 
```