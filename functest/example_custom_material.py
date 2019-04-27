# ---
# jupyter:
#   jupytext:
#     formats: ipynb,py:light
#     text_representation:
#       extension: .py
#       format_name: light
#       format_version: '1.3'
#       jupytext_version: 1.0.1
#   kernelspec:
#     display_name: Python 3
#     language: python
#     name: python3
# ---

# + {"raw_mimetype": "text/restructuredtext", "active": ""}
# .. _example_custom_material
#
# Rendering with custom material
# ================================
#
# This example demostrates how to extend the framework using user-defined plugins. We extend :cpp:class:`lm::Material` interface to implement an user-defined material. The implementation is defined in ``renderer_ao.cpp``.
#
# :cpp:class:`lm::Material` interface provides several virtual function to be implemented.
# In this example, we are only interested in :cpp:func:`lm::Material::reflectance` function being used to fetch colors in ``raycast`` renderer.
#
# .. literalinclude:: ../functest/custom_material.cpp
#     :language: cpp
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

# Load plugin
lm.comp.loadPlugin(os.path.join(ft.env.bin_path, 'functest_material_visualize_normal'))

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
# -

lm.build('accel::sahbvh', {})
lm.render('renderer::raycast', {
    'output': lm.asset('film1'),
    'use_constant_color': True
})

img = np.flip(np.copy(lm.buffer(lm.asset('film1'))), axis=0)
f = plt.figure(figsize=(15,15))
ax = f.add_subplot(111)
ax.imshow(np.clip(np.power(img,1/2.2),0,1))
plt.show()
