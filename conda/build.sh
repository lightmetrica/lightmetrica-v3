#!/bin/bash

cmake -H. -B_build \
      -D CMAKE_BUILD_TYPE=Release \
      -D CMAKE_INSTALL_PREFIX=$PREFIX \
      -D LM_BUILD_TESTS=OFF \
      -D LM_BUILD_EXAMPLES=OFF

cmake --build _build --target install

$PYTHON -m pip install --no-deps --ignore-installed .
