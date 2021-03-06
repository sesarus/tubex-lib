# ==================================================================
#  tubex-lib - tests
# ==================================================================

#!/bin/bash

TUBELIB_DIR="$(pwd)"

cd ..
sh $TUBELIB_DIR/scripts/ibex/build_IbexLib.sh
cd $TUBELIB_DIR

mkdir build -p
cd build
cmake -DCMAKE_INSTALL_PREFIX=~/ -DIBEX_ROOT=~/ibex -DBUILD_TESTS=ON ..
make
cd ..