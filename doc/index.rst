.. Lightmetrica Version 3 documentation master file, created by
   sphinx-quickstart on Fri Feb 23 15:44:26 2018.
   You can adapt this file completely to your liking, but it should at least
   contain the root `toctree` directive.

Lightmetrica -- Research-oriented renderer
============================================

.. warning::

    `This project is under development for the initial release of Version 3. Please be careful using the versions in development because they might incur abrupt API changes.`

Welcome to Lightmetrica Version 3 documentation!
This documentation includes **guide**, **examples and tests**, and **references**.

**Guide** explains basic to advanced usage of the framework, for instance how to build the framework from the source or how to render the images, or how to extend the framework.

**Example and tests** illustrates actual usage of the framework using Python/C++ API and the results of functional/performance/statistics tests. The examples and tests are written in a Jupyter notebook and the results are always executed in a CI build. It guarantees that the results are always generated from the working version of the framework. In other words, the broken documentation indicates a new commit ruined the runtime behavior of the framework.

**References** describe the explanation of the detailed API and the components of the framework. API reference describes function-wise or interface-wise features of the framework. It is especially useful when you want develop your own extensions. Build-in component references describes the built-in components of the interfaces such as materials or renderers. It also explains a brief background of the theory behind the component. This reference is useful when you want to use the renderer as a user.

----

.. toctree::
    :maxdepth: 1

    intro
    changelog

.. toctree::
    :maxdepth: 2
    :caption: Guide

    build
    user
    
.. toctree::
    :maxdepth: 2
    :caption: Advanced Topics

    build_system
    component
    python_binding

.. toctree::
    :maxdepth: 2
    :caption: Examples and tests

    test
    example
    functest
    perftest

.. toctree::
    :maxdepth: 2
    :caption: References

    component_ref
    api_ref

.. todolist::

Indices and tables
-------------------

* :ref:`genindex`
* :ref:`modindex`
* :ref:`search`



