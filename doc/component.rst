Component
===============

*Component* is responsible for object handling in Lightmetrica. 

Features
-------------

| **Dependency-injection**:
| An implementation is perfectly decoupled from the interface.
  We provide a simple factory function to create instances.

| **Native/Python plugin support**:
| An developer can easily extend features with dynamically loaded
  native plugins, or even with python scripts.

| **Assembly agnostic**:
| An component implementation can be accessible from the same interface
  irrespective to the assembly where the implementation is defined.

| **Serialization/deserialization**:
| We provide an interface for serialization/deserialization
  of component instances, simple yet sufficient to handle practical cases with dependent instances.

API
-------------

.. doxygenclass:: lm::Component
.. doxygenfunction:: lm::comp::create(const char *)
.. doxygenfunction:: lm::comp::create(const char *, const json&, Component *)
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

.. literalinclude:: ../lm/test/test_comp.cpp
   :language: cpp
   :start-after: // _begin_snippet: A
   :end-before: // _end_snippet: A
   :linenos:

Having registered the implementation, we can instantiate the component using ``lm::comp::create`` via the name that we specified; in this example, ``test::comp::a1``. ``lm::comp::create`` returns unique_ptr of the component interface type with proper release function.

.. literalinclude:: ../lm/test/test_comp.cpp
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

