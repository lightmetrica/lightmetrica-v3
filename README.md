Lightmetrica Version 3
====================

A research-oriented meta renderer.

## Setup

### Prerequisites

- doxygen
- miniconda

### Create python environment

```bash
$ conda create -n lm3 python=3.6
```

### Install required packages

```bash
$ source activate lm3
$ conda install -c conda-forge sphinx jupyter matplotlib
$ pip install sphinx-autobuild sphinx_rtd_theme breathe sphinx_tabs pytest tqdm
```

Optionally, you can install Jupyter notebook for some examples:

```bash
$ conda install -c conda-forge jupyter ipywidgets
```

### Display and edit documentation

Access `http://127.0.0.1:8000` after

```
$ sphinx-autobuild doc doc/_build/html
```

### Update Doxygen-generated files

```bash
$ cd doc
$ doxygen
```

### Build

Install external libraries

```bash
$ cd external
$ git clone --depth 1 git@github.com:pybind/pybind11.git
$ git clone --depth 1 git@github.com:nlohmann/json.git
$ git clone --depth 1 git@github.com:g-truc/glm.git
$ git clone --depth 1 git@github.com:onqtam/doctest.git
$ git clone --depth 1 git@github.com:fmtlib/fmt.git
$ git clone --depth 1 git@github.com:USCiLab/cereal.git
$ git clone --depth 1 git@github.com:agauniyal/rang.git
```

Build library

```bash
$ source activate lm3
$ mkdir build && cd build
$ cmake -G "Visual Studio 15 2017 Win64" ..
$ start lightmetrica.sln
```

### Build (docker)

```bash
> docker build -t lm3_dev -f Dockerfile_dev .
> docker run --rm --user root -v ${PWD}:/usr/src/lightmetrica-v3 -it lm3_dev /bin/bash
$ cd /usr/src/lightmetrica-v3
$ mkdir build && cd build
$ cmake -DCMAKE_BUILD_TYPE=Release ..
$ make -j
```

### Run pytest

Execute following commands after build:

```bash
$ python -m pytest --lm <Binary directory of lightmetrica> lm/pytest
```

For instance, if you want to use Debug build, run

```bash
$ python -m pytest --lm build/bin/Debug pytest
```

