Lightmetrica -- A research-oriented renderer
====================

[![Build Status](https://travis-ci.com/lightmetrica/lightmetrica-v3.svg?branch=master)](https://travis-ci.com/lightmetrica/lightmetrica-v3)
[![Documentation](https://img.shields.io/badge/docs-Sphinx-blue.svg)](https://hi2p-perim.github.io/lightmetrica-v3-doc/)
[![GitHub license](https://img.shields.io/badge/license-MIT-blue.svg)](https://github.com/lightmetrica/lightmetrica-v3/blob/master/LICENSE)
<!-- [![GitHub Releases](https://img.shields.io/github/release/hi2p-perim/lightmetrica-v3.svg)](https://github.com/hi2p-perim/lightmetrica-v3/releases) -->

*This project is under development for the initial release of Version 3. Please be careful to use the versions in development because they might incur abrupt API changes.*

![teaser](doc/_static/example/pt_fireplace_room.jpg)

**Lightmetrica** is a research-oriented renderer. The development of the framework is motivated by the goal to provide a practical environment for rendering research and development, where the researchers and developers need to tackle various challenging requirements through the development process.

## Quick start

Install [miniconda](https://docs.conda.io/en/latest/miniconda.html) and type the following commands.

```bash
$ conda create -n lm3 python=3.7
$ conda activate lm3
$ conda install lightmetrica -c conda-forge -c hi2p-perim
$ python
>>> import lightmetrica as lm
>>> lm.init()
>>> lm.info()
```

## Documentation

You can find tutorials and API references from [here](https://hi2p-perim.github.io/lightmetrica-v3-doc/).

## Features

- Extension support
  - Component object model that allows to extend any interfaces as plugins
  - Serialization of component objects
  - Position-agnostic plugins
- Verification support
  - Visual debugger
  - Performance measurements
  - Statistical verification
- Orchestration support
  - Complete set of Python API
  - Jupyter notebook integration

## Author

Hisanari Otsu ([Personal site](http://lightmetrica.org/h-otsu/), [Twitter](https://twitter.com/hisanari_otsu))

## License

This software is distributed under MIT License. For details, see LICENCE file.
