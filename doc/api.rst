API reference
#############

.. ----------------------------------------------------------------------------

User
===========

.. doxygengroup:: user
   :content-only:
   :members:

.. ----------------------------------------------------------------------------

Log
===========

.. doxygengroup:: log
   :content-only:
   :members:

.. ----------------------------------------------------------------------------

Component
=============

API
-------------

.. doxygenclass:: lm::Component
.. doxygenfunction:: lm::comp::create(const std::string&, Component *)
.. doxygenfunction:: lm::comp::create(const std::string&, Component *, const Json&)
.. doxygendefine:: LM_COMP_REG_IMPL

Notes
-------------

Component instance and module boundary
"""""""""""""""""""""""""""""""""""""""""

A component instance might be instantiated inside another module boundary (e.g., dynamic link library, shared library). ``lm::comp::create`` returns unique_ptr so the user do not need to care about where the component is instantiated [] as long as ``lm::comp::create`` is used. A problem, however, arises when you want to manage raw pointers of components, because the deletion of the instance must happen in the same module. For this purpose, we provide ``lm::comp::detail::releaseFunc`` to get implementation-specific deleter function.

Examples
-------------

Basic instance creation
"""""""""""""""""""""""

In this example, we will define a simple interface ``A``
and its implementation ``A1``. The component interface must inherit ``lm::Component``
and the component implementation must be registered to the framework via ``LM_COMP_REG_IMPL``.

.. literalinclude:: ../test/test_component.cpp
   :language: cpp
   :start-after: // _begin_snippet: A
   :end-before: // _end_snippet: A
   :linenos:

Having registered the implementation, we can instantiate the component using ``lm::comp::create`` via the name that we specified; in this example, ``test::comp::a1``. ``lm::comp::create`` returns unique_ptr of the component interface type with proper release function.

.. literalinclude:: ../test/test_component.cpp
   :language: cpp
   :start-after: // _begin_snippet: A_impl
   :end-before: // _end_snippet: A_impl
   :linenos:
   :lines: 1,3-4
   :dedent: 8


Native plugin 
"""""""""""""

.. note:: Unlike Lightmetrica Version 2, ``Component`` does not support ABI-compatible plugins.
   Native component plugins must be compiled with the ABI-compatible compilers to the main library.

Python plugin
"""""""""""""

