Note for developers
=====================

doxygen
----------

Nested namespace
"""""""""""""""""""

C++ parser in doxygen (checked: ``doxygen 1.8.17``) does not support nested namespace in C++17.


pybind11
----------

Binding unique_ptr with custom deleter 
""""""""""""""""""""""""""""""""""""""""

If we want to bind unique_ptr with custom deleter using pybind11,
the type of the deleter must be specified by ``decltype()``.
The specifialization for ``std::function<void(T*)>`` or ``void(*)(T*)`` generates a runtime error.
