# ---
# jupyter:
#   jupytext:
#     formats: ipynb,py:light
#     text_representation:
#       extension: .py
#       format_name: light
#       format_version: '1.4'
#       jupytext_version: 1.2.1
#   kernelspec:
#     display_name: Python 3
#     language: python
#     name: python3
# ---

# + {"raw_mimetype": "text/restructuredtext", "active": ""}
# .. _example_custom_material:
#
# Rendering with custom material
# ================================
#
# This example demostrates how to extend the framework using user-defined plugins. We extend :cpp:class:`lm::Material` interface to implement an user-defined material. We will implement the extension using C++ as a plugin. The implementation is defined in ``material_visualize_normal.cpp`` (`code <https://github.com/hi2p-perim/lightmetrica-v3/tree/master/functest/material_visualize_normal.cpp>`__):
#
# .. literalinclude:: ../../functest/material_visualize_normal.cpp
#     :language: cpp
#     :lines: 6-
#    
# In the first line you want to include ``lm.h``. The header provides everything necessary to use Lightmetrica in C++. :cpp:class:`lm::Material` interface provides several virtual function to be implemented. In this example, we are only interested in :cpp:func:`lm::Material::reflectance` function being used to fetch colors in ``raycast`` renderer.
#
# To register the implementation to the framework, you want to use :cpp:func:`LM_COMP_REG_IMPL` macro in the global scope. The second argument describes the name of the implementation, which will be used to instantiate the class.
#
# Once you prepared the code, you can easily build the plugin with ``lm_add_plugin`` in the `CMake script <https://github.com/hi2p-perim/lightmetrica-v3/tree/master/functest/CMakeLists.txt>`__. To use the function, you need to include ``LmAddPlugin.cmake``. You want to specify the name of the plugin with ``NAME`` argument. The dynamic library of the specified name will be built. In ``SOURCES`` argugment, you can specify the sources containing component implementations.
#
# .. literalinclude:: ../../functest/CMakeLists.txt
#     :language: cmake
#     :lines: 8-11
# -

import os
import numpy as np
import imageio
# %matplotlib inline
import matplotlib.pyplot as plt
import lmfunctest as ft
import lightmetrica as lm
# %load_ext lightmetrica_jupyter

lm.init()
lm.log.init('logger::jupyter')
lm.progress.init('progress::jupyter')
lm.info()

# + {"raw_mimetype": "text/restructuredtext", "active": ""}
# A plugin can be loaded by :cpp:func:`lm::comp::loadPlugin` function where you specify the path to the plugin as an argument. You don't want to specify the extension of the dynamic library because it is inferred according to the platform.
# -

# Load plugin
lm.comp.loadPlugin(os.path.join(ft.env.bin_path, 'functest_material_visualize_normal'))

# + {"raw_mimetype": "text/restructuredtext", "active": ""}
# We can use the loaded extension in the same way as build-in assets using :cpp:func:`lm::asset` function. We feed the material to the obj model to apply the loaded material to the mesh.

# +
# Custom material
lm.asset('visualize_normal_mat', 'material::visualize_normal', {});

# OBJ model
lm.asset('obj1', 'model::wavefrontobj', {
    'path': os.path.join(ft.env.scene_path, 'fireplace_room/fireplace_room.obj'),
    'base_material': lm.asset('visualize_normal_mat')
})

# +
# Film for the rendered image
lm.asset('film1', 'film::bitmap', {
    'w': 1920,
    'h': 1080
})

# Pinhole camera
lm.asset('camera1', 'camera::pinhole', {
    'position': [5.101118, 1.083746, -2.756308],
    'center': [4.167568, 1.078925, -2.397892],
    'up': [0,1,0],
    'vfov': 43.001194
})

# Camera
lm.primitive(lm.identity(), {
    'camera': lm.asset('camera1')
})

# Create primitives from model asset
lm.primitive(lm.identity(), {
    'model': lm.asset('obj1')
})

# + {"raw_mimetype": "text/restructuredtext", "active": ""}
# Executing the renderer will produce the following image.
# -

lm.build('accel::sahbvh', {})
lm.render('renderer::raycast', {
    'output': lm.asset('film1'),
    'use_constant_color': True
})

img = np.copy(lm.buffer(lm.asset('film1')))
f = plt.figure(figsize=(15,15))
ax = f.add_subplot(111)
ax.imshow(np.clip(np.power(img,1/2.2),0,1), origin='lower')
plt.show()
