Lightmetrica Version 3
====================

Research-oriented renderer and environment for researchers and developers

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
$ conda install sphinx
$ pip install sphinx-autobuild sphinx_rtd_theme breathe sphinx_tabs pytest
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
$ git clone --depth 1 git@github.com:pybind/pybind11.git && \
  git clone --depth 1 git@github.com:nlohmann/json.git && \
  git clone --depth 1 git@github.com:g-truc/glm.git && \
  git clone --depth 1 git@github.com:onqtam/doctest.git && \
  git clone --depth 1 git@github.com:gabime/spdlog.git && \
  git clone --depth 1 git@github.com:fmtlib/fmt.git
```

Build library

```bash
$ source activate lm3
$ mkdir build && cd build
$ cmake -G "Visual Studio 15 2017 Win64" ..
$ start lightmetrica-v3.sln
```
### Run pytest

Execute following commands after build:

```bash
$ python -m pytest --lm <Binary directory of lightmetrica> lm/pytest
```

For instance, if you want to use Debug build, run

```bash
$ python -m pytest --lm build/lm/Debug lm/pytest
```

