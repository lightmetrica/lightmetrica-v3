Python binding
########################

Python API
====================================

The features of the framework can be directly accessible via Python API so that we can directly manipulate the renderer from the experimental codes. This Python binding is achieved with `pybind11 <https://github.com/pybind/pybind11>`_ library.

We can manipulate the scene or organize the renderer jobs only with the Python API. Although we also expose C++ API as well as Python API, our recommendation is to use Python API as much as possible. We believe our Python API is flexible enough to cover the most of experimental workflow. Technically, we can even extend the renderer only with Python API, e.g., extending rendering techniques or materials, although we do not recommend it due to the massive performance loss. For detail, find corresponding examples in :ref:`functest`.

Also, our Python API supports automatic type conversion of the types between the corresponding Python and C++ interfaces. Some of the internal types appeared in the C++ API are automatically converted to the corresponding types in Python. For instance, math types (``lm::Vec3``, ``lm::Mat4``) are converted to numpy array and Json type is converted to a ``dict``. 
