.. _component_ref:

Built-in component reference
################################

This section describes build-in components.
The label of each entry shows the key with ``interface::implementation`` format used in the instantiation of the component.
The parameters of the components are given as an argument of Json type to :cpp:func:`lm::Component::construct` function.

Material
======================

Components implementing :cpp:class:`lm::Material`.

.. include:: ../src/material/material_diffuse.cpp
   :start-after: \rst
   :end-before: \endrst

.. include:: ../src/material/material_glossy.cpp
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

.. include:: ../src/material/material_proxy.cpp
   :start-after: \rst
   :end-before: \endrst

.. include:: ../src/material/material_mixture.cpp
   :start-after: \rst
   :end-before: \endrst

.. include:: ../src/material/material_mixture.cpp
   :start-after: \rst2
   :end-before: \endrst2

.. include:: ../src/model/model_wavefrontobj.cpp
   :start-after: \rst3
   :end-before: \endrst3

Camera
======================

Components implementing :cpp:class:`lm::Camera`.

.. include:: ../src/camera/camera_pinhole.cpp
   :start-after: \rst
   :end-before: \endrst

Light
======================

Components implementing :cpp:class:`lm::Light`.

.. include:: ../src/light/light_area.cpp
   :start-after: \rst
   :end-before: \endrst

.. include:: ../src/light/light_env.cpp
   :start-after: \rst
   :end-before: \endrst

.. include:: ../src/light/light_envconst.cpp
   :start-after: \rst
   :end-before: \endrst

.. include:: ../src/light/light_point.cpp
   :start-after: \rst
   :end-before: \endrst

.. include:: ../src/light/light_directional.cpp
   :start-after: \rst
   :end-before: \endrst

Medium
======================

Components implementing :cpp:class:`lm::Medium`.

.. include:: ../src/medium/medium_homogeneous.cpp
   :start-after: \rst
   :end-before: \endrst

.. include:: ../src/medium/medium_heterogeneous.cpp
   :start-after: \rst
   :end-before: \endrst

Volume
======================

Components implementing :cpp:class:`lm::Volume`.

.. include:: ../src/volume/volume_constant.cpp
   :start-after: \rst
   :end-before: \endrst

.. include:: ../src/volume/volume_checker.cpp
   :start-after: \rst
   :end-before: \endrst

Phase
======================

Components implementing :cpp:class:`lm::Phase`.

.. include:: ../src/phase/phase_isotropic.cpp
   :start-after: \rst
   :end-before: \endrst

.. include:: ../src/phase/phase_hg.cpp
   :start-after: \rst
   :end-before: \endrst

Mesh
======================

Components implementing :cpp:class:`lm::Mesh`.

.. include:: ../src/mesh/mesh_raw.cpp
   :start-after: \rst
   :end-before: \endrst

.. include:: ../src/model/model_wavefrontobj.cpp
   :start-after: \rst2
   :end-before: \endrst2
   
Texture
======================

Components implementing :cpp:class:`lm::Texture`.

.. include:: ../src/texture/texture_constant.cpp
   :start-after: \rst
   :end-before: \endrst

.. include:: ../src/texture/texture_bitmap.cpp
   :start-after: \rst
   :end-before: \endrst

Model
======================

Components implementing :cpp:class:`lm::Model`.

.. include:: ../src/model/model_wavefrontobj.cpp
   :start-after: \rst
   :end-before: \endrst

Acceleration structure
======================

Components implementing :cpp:class:`lm::Accel`.

.. include:: ../src/accel/accel_sahbvh.cpp
   :start-after: \rst
   :end-before: \endrst

Film
======================

Components implementing :cpp:class:`lm::Film`.

.. include:: ../src/film/film_bitmap.cpp
   :start-after: \rst
   :end-before: \endrst

