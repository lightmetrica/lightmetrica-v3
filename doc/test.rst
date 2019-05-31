Tests in Lightmetrica
########################

Test types
==========================

Lightmetrica implements various tests to maintain the quality of the framework.
We used four types of tests: *unit test*, *functional test*, *performance test*, *statistical test*. All tests are designed to be automated and thus possible to be integrated into a CI service.

**Unit test**.
*Unit testing* is responsible for the tests of indivisual components of the framework. We mainly use unit testing to test interal features like component object manipulation, assets management, or scene management. 
In addition to the tests of the internal features written in C++, we provides unit tests for the Python binding of the framework.

- `Unit tests in C++`_
- `Unit tests in Python`_

.. _Unit tests in C++: https://github.com/hi2p-perim/lightmetrica-v3/tree/master/test
.. _Unit tests in Python: https://github.com/hi2p-perim/lightmetrica-v3/tree/master/pytest

**Functional test**.
*Functional testing* is responsible for testing behavioral aspects of the framework without requiring internal knowledge of the framework. 
In other words, functional testing is in charge for the tests of possible usage by users using *only* the exposed APIs.
In software development, functional testing is coined as a test to check if software or library functions properly according to the requirements.
This means we can use functional tests to define the requirements of the framework.

- :ref:`example`
- :ref:`functest`

**Performance test**.
*Performance testing* is responsible for testing performance requirements of the framework. We rather focus on testing relative performance rather than absolute performance, because relative performance comparison is what research and development requires. We mainly use the tests to compare performances of multiple implementations of the same interface, for instance, the performance of acceleration structures, scene loading, etc.

- :ref:`perftest`

**Statistical test**.
*Statistical testing* is responsible for testing correctness of the behavior of the framework using statistical approaches.
We can describe statistical test being a kind of functional test where the correctness of the behavior is evaluated only by the statistical verifications.
We used the tests to verify the correctness of sampling functions or rendering techniques.

Executing tests
==========================

Executing unit tests
--------------------------

To execute unit tests, run the following command after :ref:`building the framework from source <building_from_source>`.
You may need to specify ``LD_LIBRARY_PATH`` for testing of plugins in Linux environment.

.. code-block:: console

    $ cd <lightmetrica binary directory>
    $ ./lm_test                     # Windows
    $ LD_LIBRARY_PATH=. ./lm_test   # Linux

Additionally, you can execute the Python tests with the following commands.

.. code-block:: console

    $ cd <source directory>
    $ python -m pytest --lm <lightmetrica binary dir> pytest

Executing functional tests
--------------------------

To execute all functional tests, you first want to create ``.lmenv`` file
where the runtime information is written. ``.lmenv`` file is a simple JSON file having a format as follows:

.. code-block:: json

    {
        "path": "<Source directory>",
        "bin_path": "<Binary directory>",
        "scene_path": "<Scene directory>"
    }

Put this file in ``functest`` directory, and run the follow commands to execute all functional tests. The command executes the Jupyter notebooks inside the ``functest`` directory and outputs the executed notebook in the ``executed_functest`` directory.

.. code-block:: console

    $ cd functest
    $ python run_all.py