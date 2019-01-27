Component
############

This section describes build-in components.
The label of each entry shows the key with ``interface::implementation`` format used in the instantiation of the component.
The parameters of the components are given as an argument of Json type to :cpp:func:`lm::Component::construct` function.

Material
======================

Components implementing :cpp:class:`lm::Material`.

.. include:: ../src/material/material_diffuse.cpp
   :start-after: \rst
   :end-before: \endrst

.. include:: ../src/material/material_mirror.cpp
   :start-after: \rst
   :end-before: \endrst

.. include:: ../src/material/material_glass.cpp
   :start-after: \rst
   :end-before: \endrst

.. include:: ../src/material/material_proxy.cpp
   :start-after: \rst
   :end-before: \endrst

.. include:: ../src/material/material_mask.cpp
   :start-after: \rst
   :end-before: \endrst

Acceleration structure
======================

Components implementing :cpp:class:`lm::Accel`.

.. include:: ../src/accel/accel_sahbvh.cpp
   :start-after: \rst
   :end-before: \endrst

Camera
======================

Components implementing :cpp:class:`lm::Camera`.

.. include:: ../src/camera/camera_pinhole.cpp
   :start-after: \rst
   :end-before: \endrst

Film
======================

Components implementing :cpp:class:`lm::Film`.

.. include:: ../src/film/film_bitmap.cpp
   :start-after: \rst
   :end-before: \endrst

Light
======================

Components implementing :cpp:class:`lm::Light`.

.. include:: ../src/light/light_area.cpp
   :start-after: \rst
   :end-before: \endrst


