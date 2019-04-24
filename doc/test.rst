Tests in Lightmetrica
########################

Lightmetrica implements various tests to maintain the quality of the framework.
We used four types of tests: *unit test*, *functional test*, *performance test*, *statistical test*. All tests are designed to be automated and thus possible to be integrated into a CI service.

Unit test
==========================

*Unit testing* is responsible for the tests of indivisual components of the framework. We mainly use unit testing to test interal features like component object manipulation, assets management, or scene management. 
In addition to the tests of the internal features written in C++, we provides unit tests for the Python binding of the framework.

- `Unit tests in C++`_
- `Unit tests in Python`_

.. _Unit tests in C++: https://github.com/hi2p-perim/lightmetrica-v3/tree/master/test
.. _Unit tests in Python: https://github.com/hi2p-perim/lightmetrica-v3/tree/master/pytest

Functional test
==========================

*Functional testing* is responsible for testing behavioral aspects of the framework without requiring internal knowledge of the framework. 
In other words, functional testing is in charge for the tests of possible usage by users using *only* the exposed APIs.
In software development, functional testing is coined as a test to check if software or library functions properly according to the requirements.
This means we can use functional tests to define the requirements of the framework.

- :ref:`example`
- :ref:`functest`

Performance test
==========================

*Performance testing* is responsible for testing performance requirements of the framework. We rather focus on testing relative performance rather than absolute performance, because relative performance comparison is what research and development requires. We mainly use the tests to compare performances of multiple implementations of the same interface, for instance, the performance of acceleration structures, scene loading, etc.

- :ref:`perftest`

Statistics test
==========================

*Statistics testing* is responsible for testing correctness of the behavior of the framework using statistical approaches.
We can describe statistical test being a kind of functional test where the correctness of the behavior is evaluated only by the statistical verifications.
We used the tests to verify the correctness of sampling functions or rendering techniques.

