Lightmetrica -- A research-oriented renderer
====================

*This project is under development for the initial release of Version 3.*

![teaser](doc/_static/example/pt_fireplace_room.jpg)

**Lightmetrica** is a research-oriented renderer. The development of the framework is motivated by the goal to provide a practical environment for rendering research and development, where the researchers and developers need to tackle various challenging requirements through the development process.

## Quick start

Installation:

```bash
$ pip install lightmetrica
```

Create a Python script and run:

```python
import lightmetrica as lm
print(lm.version())
```

## Documentation

You can find tutorials and API references from [here](http://lightmetrica.org/doc).

## Features

- Basic rendering support
  - Easy-to-use API for rendering
  - Parameter configuration as Json type
  - Tristimulus/spectral rendering
  - Volume rendering
  - Network rendering
  - Differential rendering
  - Pause and resume rendering 
  - Standard scene formats (Wavefront OBJ, Mitsuba, PBRT)
  - Various implementation of rendering techniques
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
